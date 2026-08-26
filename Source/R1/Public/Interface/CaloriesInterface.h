// Fill out your copyright notice in the Description page of Project Settings.

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
	bool DecreaseCalories(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Calories")
	void RecoverCalories(float inAmount);
};
