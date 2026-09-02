#include "Widget/Multiplayer/MultiplayerMenuWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Widget/Multiplayer/MultiplayerSessionRowWidget.h"

UMultiplayerMenuWidget::UMultiplayerMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 목록 행의 기본 WBP를 지정하되, 필요하면 자식 위젯 기본값에서 다른 클래스로 교체 가능
	SessionRowWidgetClass = TSoftClassPtr<UMultiplayerSessionRowWidget>(FSoftObjectPath(
		TEXT("/Game/Blueprint/Widget/Multiplayer/WBP_MultiplayerSessionRow.WBP_MultiplayerSessionRow_C")));
}

void UMultiplayerMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 위젯 버튼 이벤트 연결
	RefreshButton->OnClicked.AddUniqueDynamic(this, &UMultiplayerMenuWidget::HandleRefreshClicked);
	JoinButton->OnClicked.AddUniqueDynamic(this, &UMultiplayerMenuWidget::HandleJoinClicked);
	HostButton->OnClicked.AddUniqueDynamic(this, &UMultiplayerMenuWidget::HandleHostClicked);
	DecreaseMaxPlayersButton->OnClicked.AddUniqueDynamic(this, &UMultiplayerMenuWidget::HandleDecreaseMaxPlayersClicked);
	IncreaseMaxPlayersButton->OnClicked.AddUniqueDynamic(this, &UMultiplayerMenuWidget::HandleIncreaseMaxPlayersClicked);

	// 위젯 프롭들 초기 설정
	JoinButton->SetIsEnabled(false);
	UpdateMaxPlayersDisplay();

	// 위젯과 연결할 세션 기능을 가져오기 위해서 SessionSubsystem 저장
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		SessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionSubsystem>();
	}

	// 세션 기능은 SessionSubsystem에 구현된 기능을 사용하도록 델리게이트 바인딩
	if (SessionSubsystem)
	{
		SessionSubsystem->OnCreateSessionResult.AddUniqueDynamic(this, &UMultiplayerMenuWidget::HandleCreateResult);
		SessionSubsystem->OnFindSessionsResult.AddUniqueDynamic(this, &UMultiplayerMenuWidget::HandleFindResult);
		SessionSubsystem->OnJoinSessionResult.AddUniqueDynamic(this, &UMultiplayerMenuWidget::HandleJoinResult);
		SessionSubsystem->OnConnectionFailure.AddUniqueDynamic(this, &UMultiplayerMenuWidget::HandleConnectionFailure);
		HandleRefreshClicked();
	}
}

void UMultiplayerMenuWidget::NativeDestruct()
{
	// 파괴되는 위젯으로 콜백이 들어오지 않도록 SessionSubsystem의 기능 바인딩을 해제
	if (SessionSubsystem)
	{
		SessionSubsystem->OnCreateSessionResult.RemoveDynamic(this, &UMultiplayerMenuWidget::HandleCreateResult);
		SessionSubsystem->OnFindSessionsResult.RemoveDynamic(this, &UMultiplayerMenuWidget::HandleFindResult);
		SessionSubsystem->OnJoinSessionResult.RemoveDynamic(this, &UMultiplayerMenuWidget::HandleJoinResult);
		SessionSubsystem->OnConnectionFailure.RemoveDynamic(this, &UMultiplayerMenuWidget::HandleConnectionFailure);
	}

	Super::NativeDestruct();
}

void UMultiplayerMenuWidget::HandleRefreshClicked()
{
	if (SessionSubsystem && !bBusy)
	{
		SetBusy(true, FText::FromString(TEXT("Searching LAN servers...")));
		SessionSubsystem->FindSessions(100);
	}
}

void UMultiplayerMenuWidget::HandleJoinClicked()
{
	// 참가 가능한 상태인지 확인
	if (!SessionSubsystem || bBusy || !FoundSessions.IsValidIndex(SelectedSessionIndex))
		return;

	// 현재 선택한 세션 정보 참조
	const FSessionListItem& SelectedSession = FoundSessions[SelectedSessionIndex];

	// 남은 자리 수 확인 -> 가득 찼으면 StatusText 문구 수정 후 종료 (참가 시도 안 함)
	if (SelectedSession.CurrentPlayers >= SelectedSession.MaxPlayers)
	{
		StatusText->SetText(FText::FromString(TEXT("This server is full.")));
		return;
	}

	// 세션 참가 시도
	SetBusy(true, FText::FromString(TEXT("Joining server...")));
	SessionSubsystem->JoinSession(SelectedSessionIndex);

}

void UMultiplayerMenuWidget::HandleHostClicked()
{
	// 호스트 가능한 상태인지 검사
	if (!SessionSubsystem || bBusy || !ServerNameInput)
	{
		return;
	}

	const FString ServerName = ServerNameInput->GetText().ToString().TrimStartAndEnd();
	if (ServerName.IsEmpty())
	{
		StatusText->SetText(FText::FromString(TEXT("Enter a server name.")));
		return;
	}

	SetBusy(true, FText::FromString(TEXT("Creating server...")));

	// 설정한 최대 인원 수대로 공개 참여 슬롯 수를 제한한 세션 생성
	SessionSubsystem->CreateSession(MaxPlayers, ServerName);
}

void UMultiplayerMenuWidget::HandleDecreaseMaxPlayersClicked()
{
	if (!bBusy)
	{
		MaxPlayers = FMath::Clamp(MaxPlayers - 1, MinAllowedPlayers, MaxAllowedPlayers);
		UpdateMaxPlayersDisplay();
	}
}

void UMultiplayerMenuWidget::HandleIncreaseMaxPlayersClicked()
{
	if (!bBusy)
	{
		MaxPlayers = FMath::Clamp(MaxPlayers + 1, MinAllowedPlayers, MaxAllowedPlayers);
		UpdateMaxPlayersDisplay();
	}
}

void UMultiplayerMenuWidget::HandleCreateResult(bool bSuccess)
{
	SetBusy(false, FText::FromString(bSuccess ? TEXT("Server created. Travelling...") : TEXT("Could not create server.")));
}

void UMultiplayerMenuWidget::HandleFindResult(bool bSuccess, const TArray<FSessionListItem>& Sessions)
{
	FoundSessions = Sessions;
	SelectedSessionIndex = INDEX_NONE;
	BuildSessionRows();
	SetBusy(false, FText::FromString(!bSuccess ? TEXT("Server search failed.") : Sessions.IsEmpty() ? TEXT("No LAN servers found.") : *FString::Printf(TEXT("%d servers found."), Sessions.Num())));
}

void UMultiplayerMenuWidget::HandleJoinResult(bool bSuccess)
{
	SetBusy(false, FText::FromString(bSuccess ? TEXT("Connected. Travelling...") : TEXT("Could not join server.")));
}

void UMultiplayerMenuWidget::HandleConnectionFailure(const FString& ErrorMessage)
{
	SetBusy(false, FText::FromString(FString::Printf(TEXT("Connection lost: %s"), *ErrorMessage)));
}

void UMultiplayerMenuWidget::SelectSession(int32 SessionIndex)
{
	// 검색 결과의 실제 인덱스를 보존해 JoinSession 호출 시 같은 항목을 전달
	SelectedSessionIndex = SessionIndex;

	// 참: 유효한 세션 인덱스이고, 참가 자리가 남아있을 때
	const bool bCanJoin = FoundSessions.IsValidIndex(SessionIndex)
		&& FoundSessions[SessionIndex].CurrentPlayers < FoundSessions[SessionIndex].MaxPlayers;

	if (JoinButton)
	{
		// 비동기 요청 실행 중이 아니고, 참가 가능한 세션일 때만 참가 버튼 활성화
		JoinButton->SetIsEnabled(!bBusy && bCanJoin);
	}

	// StatusText 객체가 있고, 유효한 세션 인덱스일 때
	if (StatusText && FoundSessions.IsValidIndex(SessionIndex))
	{
		// 세션 참가 자리 여부에 따라 StatusText 변경
		StatusText->SetText(
			bCanJoin
			?
			FText::FromString(FString::Printf(TEXT("Selected: %s"), *FoundSessions[SessionIndex].ServerName))
			:
			FText::FromString(TEXT("This server is full."))
		);
	}
}

void UMultiplayerMenuWidget::BuildSessionRows()
{
	if (!SessionList)
	{
		return;
	}
	SessionList->ClearChildren();	// 세션 목록 초기화

	// 세션 행 위젯의 인스턴스 생성 및 데이터 적용
	UClass* RowClass = SessionRowWidgetClass.LoadSynchronous();
	if (!RowClass)
	{
		StatusText->SetText(FText::FromString(TEXT("Session row widget class is not configured.")));
		return;
	}

	for (const FSessionListItem& Item : FoundSessions)
	{
		UMultiplayerSessionRowWidget* RowWidget = CreateWidget<UMultiplayerSessionRowWidget>(GetOwningPlayer(), RowClass);
		if (RowWidget)
		{
			RowWidget->InitializeRow(this, Item);
			SessionList->AddChild(RowWidget);	// 세션 목록에 추가
		}
	}
}

void UMultiplayerMenuWidget::SetBusy(bool bInBusy, const FText& Message)
{
	// 비동기 세션 요청 중에는 중복 요청과 인원 변경을 한곳에서 차단
	bBusy = bInBusy;
	if (StatusText) StatusText->SetText(Message);
	if (RefreshButton) RefreshButton->SetIsEnabled(!bBusy);
	if (HostButton) HostButton->SetIsEnabled(!bBusy);
	if (JoinButton) JoinButton->SetIsEnabled(!bBusy && SelectedSessionIndex != INDEX_NONE);
	UpdateMaxPlayersDisplay();
}

void UMultiplayerMenuWidget::UpdateMaxPlayersDisplay()
{
	// 미리 설정한 인원 수대로 최대 참가 인원 수가 제한되도록 보정
	MaxPlayers = FMath::Clamp(MaxPlayers, MinAllowedPlayers, MaxAllowedPlayers);
	if (MaxPlayersText) MaxPlayersText->SetText(FText::AsNumber(MaxPlayers));
	if (DecreaseMaxPlayersButton) DecreaseMaxPlayersButton->SetIsEnabled(!bBusy && MaxPlayers > MinAllowedPlayers);
	if (IncreaseMaxPlayersButton) IncreaseMaxPlayersButton->SetIsEnabled(!bBusy && MaxPlayers < MaxAllowedPlayers);
}
