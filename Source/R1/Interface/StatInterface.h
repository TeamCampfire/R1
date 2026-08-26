// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StatInterface.generated.h"

UENUM(BlueprintType)
enum class EStatusEffectType : uint8
{
	Cold,
	FoodPoison,
	Thirsty,
	Hungry,
	Hyperthermia,
	Hypothermia,
	Tetanus
};

UINTERFACE(MinimalAPI)
class UStatInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IStatInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Health")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Health")
	void SubHealth(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Health")
	void AddHealth(float inAmount);

	virtual bool IsAlive() const = 0;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Stamina")
	float GetCurrentStamina() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Stamina")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Stamina")
	bool ConsumeStamina(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Stamina")
	void RecoverStamina(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hunger")
	float GetCurrentHunger() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hunger")
	bool IncreaseHunger(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hunger")
	void RecoverHunger(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Thirst")
	float GetCurrentThirst() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Thirst")
	bool IncreaseThirst(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Thirst")
	void RecoverThirst(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Temperature")
	float GetCurrentTemperature() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Temperature")
	bool IncreaseTemperature(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Temperature")
	void DecreaseTemperature(float inAmount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	EStatusEffectType GetCurrentStatusEffect();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void RemoveCold();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void RemoveFoodPoison();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void RemoveThirsty();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void RemoveHungry();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void RemoveHyperthermia();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void RemoveHypothermia();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void RemoveTetanus();
};
