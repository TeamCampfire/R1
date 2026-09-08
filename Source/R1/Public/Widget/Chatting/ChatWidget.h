/// 최초작성 : 2026.09.08
/// 작 성 자 : 우 진

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatWidget.generated.h"

/**
 * 
 */
UCLASS()
class R1_API UChatWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

public:
	// 기본 상태의 UI로 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void CloseChat();

	// 입력창이 뜨는 UI로 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void OpenChat();

	UFUNCTION(BlueprintCallable, Category = "Chat")
	bool IsChatOpen() { return bChatOpen; }

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWidgetSwitcher> WidgetSwitcher_ChatMode; // 기본채팅창, 입력채팅창 위젯을 갈아끼울 수 있는 위젯스위처

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableText> EditableText_MessageInput; // 입력 가능 채팅 EditableText

	bool bChatOpen; // false 기본 채팅창 / true 입력 채팅창
};
