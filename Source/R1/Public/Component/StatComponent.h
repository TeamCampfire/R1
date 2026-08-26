// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/HealthInterface.h"
#include "Interface/CaloriesInterface.h"
#include "Interface/StaminaInterface.h"
#include "Interface/StatusEffectInterface.h"
#include "Interface/TemperatureInterface.h"
#include "Interface/HydrationInterface.h"
#include "StatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatEmpty);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChange, float, Current, float, Max);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class R1_API UStatComponent : public UActorComponent, public IHealthInterface, public IStaminaInterface, public ICaloriesInterface, public ITemperatureInterface, public IHydrationInterface, public IStatusEffectInterface
{
	GENERATED_BODY()

public:
	UStatComponent();

	void InitializeStat();
	// 체력
	virtual float GetCurrentHealth_Implementation() const override;
	virtual float GetMaxHealth_Implementation() const override;
	virtual void InflictDamage_Implementation(float inAmount) override;
	virtual void Heal_Implementation(float inAmount) override;
	virtual bool IsAlive() const override;

	// 스태미나
	virtual float GetCurrentStamina_Implementation() const override;
	virtual float GetMaxStamina_Implementation() const override;
	virtual bool ConsumeStamina_Implementation(float inAmount) override;
	virtual void RecoverStamina_Implementation(float inAmount) override;
	// 칼로리
	virtual float GetCurrentCalories_Implementation() const override;
	virtual bool DecreaseCalories_Implementation(float inAmount) override;
	virtual void RecoverCalories_Implementation(float inAmount) override;
	// 수분
	virtual float GetCurrentHydration_Implementation() const override;
	virtual bool DecreaseHydration_Implementation(float inAmount) override;
	virtual void RecoverHydration_Implementation(float inAmount) override;
	// 체온
	virtual float GetCurrentTemperature_Implementation() const override;
	virtual bool IncreaseTemperature_Implementation(float inAmount) override;
	virtual void DecreaseTemperature_Implementation(float inAmount) override;
	// 상태이상
	virtual EStatusEffectType GetCurrentStatusEffect_Implementation() override;
	virtual void RemoveStatusEffect_Implementation(EStatusEffectType inStatusEffectType) override;
	virtual void SetStatusEffect_Implementation(EStatusEffectType inStatusEffectType) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHydration = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHydration = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentCalories = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxCalories = 100.0f;


	// Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DebugDamage = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DebugStamina = 10.0f;

private:
	bool bAlive = false;

public:
	UPROPERTY(BlueprintAssignable, Category = "Stat|Stamina")
	FOnStatEmpty OnStaminaEmpty;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Health")
	FOnStatEmpty OnDeath;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Stamina")
	FOnStatChange OnStaminaChange;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Health")
	FOnStatChange OnHealthChange;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Hydration")
	FOnStatChange OnHydrationChange;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Calories")
	FOnStatChange OnCaloryChange;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	UFUNCTION(CallInEditor, Category = "Debug")
	void TestInflictDamage();
	UFUNCTION(CallInEditor, Category = "Debug")
	void TestConsumeStamina();

};
