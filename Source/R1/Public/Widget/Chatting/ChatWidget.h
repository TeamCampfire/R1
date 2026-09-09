/// 최초작성 : 2026.09.08
/// 작 성 자 : 우 진

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatWidget.generated.h"

class UEditableText;
/**
 * 
 */
UCLASS()
class R1_API UChatWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeDestruct() override;
	virtual void NativePreConstruct() override;

public:
	// 기본 상태의 UI로 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void CloseChat();

	// 입력창이 뜨는 UI로 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void OpenChat();

	UFUNCTION(BlueprintCallable, Category = "Chat")
	bool IsChatOpen() { return bChatOpen; }

	// 채팅을 쳐서 전송했을 때 UI에 이미 쳐진 채팅으로 추가하는 함수
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void AddChatMessageToUI(const FString& SenderName, const FString& Message, const FString& Timestamp);

protected:
	// 채팅 입력이 완료되었을 때 호출 (Enter를 눌렀을 때)
	UFUNCTION()
	void HandleMessageCommitted(const FText& Text, ETextCommit::Type CommitMethod);

private:
	// ActionGameState를 찾아 채팅 추가될 때 브로드캐스트하는 델리게이트를 구독하는 함수
	void BindToActionGameState();

	// 채팅 델리게이트 발동되었을 때 실행할 함수
	void HandleGlobalChatMessageReceived(const struct FGlobalChatMessage& Chat);

	// 입력창 있는 UI의 메시지 개수를 현재 GameState가 보관중인 채팅 기록의 개수와 맞추는 함수
	void DestroyExpandedMessagesToGameStateHistory();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWidgetSwitcher> WidgetSwitcher_ChatMode; // 기본채팅창, 입력채팅창 위젯을 갈아끼울 수 있는 위젯스위처

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableText> EditableText_MessageInput; // 입력 가능 채팅 EditableText

	bool bChatOpen; // false 기본 채팅창 / true 입력 채팅창

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UVerticalBox> VerticalBox_CompactMessages; // 기본 채팅창의 최근 메시지 목록

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UVerticalBox> VerticalBox_ExpandedMessages; // 입력 채팅창의 저장된 메시지 목록

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UScrollBox> ScrollBox_ExpandedMessages; // 입력 채팅창의 스크롤

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<class UChatMessageWidget> ChatMessageWidgetClass; // 채팅 한줄짜리 위젯 클래스 -> 메시지 추가될 때 마다 만듦

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class USizeBox> SizeBox_ExpandedMessageViewport; // 입력 채팅 UI에서 채팅 기록 보이는 영역

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat")
	int32 MaxCompactMessageCount = 3; // 기본 채팅창에 동시에 표시할 최대 메시지 개수

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat")
	int32 ExpandedVisibleLineCount = 10; // 입력 채팅 UI에서 한 화면에 보여줄 채팅 개수

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat")
	float SingleChatLineHeight = 24.0f; // WBP_ChatMessage 위젯의 한 줄 높이

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat")
	float CompactMessageDisplayDuration = 8.0f; // 기본 채팅창에서 메시지가 유지되는 시간

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UBorder> Border_ChatClickCatcher; // 채팅 입력 상태에서 화면 전체의 좌클릭 감지하는 투명 Border

private:
	TWeakObjectPtr<class AActionGameState> ActionGameState; // 현재 연결된 GameState

	int32 LastShowGlobalChatMessageID = 0; // 이 UI가 마지막으로 화면에 표시한 채팅 ID

};
