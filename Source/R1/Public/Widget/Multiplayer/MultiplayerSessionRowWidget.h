#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Multiplayer/MultiplayerSessionSubsystem.h"
#include "MultiplayerSessionRowWidget.generated.h"

class UButton;
class UTextBlock;
class UMultiplayerMenuWidget;

/*
* 검색된 세션 하나를 표시하는 행
* 레이아웃은 WBP_MultiplayerSessionRow가 담당
*/
UCLASS(Blueprintable)
class R1_API UMultiplayerSessionRowWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	virtual void NativeOnInitialized() override;

public:

	// 행 데이터 초기화
	// - 세션 검색 결과 데이터를 바인딩된 위젯 텍스트에 반영
	// - 클릭(선택)을 전달할 부모 메뉴 기억
	void InitializeRow(UMultiplayerMenuWidget* InOwnerWidget, const FSessionListItem& InSession);

private:

	// 행 클릭(선택) 시 부모 메뉴에 세션 검색 결과 인덱스 전달
	UFUNCTION()
	void HandleRowClicked();

private :

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> RowButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> ServerNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> PlayersText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> PingText;

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerMenuWidget> OwnerWidget;

	// SessionSubsystem 검색 결과 배열에서 이 행이 나타내는 실제 위치
	int32 SessionIndex = INDEX_NONE;
};
