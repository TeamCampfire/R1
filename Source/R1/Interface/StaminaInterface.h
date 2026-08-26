// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StaminaInterface.generated.h"

UINTERFACE(MinimalAPI)
class UStaminaInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IStaminaInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Stamina")
	float GetCurrentStamina() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Stamina")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Stamina")
	bool ConsumeStamina(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Stamina")
	void RecoverStamina(float inAmount);
};
