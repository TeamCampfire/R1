/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TemperatureInterface.generated.h"

UINTERFACE(MinimalAPI)
class UTemperatureInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API ITemperatureInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Temperature")
	float GetCurrentTemperature() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Temperature")
	bool IncreaseTemperature(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Temperature")
	void DecreaseTemperature(float inAmount);
};
