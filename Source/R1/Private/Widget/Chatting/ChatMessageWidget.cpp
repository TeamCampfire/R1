/// 최초작성 : 2026.09.08
/// 작 성 자 : 우 진

#include "Widget/Chatting/ChatMessageWidget.h"
#include "TimerManager.h"
#include "Components/TextBlock.h"

void UChatMessageWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld()) // 타이머 정리
		World->GetTimerManager().ClearTimer(AutoRemoveTimerHandle);

	Super::NativeDestruct();
}

void UChatMessageWidget::SetMessage(const FString& SenderName, const FString& Message, const FString& Timestamp)
{
	if (nullptr == TextBlock_SenderName || nullptr == TextBlock_Message || nullptr == TextBlock_Timestamp)
		return;

	TextBlock_SenderName->SetText(FText::FromString(SenderName));
	TextBlock_Message->SetText(FText::FromString(Message));
	TextBlock_Timestamp->SetText(FText::FromString(Timestamp));
}

void UChatMessageWidget::StartAutoRemoveTimer(float DisplayDuration)
{
	if (DisplayDuration <= 0.f) return;

	GetWorld()->GetTimerManager().SetTimer(
		AutoRemoveTimerHandle, this, &UChatMessageWidget::RemoveFromChatList, DisplayDuration, false);
}

void UChatMessageWidget::RemoveFromChatList()
{
	// 부모에게서 제거함
	// 부모가 누ㅜ구냐면 ChatWidget의 VerticalBox_CompactMessage
	RemoveFromParent();
}
