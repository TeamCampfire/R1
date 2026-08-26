// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ThirstInterface.generated.h"

UINTERFACE(MinimalAPI)
class UThirstInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IThirstInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Thirst")
	float GetCurrentThirst() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Thirst")
	bool IncreaseThirst(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Thirst")
	void RecoverThirst(float inAmount);
};
