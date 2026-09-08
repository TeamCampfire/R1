/// 최초작성 : 2026.09.08
/// 작 성 자 : 우 진

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatMessageWidget.generated.h"

/**
 * 
 */
UCLASS()
class R1_API UChatMessageWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeDestruct() override;

public:
	// 채팅 한 줄에 표시할 플레이어 이름과 메시지를 세팅하는 함수
	UFUNCTION(BlueprintCallable)
	void SetMessage(const FString& SenderName, const FString& Message);

	// 일정 시간이 지나면 부모 메시지 목록 위젯에서 본인을 제거 (기본 채팅에서만)
	void StartAutoRemoveTimer(float DisplayDuration);

private:
	void RemoveFromChatList();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> TextBlock_SenderName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> TextBlock_Message;

private:
	FTimerHandle AutoRemoveTimerHandle; // 일정 시간이 지나면 채팅을 자동 제거하는 타이머
};
