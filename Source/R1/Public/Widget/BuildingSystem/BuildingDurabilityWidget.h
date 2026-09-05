// 작업 시작일 : 9/5
// 작업자 : 우진

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuildingDurabilityWidget.generated.h"

/**
 * 
 */
UCLASS()
class R1_API UBuildingDurabilityWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 게이지와 숫자 텍스트를 갱신
	UFUNCTION(BlueprintCallable, Category = "Building|Durability")
	void UpdateDurability(float CurrentDurability, float MaxDurability);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building|Durability")
	TObjectPtr<class UProgressBar> ProgressBar_BuildingDurability;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building|Durability")
	TObjectPtr<class UTextBlock> Text_BuildingDurability;
};
