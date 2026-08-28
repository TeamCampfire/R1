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

	UFUNCTION()
	void UpdateStatusEffects();

	void InitializeSurvivalStatBars();
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

};
