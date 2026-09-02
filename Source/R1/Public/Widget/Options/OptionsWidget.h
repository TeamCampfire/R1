/// 최초작성 : 2026.09.02
/// 작 성 자 : 최 요 환
/// 간단설명 : ESC로 여는 옵션(환경설정) 패널 최상위 위젯 — 왼쪽 카테고리 버튼으로 가운데 콘텐츠를 전환한다.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OptionsWidget.generated.h"

class UButton;
class UWidgetSwitcher;

// ContentSwitcher의 자식 인덱스와 반드시 일치해야 한다 (0번 슬롯 = Controls, 1번 슬롯 = Display).
// 새 카테고리(예: 오디오)를 추가할 땐 이 enum에 값을 추가하고, WBP에서 ContentSwitcher에 같은
// 순서로 자식 위젯을 배치 + 새 카테고리 버튼을 NativeOnInitialized에서 바인딩하면 된다.
UENUM(BlueprintType)
enum class EOptionsCategory : uint8
{
	Controls,	// 단축키(키 리바인딩)
	Display,	// 화면(해상도/화면모드)
};

/**
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다:
 * - CategoryButton_Controls : "단축키" 카테고리 버튼.
 * - CategoryButton_Display  : "화면" 카테고리 버튼.
 * - ContentSwitcher         : 카테고리별 세부 패널을 담는 WidgetSwitcher.
 *   자식 0번 슬롯에 WBP_OptionsControls, 1번 슬롯에 WBP_OptionsDisplay를 배치한다(EOptionsCategory 순서와 일치).
 */
UCLASS()
class R1_API UOptionsWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	UFUNCTION()
	void HandleCategoryControlsClicked();

	UFUNCTION()
	void HandleCategoryDisplayClicked();

	void SwitchToCategory(EOptionsCategory Category);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CategoryButton_Controls;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CategoryButton_Display;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;
};
