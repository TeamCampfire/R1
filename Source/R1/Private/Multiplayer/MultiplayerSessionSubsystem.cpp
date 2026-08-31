#include "Multiplayer/MultiplayerSessionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

// 검색 결과에 포함시킬 사용자 정의 방 이름의 Key입니다.
// 호스트와 검색 측이 반드시 같은 Key를 사용해야 값을 다시 읽을 수 있습니다.
const FName UMultiplayerSessionSubsystem::ServerNameSettingKey(TEXT("SERVER_NAME"));

UMultiplayerSessionSubsystem::UMultiplayerSessionSubsystem()
{
	// Online Session 요청은 결과를 즉시 반환하지 않음
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
	// 세션 생성/검색/참가/제거는 모두 이 인터페이스를 통해 수행하며
	// Online Services의 ISessions는 사용하지 않음
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
	ClearOnlineDelegateHandles();

	if (GEngine && NetworkFailureDelegateHandle.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureDelegateHandle);
		NetworkFailureDelegateHandle.Reset();
	}

	SessionSearch.Reset();
	SessionInterface.Reset();
	Super::Deinitialize();
}

void UMultiplayerSessionSubsystem::CreateSession(int32 NumPublicConnections, const FString& ServerName)
{
	// Interface/입력값/진행 상태를 먼저 검사해 잘못된 요청과 중복 클릭을 차단합니다.
	if (!SessionInterface.IsValid() || NumPublicConnections <= 0 || ServerName.IsEmpty() ||
		bCreateInProgress || bDestroyInProgress)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession failed: invalid state or arguments"));
		OnCreateSessionResult.Broadcast(false);
		return;
	}

	// OSS는 같은 Local Session Name(NAME_GameSession)을 중복 생성할 수 없습니다.
	// 기존 세션이 있으면 입력값을 보관하고 Destroy 완료 후 다시 생성합니다.
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bCreateSessionAfterDestroy = true;
		PendingNumPublicConnections = NumPublicConnections;
		PendingServerName = ServerName;
		DestroySession();
		return;
	}

	// 호스트가 LAN에 광고할 세션의 규칙입니다.
	FOnlineSessionSettings Settings;
	// Null 기반 LAN 검색 패킷으로만 노출합니다.
	Settings.bIsLANMatch = true;
	// 호스트를 포함해 세션에 들어올 수 있는 Public Connection의 총 개수입니다.
	Settings.NumPublicConnections = NumPublicConnections;
	// 다른 컴퓨터의 FindSessions 결과에 이 방이 나타나도록 광고합니다.
	Settings.bShouldAdvertise = true;
	// 게임 맵으로 이동한 뒤에도 검색 및 참가할 수 있게 합니다.
	Settings.bAllowJoinInProgress = true;
	// Presence/Invite는 LAN Null 목표에 필요하지 않으므로 사용하지 않습니다.
	Settings.bAllowJoinViaPresence = false;
	Settings.bAllowInvites = false;
	Settings.Set(
		ServerNameSettingKey,
		ServerName,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// Complete Delegate를 먼저 등록한 후 요청합니다. 반환된 Handle은 정확히 이 바인딩만
	// 나중에 해제하기 위해 보관합니다.
	CreateSessionDelegateHandle =
		SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	bCreateInProgress = true;

	// false는 비동기 작업 자체가 시작되지 않았다는 의미입니다.
	// 이 경우 Complete Callback이 오지 않으므로 여기서 직접 Handle과 실패 UI를 처리합니다.
	if (!SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		CreateSessionDelegateHandle.Reset();
		bCreateInProgress = false;
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

	// 검색은 비동기이므로 지역 변수가 아니라 멤버 TSharedPtr로 수명을 유지합니다.
	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = MaxSearchResults;
	SessionSearch->bIsLanQuery = true;

	FindSessionsDelegateHandle =
		SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
	bFindInProgress = true;

	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		FindSessionsDelegateHandle.Reset();
		bFindInProgress = false;
		OnFindSessionsResult.Broadcast(false, TArray<FSessionListItem>());
	}
}

void UMultiplayerSessionSubsystem::JoinSession(int32 SessionIndex)
{
	// UI가 받은 SearchResultIndex가 현재 검색 배열에서 여전히 유효한지 확인합니다.
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid() || bJoinInProgress ||
		!SessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession failed (session index: %d)"), SessionIndex);
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	JoinSessionDelegateHandle =
		SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	bJoinInProgress = true;

	if (!SessionInterface->JoinSession(
		0, NAME_GameSession, SessionSearch->SearchResults[SessionIndex]))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		JoinSessionDelegateHandle.Reset();
		bJoinInProgress = false;
		OnJoinSessionResult.Broadcast(false);
	}
}

void UMultiplayerSessionSubsystem::DestroySession()
{
	// Classic IOnlineSession에는 플랫폼 공통 LeaveSession 함수가 없습니다.
	// 호스트는 광고 세션을 종료하고, 클라이언트는 자기 로컬 Named Session을 정리할 때
	// 모두 DestroySession을 사용합니다. 아래 bWasHost가 UI에 두 상황을 구분해 줍니다.
	if (!SessionInterface.IsValid() || bDestroyInProgress)
	{
		const bool bWasHost = GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer;
		if (bCreateSessionAfterDestroy)
		{
			bCreateSessionAfterDestroy = false;
			OnCreateSessionResult.Broadcast(false);
		}
		else
		{
			OnDestroySessionResult.Broadcast(false, bWasHost);
		}
		return;
	}

	const bool bWasHost = GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer;
	bDestroyingHostedSession = bWasHost;

	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		if (bCreateSessionAfterDestroy)
		{
			const int32 NumPublicConnections = PendingNumPublicConnections;
			const FString ServerName = PendingServerName;
			bCreateSessionAfterDestroy = false;
			CreateSession(NumPublicConnections, ServerName);
		}
		else
		{
			OnDestroySessionResult.Broadcast(true, bWasHost);
			TravelToMainMenu();
		}
		return;
	}

	DestroySessionDelegateHandle =
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
	bDestroyInProgress = true;

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		DestroySessionDelegateHandle.Reset();
		bDestroyInProgress = false;

		if (bCreateSessionAfterDestroy)
		{
			bCreateSessionAfterDestroy = false;
			OnCreateSessionResult.Broadcast(false);
		}
		else
		{
			OnDestroySessionResult.Broadcast(false, bWasHost);
		}
	}
}

void UMultiplayerSessionSubsystem::LeaveSession()
{
	// Listen Server가 실수로 클라이언트 퇴장 경로를 호출해 방 전체를 닫지 않게 막습니다.
	if (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("LeaveSession is for clients. Use EndHostedSession on a listen server."));
		OnDestroySessionResult.Broadcast(false, true);
		return;
	}

	DestroySession();
}

void UMultiplayerSessionSubsystem::EndHostedSession()
{
	// 호스트 전용 경로입니다. Destroy 완료 후 호스트는 Lv_MainMenu로 이동하며,
	// 연결된 클라이언트는 Network Failure/Connection Lost 처리를 받게 됩니다.
	if (!GetWorld() || GetWorld()->GetNetMode() != NM_ListenServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("EndHostedSession requires a listen server."));
		OnDestroySessionResult.Broadcast(false, false);
		return;
	}

	DestroySession();
}

void UMultiplayerSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// Complete Callback은 성공/실패와 관계없이 한 번 왔으므로 Handle부터 해제합니다.
	if (SessionInterface.IsValid() && CreateSessionDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		CreateSessionDelegateHandle.Reset();
	}
	bCreateInProgress = false;

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession failed for %s"), *SessionName.ToString());
		OnCreateSessionResult.Broadcast(false);
		return;
	}

	// Engine/World.h가 필요한 이유:
	// GetWorld()가 반환하는 UWorld의 ServerTravel과 GetNetMode를 호출하려면
	// UWorld의 완전한 클래스 정의가 .cpp에 포함되어 있어야 합니다.
	UWorld* World = GetWorld();
	if (!World || !World->ServerTravel(TEXT("/Game/Maps/Lv01_PlayerMoveTest?listen")))
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession succeeded, but ServerTravel failed"));
		OnCreateSessionResult.Broadcast(false);
		DestroySession();
		return;
	}

	OnCreateSessionResult.Broadcast(true);
}

void UMultiplayerSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface.IsValid() && FindSessionsDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		FindSessionsDelegateHandle.Reset();
	}
	bFindInProgress = false;

	TArray<FSessionListItem> Items;
	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions failed"));
		OnFindSessionsResult.Broadcast(false, Items);
		return;
	}

	// 검색 성공과 검색 결과 0개는 서로 다릅니다.
	// 성공했지만 방이 없으면 bSuccess=true와 빈 배열을 UI에 전달합니다.
	Items.Reserve(SessionSearch->SearchResults.Num());
	for (int32 Index = 0; Index < SessionSearch->SearchResults.Num(); ++Index)
	{
		const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[Index];
		FSessionListItem& Item = Items.AddDefaulted_GetRef();
		Item.SearchResultIndex = Index;
		// 열린 슬롯 수를 최대 슬롯에서 빼면 현재 참가 인원을 계산할 수 있습니다.
		Item.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Item.CurrentPlayers = Item.MaxPlayers - Result.Session.NumOpenPublicConnections;
		Item.Ping = Result.PingInMs;

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
	if (SessionInterface.IsValid() && JoinSessionDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		JoinSessionDelegateHandle.Reset();
	}
	bJoinInProgress = false;

	if (!SessionInterface.IsValid() || Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession failed with result %d"), static_cast<int32>(Result));
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	// JoinSession은 논리적인 세션 참가만 완료합니다. 실제 IP:Port 접속 주소는
	// OSS 구현체가 제공하므로 직접 조립하지 않고 GetResolvedConnectString을 사용합니다.
	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("GetResolvedConnectString failed"));
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	APlayerController* PlayerController =
		GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("No local PlayerController for ClientTravel"));
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	// ClientTravel이 로컬 클라이언트를 Listen Server의 게임 월드로 접속시킵니다.
	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
	OnJoinSessionResult.Broadcast(true);
}

void UMultiplayerSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid() && DestroySessionDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		DestroySessionDelegateHandle.Reset();
	}
	bDestroyInProgress = false;

	const bool bWasHost = bDestroyingHostedSession;
	bDestroyingHostedSession = false;

	// CreateSession 도중 발견한 기존 세션을 교체하기 위한 Destroy라면
	// 메뉴로 가지 않고 저장해 둔 설정으로 새 세션을 만듭니다.
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

	OnDestroySessionResult.Broadcast(bWasSuccessful, bWasHost);
	if (bWasSuccessful)
	{
		TravelToMainMenu();
	}
}

void UMultiplayerSessionSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	UE_LOG(LogTemp, Error, TEXT("Network failure %d: %s"), static_cast<int32>(FailureType), *ErrorString);
	// 호스트가 Listen Server를 종료하면 원격 클라이언트에는 Destroy Complete가 오지 않고
	// ConnectionLost 같은 Network Failure가 전달됩니다. UI에 원인을 먼저 알린 뒤 메뉴로 복귀합니다.
	OnConnectionFailure.Broadcast(ErrorString);
	TravelToMainMenu();
}

void UMultiplayerSessionSubsystem::TravelToMainMenu()
{
	/* Lv_MainMenu로 이동 */
	// - 호스트: 세션 광고 제거가 완료된 뒤
	// - 클라이언트: 로컬 세션 정리가 완료된 뒤 호출
	// * 세션 광고(Session Advertisement): 호스트가 만든 방 정보를 다른 플레이어의 세션 검색에 노출하는 것
	static const FName MainMenuMapName(TEXT("/Game/Maps/Lv_MainMenu"));
	UGameplayStatics::OpenLevel(this, MainMenuMapName);
}

void UMultiplayerSessionSubsystem::ClearOnlineDelegateHandles()
{
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
