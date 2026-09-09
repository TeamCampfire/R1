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
#include "Components/SizeBox.h"
#include "Character/ActionPlayerController.h"
#include "Framework/GameState/ActionGameState.h"

void UChatWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	CloseChat(); // 기본 채팅 상태

	// EditableText에서 입력 포커싱이 해제되었을 때 HandleMessageCommitted 이거 호출하도록 바인딩했어요
	if (EditableText_MessageInput)
		EditableText_MessageInput->OnTextCommitted.AddDynamic(this, &UChatWidget::HandleMessageCommitted);

	BindToActionGameState(); // GameState 채팅 관련 델리게이트 구독
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

void UChatWidget::NativeDestruct()
{
	if (true == ActionGameState.IsValid())
	{
		ActionGameState->OnGlobalChatMessageReceive.RemoveAll(this);
		ActionGameState.Reset();
	}

	Super::NativeDestruct();
}

void UChatWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (nullptr == SizeBox_ExpandedMessageViewport) return;

	// 채팅이 들어가는 공간의 높이의 최대를 세팅
	SizeBox_ExpandedMessageViewport->SetHeightOverride(SingleChatLineHeight * ExpandedVisibleLineCount);
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
		PlayerController->SubmitGlobalChatMessage(msg); // 위젯은 플레이어 컨트롤러를 통해 플레이어 이름을 찾거나 메시지를 목록에 추가함

	// 뭐가 됐든 채팅창은 닫아야 해요
	PlayerController->OnCloseChat();
}

void UChatWidget::BindToActionGameState()
{
	AActionGameState* pActionGameState = GetWorld()->GetGameState<AActionGameState>();

	if (nullptr == pActionGameState)
	{
		UE_LOG(LogTemp, Warning, TEXT(
				"[Chat] AActionGameState를 찾지 못했습니다. "
				"GameMode의 Game State Class를 확인해 주세요."));
		return;
	}

	ActionGameState = pActionGameState;

	// 새로운 채팅이 추가될 떄 호출하는 델리게이트 구독
	ActionGameState->OnGlobalChatMessageReceive.AddUObject(this, &UChatWidget::HandleGlobalChatMessageReceived);

	// GameState의 채팅 복제가 위젯 생성보다 먼저 되었을 경우를 대비해
	// 그런 경우는 델리게이트를 놓쳤을 수도 있으니 현재 보관된 기록을 한 번 확인
	for (const FGlobalChatMessage& Chat : ActionGameState->GetGlobalChatHistroy())
	{
		HandleGlobalChatMessageReceived(Chat);
	}
}

void UChatWidget::HandleGlobalChatMessageReceived(const FGlobalChatMessage& Chat)
{
	// UI가 이미 화면에 보여준 채팅이면.. 리턴
	if (LastShowGlobalChatMessageID >= Chat.MessageID) return;

	// 채팅을 UI에 보여주고 마지막으로 보여준 Chat ID를 저장해요
	AddChatMessageToUI(Chat.SenderName, Chat.Message);
	LastShowGlobalChatMessageID = Chat.MessageID;

	// 채팅 추가 후
	// 현재 GameState의 채팅 기록 개수와 입력창 있는 채팅 UI에서의 채팅 개수를 맞춰줘요
	DestroyExpandedMessagesToGameStateHistory();
}

void UChatWidget::DestroyExpandedMessagesToGameStateHistory()
{
	if (false == ActionGameState.IsValid() || !VerticalBox_ExpandedMessages) return;

	// GameState가 가진 모든 채팅 개수
	const int32 AllChatCount = ActionGameState->GetGlobalChatHistroy().Num();

	while (VerticalBox_ExpandedMessages->GetChildrenCount() > AllChatCount + 1) // 1은 VerticalBox에 든 아래 정렬을 위한 스페이서 개수
	{
		VerticalBox_ExpandedMessages->RemoveChildAt(1); // 0은 스페이서, 1부터 삭제해야 함
	}
}
