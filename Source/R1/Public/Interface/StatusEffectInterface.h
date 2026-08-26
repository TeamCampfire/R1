// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StatusEffectInterface.generated.h"

UENUM(BlueprintType)
enum class EStatusEffectType : uint8
{
	None,
	Cold,
	FoodPoison,
	Thirsty,
	Hungry,
	Hyperthermia,
	Hypothermia,
	Tetanus
};

UINTERFACE(MinimalAPI)
class UStatusEffectInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IStatusEffectInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	EStatusEffectType GetCurrentStatusEffect();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void RemoveStatusEffect(EStatusEffectType inStatusEffectType);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void SetStatusEffect(EStatusEffectType inStatusEffectType);
};
