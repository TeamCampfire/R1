/// 최초작성 : 2026.09.02
/// 작 성 자 : 최 요 환
/// 간단설명 : 단축키 리바인딩 중 이미 다른 액션에 쓰이고 있는 키를 고르면 뜨는 충돌 확인 모달.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KeyConflictDialogWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKeyConflictReplaceConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKeyConflictCancelled);

/**
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다:
 * - ConflictKeyText    : 충돌난 키 이름(예: "S").
 * - OldActionNameText  : 그 키를 원래 쓰고 있던 액션 이름(예: "뒤로").
 * - NewActionNameText  : 새로 그 키를 받으려는 액션 이름(예: "앞으로").
 * - ReplaceButton      : 확인 — 기존 액션에서 키를 빼앗아 새 액션에 넘긴다.
 * - CancelButton       : 취소 — 리바인딩을 포기하고 기존 키 그대로 둔다.
 *
 * 이 위젯 자체는 어떤 매핑을 바꿀지 모른다 — OptionsControlsWidget이 충돌을 감지하면
 * ShowConflict로 표시할 텍스트만 넘겨주고, 사용자가 Replace/Cancel 중 뭘 눌렀는지만
 * 델리게이트로 돌려받아서 실제 리바인딩(UnMapPlayerKey + MapPlayerKey)은 OptionsControlsWidget이 처리한다.
 */
UCLASS()
class R1_API UKeyConflictDialogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 충돌 안내를 채우고 다이얼로그를 보이게 한다.
	void ShowConflict(const FText& InConflictKeyName, const FText& InOldActionName, const FText& InNewActionName);

	// 다이얼로그를 닫는다(Replace/Cancel 처리 후 OptionsControlsWidget이 호출).
	void CloseDialog();

	UPROPERTY(BlueprintAssignable)
	FOnKeyConflictReplaceConfirmed OnReplaceConfirmed;

	UPROPERTY(BlueprintAssignable)
	FOnKeyConflictCancelled OnCancelled;

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	UFUNCTION()
	void HandleReplaceClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConflictKeyText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OldActionNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NewActionNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ReplaceButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelButton;
};
