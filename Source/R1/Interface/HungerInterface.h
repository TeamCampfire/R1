// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HungerInterface.generated.h"

UINTERFACE(MinimalAPI)
class UHungerInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IHungerInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hunger")
	float GetCurrentHunger() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hunger")
	bool IncreaseHunger(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hunger")
	void RecoverHunger(float inAmount);
};
