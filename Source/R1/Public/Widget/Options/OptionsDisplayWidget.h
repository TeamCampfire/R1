/// 최초작성 : 2026.09.02
/// 작 성 자 : 최 요 환
/// 간단설명 : 옵션 패널의 "화면" 카테고리 — 해상도/화면모드를 UGameUserSettings로 조회·적용한다.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OptionsDisplayWidget.generated.h"

class UComboBoxString;
class UButton;

/**
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다:
 * - ResolutionComboBox : 지원되는 전체화면 해상도 목록("1920 x 1080" 형식 문자열).
 * - WindowModeComboBox : "전체화면"/"테두리없는 창모드"/"창모드".
 * - ApplyButton        : 위 두 콤보의 현재 선택값을 실제로 적용 + 저장.
 *
 * 해상도는 즉시 반영하면 화면이 깨질 수 있어, 콤보 선택만으로는 적용하지 않고
 * ApplyButton을 눌러야 SetScreenResolution/SetFullscreenMode + SaveSettings가 실행된다.
 */
UCLASS()
class R1_API UOptionsDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 콤보박스를 현재 UGameUserSettings 값으로 채우고 현재 값을 선택 상태로 맞춘다.
	// public인 이유: 옵션 패널을 열 때마다(OptionsWidget::RefreshActiveCategory) 다시 호출해서
	// 다른 경로(예: 게임 중 해상도 변경)로 설정이 바뀌었어도 최신 값을 보여주기 위함.
	void RefreshFromCurrentSettings();

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	UFUNCTION()
	void HandleApplyClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ResolutionComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> WindowModeComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ApplyButton;
};
