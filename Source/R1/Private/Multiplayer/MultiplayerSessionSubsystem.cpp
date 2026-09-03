#include "Multiplayer/MultiplayerSessionSubsystem.h"

#include "Engine/Engine.h"	// 네트워크 실패 델리게이트 사용을 위해 포함
#include "Engine/World.h"	// GetWorld()가 반환하는 UWorld의 ServerTravel과 GetNetMode를 호출하려면 UWorld의 완전한 클래스 정의 필요
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

// 검색 결과에 포함시킬 사용자 정의 방 이름의 Key
// 호스트와 검색 측이 반드시 같은 Key를 사용해야 값을 다시 읽을 수 있기 때문에 .h에서 static const로 선언
const FName UMultiplayerSessionSubsystem::ServerNameSettingKey(TEXT("SERVER_NAME"));

UMultiplayerSessionSubsystem::UMultiplayerSessionSubsystem()
{
	// Online Session API는 요청 시작 여부만 bool로 즉시 반환
	// 최종 성공, 실패 여부는 나중에 Complete Delegate로 전달
	// 델리게이트를 구독해서 각 비동기 작업이 실제로 끝났을 때 호출될 멤버 함수를 지정
	CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnCreateSessionComplete);
	FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnFindSessionsComplete);
	JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnJoinSessionComplete);
	DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnDestroySessionComplete);
}

void UMultiplayerSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// DefaultEngine.ini의 DefaultPlatformService=Null 설정에 따라 OnlineSubsystemNull 인스턴스를 얻음
	// 세션 생성/검색/참가/제거는 모두 이 인터페이스를 통해 수행
	// UE5.8의 새로운 Online Services(UE::Online::ISessions)는 Beta Sessions API이기 때문에 사용하지 않음
	// 기존부터 있던 기능인 Online Subsystem 사용
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}

	// GEngine: UEngine의 전역 포인터이며 OnNetworkFailure Delegate의 소유자
	// Join 성공 후에도 연결 거부, 시간 초과, 호스트 종료가 발생할 수 있으므로 감시
	if (GEngine)
	{
		NetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(
			this, &UMultiplayerSessionSubsystem::HandleNetworkFailure);
	}
}

void UMultiplayerSessionSubsystem::Deinitialize()
{
	// GameInstance가 종료될 때 Online Subsystem에 남은 UObject Delegate를 제거
	// 다음 PIE 실행이나 종료 과정에서 이전 객체가 호출되는 것 방지
	ClearOnlineCompleteDelegateHandles();

	// 세션 연결 실패 델리게이트는 GEngine 소유이기 때문에 따로 정리
	if (GEngine && NetworkFailureDelegateHandle.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureDelegateHandle);
		NetworkFailureDelegateHandle.Reset();
	}

	// Shared Pointer 참조 해제
	SessionSearch.Reset();
	SessionInterface.Reset();
	Super::Deinitialize();
}

bool UMultiplayerSessionSubsystem::IsInSession() const
{
	return SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
}

bool UMultiplayerSessionSubsystem::IsHostingSession() const
{
	const UWorld* World = GetWorld();
	return IsInSession() && World && World->GetNetMode() == NM_ListenServer;
}

bool UMultiplayerSessionSubsystem::GetCurrentSessionInfo(FSessionListItem& OutSession) const
{
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	const FNamedOnlineSession* CurrentSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (!CurrentSession)
	{
		return false;
	}

	OutSession = FSessionListItem();
	OutSession.MaxPlayers = CurrentSession->SessionSettings.NumPublicConnections;
	OutSession.CurrentPlayers = OutSession.MaxPlayers - CurrentSession->NumOpenPublicConnections;
	OutSession.SearchResultIndex = INDEX_NONE;
	OutSession.bIsCurrentSession = true;

	// 호스트의 로컬 세션 슬롯 등록이 늦더라도 실제 PlayerState 수보다 적게 표시하지 않음
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GameState = World->GetGameState<AGameStateBase>())
		{
			OutSession.CurrentPlayers = FMath::Max(
				OutSession.CurrentPlayers,
				GameState->PlayerArray.Num());
		}
	}
	OutSession.CurrentPlayers = FMath::Clamp(OutSession.CurrentPlayers, 0, OutSession.MaxPlayers);

	if (!CurrentSession->SessionSettings.Get(ServerNameSettingKey, OutSession.ServerName))
	{
		OutSession.ServerName = TEXT("Unnamed Server");
	}

	return true;
}

bool UMultiplayerSessionSubsystem::ConsumeConnectionFailureMessage(FString& OutMessage)
{
	if (PendingConnectionFailureMessage.IsEmpty())
	{
		return false;
	}

	OutMessage = MoveTemp(PendingConnectionFailureMessage);
	PendingConnectionFailureMessage.Reset();
	return true;
}

void UMultiplayerSessionSubsystem::CreateSession(int32 NumPublicConnections, const FString& ServerName)
{
	// 이미 다른 서버에 접속한 클라이언트는 호스트 세션을 만들거나 ServerTravel을 시작할 수 없다.
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateSession rejected: clients cannot host a session"));
		OnCreateSessionResult.Broadcast(false);
		return;
	}

	// Interface, 입력값, 진행 상태를 먼저 검사해 잘못된 요청과 중복 클릭을 차단
	if (!SessionInterface.IsValid() || NumPublicConnections <= 0 || ServerName.IsEmpty() ||
		bCreateInProgress || bDestroyInProgress)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession failed: invalid state or arguments"));
		OnCreateSessionResult.Broadcast(false);
		return;
	}

	// 비동기 생성 완료 후 ServerTravel URL에도 같은 정원을 전달할 수 있도록 보관
	HostedMaxPlayers = NumPublicConnections;

	// Online Subsystem은 같은 Local Session Name(NAME_GameSession) 중복 생성 불가
	// 기존 세션이 있으면 입력값을 보관하고, Destroy 완료 후 다시 생성
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bCreateSessionAfterDestroy = true;	// 세션 재생성을 위한 Destroy임을 표시
		PendingNumPublicConnections = NumPublicConnections;
		PendingServerName = ServerName;
		DestroySession();
		return;
	}

	// 호스트가 LAN에 광고할 세션 규칙
	FOnlineSessionSettings Settings;
	// 생성할 세션을 LAN Match 지정
	Settings.bIsLANMatch = true;
	// 세션에 설정할 전체 Public Connection 슬롯 수
	Settings.NumPublicConnections = NumPublicConnections;
	// 다른 컴퓨터의 FindSessions 결과에 이 방이 나타나도록 광고 (방 목록에 이 방을 올림)
	Settings.bShouldAdvertise = true;
	// 게임 맵으로 이동한 뒤에도 검색 및 참가할 수 있게 설정
	Settings.bAllowJoinInProgress = true;
	// Presence, Invite는 LAN Null 목표에 필요하지 않으므로 사용하지 않음
	Settings.bAllowJoinViaPresence = false;
	Settings.bAllowInvites = false;

	// 호스트가 입력한 방 이름을 세션의 Custom Setting으로 저장
	Settings.Set(
		ServerNameSettingKey,	// 이 값이 방 이름이라는 것을 표시하는 Key
		ServerName,				// 실제로 표시될 사용자 지정 방 이름
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);	// 방 목록에 올릴 때 방 이름 정보도 같이 전달하도록 설정

	// Complete 델리게이트 등록 후, 나중에 이 바인딩만 해제할 때 사용하기 위해 핸들 저장
	CreateSessionDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	bCreateInProgress = true;	// 현재 세션 생성 비동기 작업이 진행 중인지 기록 (중복 요청 방지)

	// false는 비동기 작업 자체가 시작되지 않았다는 의미이기 때문에
	// Complete Callback이 오지 않으므로 여기서 직접 Handle과 실패 UI를 처리
	if (!SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		CreateSessionDelegateHandle.Reset();
		bCreateInProgress = false;	// 현재 세션 생성 비동기 작업이 진행 중인지 기록 (중복 요청 방지)
		OnCreateSessionResult.Broadcast(false);
	}
}

void UMultiplayerSessionSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid() || MaxSearchResults <= 0 || bFindInProgress)
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions failed: invalid state or arguments"));
		OnFindSessionsResult.Broadcast(false, TArray<FSessionListItem>());
		return;
	}

	// 검색은 비동기이므로 지역 변수가 아니라 멤버 TSharedPtr로 수명을 유지
	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = MaxSearchResults;
	SessionSearch->bIsLanQuery = true;	// 세션을 온라인 검색이 아니라 LAN 기반 세션 검색으로 수행하라고 지정

	// Complete 델리게이트 등록 후, 나중에 이 바인딩만 해제할 때 사용하기 위해 핸들 저장
	FindSessionsDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
	bFindInProgress = true;	// 현재 세션 찾기 비동기 작업이 진행 중인지 기록 (중복 요청 방지)

	// 비동기 요청 시작 실패 처리
	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		FindSessionsDelegateHandle.Reset();
		bFindInProgress = false;	// 현재 세션 찾기 비동기 작업이 진행 중인지 기록 (중복 요청 방지)
		OnFindSessionsResult.Broadcast(false, TArray<FSessionListItem>());
	}
}

void UMultiplayerSessionSubsystem::JoinSession(int32 SessionIndex)
{
	// UI가 받은 SearchResultIndex가 현재 검색 배열에서 여전히 유효한지 확인
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid() || bJoinInProgress ||
		!SessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession failed (session index: %d)"), SessionIndex);
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	// 클라이언트는 기존 로컬 세션을 제거한 뒤 선택한 다른 세션 참가를 자동으로 이어서 수행
	if (IsInSession())
	{
		if (IsHostingSession())
		{
			UE_LOG(LogTemp, Warning, TEXT("JoinSession rejected: a host cannot switch sessions directly"));
			OnJoinSessionResult.Broadcast(false);
			return;
		}

		bJoinSessionAfterDestroy = true;
		PendingJoinSessionIndex = SessionIndex;
		LeaveSession();
		return;
	}

	// Complete 델리게이트 등록 후, 나중에 이 바인딩만 해제할 때 사용하기 위해 핸들 저장
	JoinSessionDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	bJoinInProgress = true;	// 현재 세션 참가 비동기 작업이 진행 중인지 기록 (중복 요청 방지)
	
	// 비동기 요청 시작 실패 처리
	if (!SessionInterface->JoinSession(
		0, NAME_GameSession, SessionSearch->SearchResults[SessionIndex]))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		JoinSessionDelegateHandle.Reset();
		bJoinInProgress = false;	// 현재 세션 참가 비동기 작업이 진행 중인지 기록 (중복 요청 방지)
		OnJoinSessionResult.Broadcast(false);
	}
}

void UMultiplayerSessionSubsystem::DestroySession()
{
	// 현재 사용 중인 IOnlineSession에는 플랫폼 공통 LeaveSession 함수가 없기 때문에
	// 호스트는 광고 세션을 종료하고, 클라이언트는 자기 로컬 Named Session을 정리할 때
	// 호스트/클라이언트 관계 없이 모두 DestroySession을 사용
	// bWasHost를 사용해 UI에 두 상황을 구분

	const UWorld* World = GetWorld();
	const bool bWasHost = World && World->GetNetMode() == NM_ListenServer;	// 리슨 서버 호스트의 호출인지 판별
	if (!SessionInterface.IsValid() || bDestroyInProgress)
	{
		if (bCreateSessionAfterDestroy)
		{
			bCreateSessionAfterDestroy = false;
			OnCreateSessionResult.Broadcast(false);
		}
		else if (bJoinSessionAfterDestroy)
		{
			bJoinSessionAfterDestroy = false;
			PendingJoinSessionIndex = INDEX_NONE;
			OnJoinSessionResult.Broadcast(false);
		}
		else
		{
			OnDestroySessionResult.Broadcast(false, bWasHost);
		}
		return;
	}

	bDestroyingHostedSession = bWasHost;	// bDestroyingHostedSession과 bWasHost는 수명과 목적이 다르기 때문에 따로 저장

	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		if (bCreateSessionAfterDestroy)
		{
			const int32 NumPublicConnections = PendingNumPublicConnections;
			const FString ServerName = PendingServerName;
			bCreateSessionAfterDestroy = false;
			CreateSession(NumPublicConnections, ServerName);
		}
		else if (bJoinSessionAfterDestroy)
		{
			const int32 SessionIndex = PendingJoinSessionIndex;
			bJoinSessionAfterDestroy = false;
			PendingJoinSessionIndex = INDEX_NONE;
			JoinSession(SessionIndex);
		}
		else
		{
			OnDestroySessionResult.Broadcast(true, bWasHost);
			TravelToMainMenu();
		}
		return;
	}

	// Complete 델리게이트 등록 후, 나중에 이 바인딩만 해제할 때 사용하기 위해 핸들 저장
	DestroySessionDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
	bDestroyInProgress = true;	// 현재 세션 삭제 비동기 작업이 진행 중인지 기록 (중복 요청 방지)

	// 비동기 요청 시작 실패 처리
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		DestroySessionDelegateHandle.Reset();
		bDestroyInProgress = false;	// 현재 세션 삭제 비동기 작업이 진행 중인지 기록 (중복 요청 방지)

		if (bCreateSessionAfterDestroy)
		{
			bCreateSessionAfterDestroy = false;
			OnCreateSessionResult.Broadcast(false);
		}
		else if (bJoinSessionAfterDestroy)
		{
			bJoinSessionAfterDestroy = false;
			PendingJoinSessionIndex = INDEX_NONE;
			OnJoinSessionResult.Broadcast(false);
		}
		else
		{
			OnDestroySessionResult.Broadcast(false, bWasHost);
		}
	}
}

void UMultiplayerSessionSubsystem::LeaveSession()
{
	// 클라이언트 퇴장용 함수
	// Listen Server가 호출해서 방 전체를 종료하지 않도록 차단
	if (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer)
	{
		// 호스트에 의해 호출된 경우 실패 처리
		UE_LOG(LogTemp, Warning, TEXT("LeaveSession is for clients. Use EndHostedSession on a listen server."));
		OnDestroySessionResult.Broadcast(false, true);
		return;
	}

	DestroySession();
}

void UMultiplayerSessionSubsystem::EndHostedSession()
{
	// 호스트만 호출 가능 (호스트 전용 작업 처리)
	// Destroy 완료 후 호스트는 Lv_MainMenu로 이동
	// 연결된 클라이언트는 Network Failure/Connection Lost 처리를 받음
	if (!GetWorld() || GetWorld()->GetNetMode() != NM_ListenServer)
	{
		// 클라이언트에 의해 호출된 경우 실패 처리
		UE_LOG(LogTemp, Warning, TEXT("EndHostedSession requires a listen server."));
		OnDestroySessionResult.Broadcast(false, false);
		return;
	}

	DestroySession();
}

void UMultiplayerSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// Complete Callback은 성공/실패와 관계없이 한 번 왔으므로 Handle 해제
	if (SessionInterface.IsValid() && CreateSessionDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		CreateSessionDelegateHandle.Reset();
	}
	bCreateInProgress = false;	// 현재 세션 생성 비동기 작업이 진행 중인지 기록 (중복 요청 방지)

	// 세션 생성 결과 확인
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession failed for %s"), *SessionName.ToString());
		OnCreateSessionResult.Broadcast(false);
		return;
	}

	// 요청 후 월드 역할이 Client로 바뀐 경우에도 절대로 ServerTravel을 호출하지 않음
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateSession completed on a client; ServerTravel was blocked"));
		OnCreateSessionResult.Broadcast(false);
		return;
	}

	UWorld* World = GetWorld();

	// 서버 이동을 하며 MaxPlayers 옵션 전달: 클라이언트가 접속할 때 서버가 내부적으로 현재 인원 검사
	// -> AGameSession::ApproveLogin()
	const FString TravelURL = FString::Printf(
		TEXT("/Game/Maps/Lv_SessionTest?listen?MaxPlayers=%d"),
		HostedMaxPlayers
	);

	// Server Travel 요청 시작 여부 확인
	if (!World || !World->ServerTravel(TravelURL))
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession succeeded, but ServerTravel failed"));
		OnCreateSessionResult.Broadcast(false);
		DestroySession();	// 세션 생성은 됐기 때문에 이미 만들어진 세션 삭제
		return;
	}

	OnCreateSessionResult.Broadcast(true);	// 실패 없이 모두 진행됐으면 방 생성 성공
}

void UMultiplayerSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	// Complete Callback은 성공/실패와 관계없이 한 번 왔으므로 Handle 해제
	if (SessionInterface.IsValid() && FindSessionsDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		FindSessionsDelegateHandle.Reset();
	}
	bFindInProgress = false;	// 현재 세션 찾기 비동기 작업이 진행 중인지 기록 (중복 요청 방지)

	TArray<FSessionListItem> Items;
	// 세션 찾기를 성공했는지 확인
	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions failed"));
		OnFindSessionsResult.Broadcast(false, Items);
		return;
	}

	// 세션 찾기는 성공했지만 방이 없으면 찾기 성공 결과와 빈 배열을 UI에 전달함
	Items.Reserve(SessionSearch->SearchResults.Num());	// 찾은 만큼 메모리 공간 확보
	const FNamedOnlineSession* CurrentSession = SessionInterface->GetNamedSession(NAME_GameSession);
	for (int32 Index = 0; Index < SessionSearch->SearchResults.Num(); ++Index)
	{
		const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[Index];

		// Items 배열의 마지막에 기본값으로 초기화된 요소 하나를 추가한 후, 추가된 요소의 참조를 Item 변수에 저장
		FSessionListItem& Item = Items.AddDefaulted_GetRef();

		// 새롭게 추가된 요소의 값을 SessionSearch로 읽어온 Result 값으로 설정
		Item.SearchResultIndex = Index;
		Item.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Item.CurrentPlayers = Item.MaxPlayers - Result.Session.NumOpenPublicConnections;	// 참가 인원 수 = 최대 인원 수 - 열린(남은) 슬롯 수
		Item.Ping = Result.PingInMs;
		Item.bIsCurrentSession =
			CurrentSession && CurrentSession->SessionInfo.IsValid() && Result.Session.SessionInfo.IsValid() &&
			CurrentSession->SessionInfo->GetSessionId().ToString() == Result.Session.SessionInfo->GetSessionId().ToString();

		// 방 이름 찾기에 실패한 세션은 "Unnamed Server"로 설정
		if (!Result.Session.SessionSettings.Get(ServerNameSettingKey, Item.ServerName))
		{
			Item.ServerName = TEXT("Unnamed Server");
		}
	}

	OnFindSessionsResult.Broadcast(true, Items);
}

void UMultiplayerSessionSubsystem::OnJoinSessionComplete(
	FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// Complete Callback은 성공/실패와 관계없이 한 번 왔으므로 Handle 해제
	if (SessionInterface.IsValid() && JoinSessionDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		JoinSessionDelegateHandle.Reset();
	}
	bJoinInProgress = false;	// 현재 세션 참가 비동기 작업이 진행 중인지 기록 (중복 요청 방지)

	// Session Interface가 유효하지 않거나 세션 참가 결과가 성공이 아닌 경우 실패 처리
	if (!SessionInterface.IsValid() || Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession failed with result %d"), static_cast<int32>(Result));
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	// JoinSession은 논리적인 세션 참가만 완료 (실제 Client Travel은 별도 처리 필요)
	// 실제 IP:Port 접속 주소는 Online Subsystem 구현체가 제공하므로 직접 조립하지 않고 GetResolvedConnectString을 사용
	FString ConnectString;
	// 참가한 세션에서 ClientTravel에 사용할 연결 문자열을 해석할 수 있는지 확인
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("GetResolvedConnectString failed"));
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();	// 호스트와 클라이언트 각자 가짐
	APlayerController* PlayerController = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;
	// 유효한 플레이어인지 확인
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("No local PlayerController for ClientTravel"));
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	// ClientTravel을 통해 로컬 클라이언트가 Listen Server의 게임 월드에 접속
	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
	OnJoinSessionResult.Broadcast(true);
}

void UMultiplayerSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	// Complete Callback은 성공/실패와 관계없이 한 번 왔으므로 Handle 해제
	if (SessionInterface.IsValid() && DestroySessionDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		DestroySessionDelegateHandle.Reset();
	}
	bDestroyInProgress = false;	// 현재 세션 삭제 비동기 작업이 진행 중인지 기록 (중복 요청 방지)

	const bool bWasHost = bDestroyingHostedSession;
	bDestroyingHostedSession = false;

	// CreateSession 도중 발견한 기존 세션을 교체하기 위한 Destroy라면 메뉴로 가지 않고 저장해 둔 설정으로 새 세션 생성
	if (bCreateSessionAfterDestroy)
	{
		const int32 NumPublicConnections = PendingNumPublicConnections;
		const FString ServerName = PendingServerName;
		bCreateSessionAfterDestroy = false;

		if (bWasSuccessful)
		{
			CreateSession(NumPublicConnections, ServerName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Could not replace existing session %s"), *SessionName.ToString());
			OnCreateSessionResult.Broadcast(false);
		}
		return;
	}

	// 다른 방 참가를 위한 Destroy라면 메인 메뉴로 이동하지 않고 예약한 참가 요청을 계속 수행
	if (bJoinSessionAfterDestroy)
	{
		const int32 SessionIndex = PendingJoinSessionIndex;
		bJoinSessionAfterDestroy = false;
		PendingJoinSessionIndex = INDEX_NONE;

		if (bWasSuccessful)
		{
			JoinSession(SessionIndex);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Could not leave session %s before joining another"), *SessionName.ToString());
			OnJoinSessionResult.Broadcast(false);
		}
		return;
	}

	OnDestroySessionResult.Broadcast(bWasSuccessful, bWasHost);
	if (bWasSuccessful)
	{
		TravelToMainMenu();
	}
	// 실패 시 바인드된 UI에서 처리
}

void UMultiplayerSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 같은 프로세스의 다른 PIE 월드에서 발생한 실패는 무시
	if (!World || World->GetGameInstance() != GetGameInstance())
		return;

	// 엔진 네트워크 실패 발생 시 실행
	UE_LOG(LogTemp, Error, TEXT("Network failure %d: %s"), static_cast<int32>(FailureType), *ErrorString);

	// 정상 흐름에서는 호스트가 Listen Server를 종료하면
	// 원격 클라이언트에는 Destroy Complete가 오지 않고 ConnectionLost 같은 Network Failure가 전달됨
	// 비정상 종료에서는 Destroy Complete가 오지 않으므로 남아 있는 로컬 Named Session을 직접 정리
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SessionInterface->RemoveNamedSession(NAME_GameSession);
	}

	// 기존 위젯은 레벨 이동으로 제거되므로 새 메인 메뉴 위젯에서도 읽을 수 있도록 안내를 보관
	PendingConnectionFailureMessage = TEXT("The host connection was lost. Returned to the main menu.");
	OnConnectionFailure.Broadcast(PendingConnectionFailureMessage);

	TravelToMainMenu();
	// 네트워크 실패 시 연결이 끊긴 클라이언트는 엔진 기본 처리에 의해 GameDefaultMap으로 설정된 맵으로 이동
}

void UMultiplayerSessionSubsystem::TravelToMainMenu()
{
	/* Lv_MainMenu로 이동 */
	// 정상흐름에서
	// - 호스트: 세션 광고 제거가 완료된 뒤
	// - 클라이언트: 로컬 세션 정리가 완료된 뒤 호출
	// * 세션 광고(Session Advertisement): 호스트가 만든 방 정보를 다른 플레이어의 세션 검색에 노출하는 것
	static const FName MainMenuMapName(TEXT("/Game/Maps/Lv_MainMenu"));
	UGameplayStatics::OpenLevel(this, MainMenuMapName);
}

void UMultiplayerSessionSubsystem::ClearOnlineCompleteDelegateHandles()
{
	// 델리게이트를 소유 중인 IOnlineSession이 유효한지 검사
	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (CreateSessionDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		CreateSessionDelegateHandle.Reset();
	}
	if (FindSessionsDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		FindSessionsDelegateHandle.Reset();
	}
	if (JoinSessionDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		JoinSessionDelegateHandle.Reset();
	}
	if (DestroySessionDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		DestroySessionDelegateHandle.Reset();
	}
}
