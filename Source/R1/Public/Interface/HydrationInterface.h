/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HydrationInterface.generated.h"

UINTERFACE(MinimalAPI)
class UHydrationInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IHydrationInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hydration")
	float GetCurrentHydration() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hydration")
	float GetMaxHydration() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hydration")
	bool DecreaseHydration(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hydration")
	void RecoverHydration(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hydration")
	float GetHydrationDropRate() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hydration")
	void SetHydrationDropRate(float InDropRate);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hydration")
	void ResetHydrationDropRate();
};
