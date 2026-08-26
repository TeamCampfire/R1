// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "R1/Interface/HealthInterface.h"
#include "R1/Interface/HungerInterface.h"
#include "R1/Interface/StaminaInterface.h"
#include "R1/Interface/StatusEffectInterface.h"
#include "R1/Interface/TemperatureInterface.h"
#include "R1/Interface/ThirstInterface.h"
#include "StatComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class R1_API UStatComponent : public UActorComponent, public IHealthInterface, public IStaminaInterface, public IHungerInterface, public ITemperatureInterface, public IThirstInterface, public IStatusEffectInterface
{
	GENERATED_BODY()

public:
	UStatComponent();

	virtual float GetCurrentStamina_Implementation() const override;
	virtual float GetMaxStamina_Implementation() const override;
	virtual bool ConsumeStamina_Implementation(float InAmount) override;
	virtual void RecoverStamina_Implementation(float InAmount) override;

	virtual float GetCurrentHunger_Implementation() const override;
	virtual bool IncreaseHunger_Implementation(float InAmount) override;
	virtual void RecoverHunger_Implementation(float InAmount) override;

	virtual float GetCurrentThirst_Implementation() const override;
	virtual bool IncreaseThirst_Implementation(float InAmount) override;
	virtual void RecoverThirst_Implementation(float InAmount) override;

	virtual float GetCurrentTemperature_Implementation() const override;
	virtual bool IncreaseTemperature_Implementation(float InAmount) override;
	virtual void DecreaseTemperature_Implementation(float InAmount) override;

	virtual EStatusEffectType GetCurrentStatusEffect_Implementation() override;
	virtual void RemoveStatusEffect_Implementation(EStatusEffectType InStatusEffectType) override;
	virtual void SetStatusEffect_Implementation(EStatusEffectType InStatusEffectType) override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
