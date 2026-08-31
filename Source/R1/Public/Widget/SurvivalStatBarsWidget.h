/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "SurvivalStatBarsWidget.generated.h"

class UParameterBarWidget;
class UParameterBarWidgetTest;
class UStatusBarWidget;
class UVerticalBox;
class UStatComponent;
enum class EStatusEffect:uint8;

/**
 * 
 */
UCLASS()
class R1_API USurvivalStatBarsWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 상태이상 업데이트 함수
	UFUNCTION()
	void UpdateStatusEffects();

	// 스탯 UI 초기화 함수
	UFUNCTION()
	void InitializeSurvivalStatBars();

	// 스탯컴포넌트<->UI 델리게이트 구독 해제 함수
	void UnbindStatDelegates();
private:

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UStatusBarWidget> StatusBarWidgetClass;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UParameterBarWidget> HealthBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UParameterBarWidget> HydrationBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UParameterBarWidget> CaloriesBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> Debuffs;

	UPROPERTY()
	TMap<EStatusEffect, TObjectPtr<UStatusBarWidget>> StatusBarWidgets;

	UPROPERTY()
	TObjectPtr<UStatComponent> StatComp;

};
