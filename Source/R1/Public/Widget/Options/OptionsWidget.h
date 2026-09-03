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
class UBorder;

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
 * - CategoryButton_Controls    : "단축키" 카테고리 버튼.
 * - CategoryButton_Display     : "화면" 카테고리 버튼.
 * - CategoryHighlight_Controls : CategoryButton_Controls를 감싸는 배경 Border(선택 표시용, 선택 안 됐을 때는 투명).
 * - CategoryHighlight_Display  : CategoryButton_Display를 감싸는 배경 Border(선택 표시용).
 * - ContentSwitcher            : 카테고리별 세부 패널을 담는 WidgetSwitcher.
 *   자식 0번 슬롯에 WBP_OptionsControls, 1번 슬롯에 WBP_OptionsDisplay를 배치한다(EOptionsCategory 순서와 일치).
 *
 * 카테고리 버튼 자체는 Normal 브러시를 투명하게 비워두고 Hovered 브러시만 반투명하게 칠해서
 * 호버 피드백을 낸다(Designer에서 설정) — "선택된" 상태는 호버와 별개로 유지되어야 해서
 * CategoryHighlight_* Border의 배경색을 SwitchToCategory에서 직접 갈아끼운다.
 */
UCLASS()
class R1_API UOptionsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 현재 ContentSwitcher에 떠 있는 두 카테고리 패널(OptionsControls/OptionsDisplay) 모두
	// 최신 데이터로 다시 채운다. MainHUDWidget이 옵션 패널을 열 때마다(ESC) 호출해준다 —
	// 이 위젯 자체는 게임 시작 시 딱 한 번만 만들어지고 이후엔 Visibility만 토글되므로,
	// 최초 생성 시점에 Enhanced Input 등록이 아직 안 끝나 있었거나 그 사이 다른 경로로
	// 설정이 바뀌었어도 다시 열 때마다 최신 상태로 보이게 하기 위함이다.
	void RefreshActiveCategory();

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
	TObjectPtr<UBorder> CategoryHighlight_Controls;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CategoryHighlight_Display;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;
};
