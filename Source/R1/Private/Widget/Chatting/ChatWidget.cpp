/// 최초작성 : 2026.09.08
/// 작 성 자 : 우 진

#include "Widget/Chatting/ChatWidget.h"
#include "Widget/Chatting/ChatMessageWidget.h"

#include "Components/WidgetSwitcher.h"
#include "Components/EditableText.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Character/ActionPlayerController.h"
#include "GameFramework/PlayerState.h"

void UChatWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	CloseChat(); // 기본 채팅 상태

	// EditableText에서 입력 포커싱이 해제되었을 때 HandleMessageCommitted 이거 호출하도록 바인딩했어요
	if (EditableText_MessageInput)
		EditableText_MessageInput->OnTextCommitted.AddDynamic(this, &UChatWidget::HandleMessageCommitted);
}

FReply UChatWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 채팅 입력 상태에서 좌클릭한 경우에만!
	if (true == bChatOpen && EKeys::LeftMouseButton == InMouseEvent.GetEffectingButton())
	{
		// 컨트롤러의 함수를 호출해야 해요
		if (AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetOwningPlayer()))
			PlayerController->OnCloseChat();

		return FReply::Handled();
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UChatWidget::CloseChat()
{
	if (nullptr == WidgetSwitcher_ChatMode) return;

	// 위젯스위처의 현재 활성화 위젯을 기본 채팅창으로 변경
	WidgetSwitcher_ChatMode->SetActiveWidgetIndex(0); // 0이 기본채팅창

	bChatOpen = false;

	if (Border_ChatClickCatcher)
		Border_ChatClickCatcher->SetVisibility(ESlateVisibility::Collapsed);

	// 입력칸에 채워진 게 있었으면 비워요
	if (EditableText_MessageInput)
		EditableText_MessageInput->SetText(FText(FText::GetEmpty()));	
}

void UChatWidget::OpenChat()
{
	if (nullptr == WidgetSwitcher_ChatMode || nullptr == EditableText_MessageInput)
	{
		UE_LOG(LogTemp, Log, TEXT("[UChatWidget::OpenChat()] : WBP_Chat 위젯 바인딩을 확인하세요."));
		return;
	}

	// 위젯스위처의 현재 활성화 위젯을 입력 채팅창으로 변경
	WidgetSwitcher_ChatMode->SetActiveWidgetIndex(1); // 1이 입력채팅창

	bChatOpen = true;

	// 채팅창이 열릴 때만 좌클릭 캐치 보더를 보이게끔 해요
	Border_ChatClickCatcher->SetVisibility(ESlateVisibility::Visible);

	// 채팅창 열리자마자 플레이어가 글자를 입력할 수 있도록 키보드 포커스 줍니다
	EditableText_MessageInput->SetKeyboardFocus();
}

void UChatWidget::AddChatMessageToUI(const FString& SenderName, const FString& Message)
{
	if (nullptr == ChatMessageWidgetClass ||
		nullptr == VerticalBox_CompactMessages || nullptr == VerticalBox_ExpandedMessages)
	{
		UE_LOG(LogTemp, Log, TEXT("[UChatWidget::AddChatMessageToUI] : WBP_ChatMessage, VerticalBox Messages들 중 하나가 바인딩 안 된 듯."));
		return;
	}

	// 기본 채팅창용, 입력 채팅창용 메시지 위젯을 개별로 만드는 이유는
	// UMG 위젯은 부모를 하나만 가질 수 있기 때문..........
	if (UChatMessageWidget* CompactMessageWidget = CreateWidget<UChatMessageWidget>(GetOwningPlayer(), ChatMessageWidgetClass))
	{
		CompactMessageWidget->SetMessage(SenderName, Message);

		if (UVerticalBoxSlot* CompactSlot = VerticalBox_CompactMessages->AddChildToVerticalBox(CompactMessageWidget))
			CompactSlot->SetHorizontalAlignment(HAlign_Fill);

		// 자동 제거 타이머도 돌려요
		CompactMessageWidget->StartAutoRemoveTimer(CompactMessageDisplayDuration);
	}

	if (UChatMessageWidget* ExpandedMessageWidget = CreateWidget<UChatMessageWidget>(GetOwningPlayer(), ChatMessageWidgetClass))
	{
		ExpandedMessageWidget->SetMessage(SenderName, Message);

		if (UVerticalBoxSlot* ExpandedSlot = VerticalBox_ExpandedMessages->AddChildToVerticalBox(ExpandedMessageWidget))
			ExpandedSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	// UI에 보여줄 Message 개수 제한
	// 오래된 메시지부터 제거
	while (MaxCompactMessageCount < VerticalBox_CompactMessages->GetChildrenCount())
		VerticalBox_CompactMessages->RemoveChildAt(0);

	while ((MaxExpandedMessageCount + 1) < VerticalBox_ExpandedMessages->GetChildrenCount())
	{
		// 0은 위쪽 공간을 차지하고 있는 스페이서
		// 스페이서를 지울 순 없지..
		VerticalBox_ExpandedMessages->RemoveChildAt(1);
	}

	// 새로운 메시지가 추가되면 스크롤을 가장 최신 메시지가 있는 아래쪽으로 내림
	if (nullptr != ScrollBox_ExpandedMessages)
		ScrollBox_ExpandedMessages->ScrollToEnd();
}

void UChatWidget::HandleMessageCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	// Enter로 포커싱 아웃을 했을 때만 메시지 입력이 적용되도록 해요
	if (ETextCommit::OnEnter != CommitMethod) return;

	FString msg = Text.ToString().TrimStartAndEnd(); // 불필요한 공백을 제거해준대요

	AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetOwningPlayer());
	if (nullptr == PlayerController) return;

	if (false == msg.IsEmpty()) // 메시지가 없으면 채팅 추가 안 해요
	{
		FString SenderName = TEXT("Default(세팅 안 된 듯)");

		if (APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>())
		{
			FString PlayerName = PlayerState->GetPlayerName().TrimStartAndEnd();
			if (false == PlayerName.IsEmpty())
				SenderName = PlayerName; // 플레이어 이름

			AddChatMessageToUI(SenderName, msg);
		}
	}

	// 뭐가 됐든 채팅창은 닫아야 해요
	PlayerController->OnCloseChat();
}
