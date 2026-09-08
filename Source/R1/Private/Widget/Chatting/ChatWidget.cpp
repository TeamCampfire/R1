/// 최초작성 : 2026.09.08
/// 작 성 자 : 우 진

#include "Widget/Chatting/ChatWidget.h"

#include "Components/WidgetSwitcher.h"
#include "Components/EditableText.h"

void UChatWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	CloseChat(); // 기본 채팅 상태
}

void UChatWidget::CloseChat()
{
	if (nullptr == WidgetSwitcher_ChatMode) return;

	// 위젯스위처의 현재 활성화 위젯을 기본 채팅창으로 변경
	WidgetSwitcher_ChatMode->SetActiveWidgetIndex(0); // 0이 기본채팅창

	bChatOpen = false;

	// 입력칸에 채워진 게 있었으면 비워요
	if (EditableText_MessageInput)
		EditableText_MessageInput->SetText(FText(FText::GetEmpty()));	
}

void UChatWidget::OpenChat()
{
	if (nullptr == WidgetSwitcher_ChatMode || nullptr == EditableText_MessageInput)
	{
		UE_LOG(LogTemp, Log, TEXT("[Chat] : WBP_Chat 위젯 바인딩을 확인하세요."));
		return;
	}

	// 위젯스위처의 현재 활성화 위젯을 입력 채팅창으로 변경
	WidgetSwitcher_ChatMode->SetActiveWidgetIndex(1); // 1이 입력채팅창

	bChatOpen = true;

	// 채팅창 열리자마자 플레이어가 글자를 입력할 수 있도록 키보드 포커스 줍니다
	EditableText_MessageInput->SetKeyboardFocus();
}
