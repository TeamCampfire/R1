/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CaloriesInterface.generated.h"

UINTERFACE(MinimalAPI)
class UCaloriesInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API ICaloriesInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Calories")
	float GetCurrentCalories() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Calories")
	float GetMaxCalories() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Calories")
	bool DecreaseCalories(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Calories")
	void RecoverCalories(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Calories")
	float GetCaloriesDropRate() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Calories")
	void SetCaloriesDropRate(float InDropRate);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Calories")
	void ResetCaloriesDropRate();
};
