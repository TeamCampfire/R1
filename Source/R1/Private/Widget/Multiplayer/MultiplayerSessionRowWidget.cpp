#include "Widget/Multiplayer/MultiplayerSessionRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Widget/Multiplayer/MultiplayerMenuWidget.h"

void UMultiplayerSessionRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 행 위젯의 버튼 클릭 이벤트 바인딩
	RowButton->OnClicked.AddUniqueDynamic(this, &UMultiplayerSessionRowWidget::HandleRowClicked);
}

void UMultiplayerSessionRowWidget::InitializeRow(UMultiplayerMenuWidget* InOwnerWidget, const FSessionListItem& InSession)
{
	// 행: 표시 데이터만 보관
	// 실제 참가 상태: 부모 메뉴가 관리
	OwnerWidget = InOwnerWidget;
	SessionIndex = InSession.bIsCurrentSession ? INDEX_NONE : InSession.SearchResultIndex;
	ServerNameText->SetText(FText::FromString(InSession.ServerName));
	PlayersText->SetText(FText::FromString(FString::Printf(
		TEXT("%d / %d"), InSession.CurrentPlayers, InSession.MaxPlayers)));
	PingText->SetText(InSession.bIsCurrentSession
		? FText::FromString(TEXT("CURRENT"))
		: FText::FromString(FString::Printf(TEXT("%d ms"), InSession.Ping)));
}

void UMultiplayerSessionRowWidget::HandleRowClicked()
{
	// 클릭된 세션 검색 결과 인덱스를 부모 메뉴에 전달해 Join 버튼 활성화
	if (OwnerWidget && SessionIndex != INDEX_NONE)
	{
		OwnerWidget->SelectSession(SessionIndex);
	}
}
