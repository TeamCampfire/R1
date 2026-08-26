// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/StatComponent.h"

UStatComponent::UStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatComponent::InitializeStat()
{
	CurrentHealth		= MaxHealth;
	CurrentHydration	= MaxHydration;
	CurrentCalories		= MaxCalories;
	bAlive = true;
}

// 체력
float UStatComponent::GetCurrentHealth_Implementation() const
{
	return CurrentHealth;
}

float UStatComponent::GetMaxHealth_Implementation() const
{
	return MaxHealth;
}

void UStatComponent::InflictDamage_Implementation(float inAmount)
{
	inAmount = FMath::Max(inAmount, 0.0f);
	CurrentHealth -= inAmount;

	if (CurrentHealth < 0.0f)
	{
		CurrentHealth = 0;
		OnHealthChange.Broadcast(CurrentHealth, MaxHealth);
		if (bAlive)
		{
			OnDeath.Broadcast();
		}
		bAlive = false;
	}
	else
	{
		OnHealthChange.Broadcast(CurrentHealth, MaxHealth);
	}
	//DEBUG
	UE_LOG(LogTemp, Log, TEXT("HP : %.1f / %.1f"), CurrentHealth, MaxHealth);

}

void UStatComponent::Heal_Implementation(float inAmount)
{
	inAmount = FMath::Max(inAmount, 0.0f);	// 음수는 0으로 처리
	CurrentHealth = FMath::Min(CurrentHealth + inAmount, MaxHealth);
	OnHealthChange.Broadcast(CurrentHealth, MaxHealth);
	//DEBUG
	UE_LOG(LogTemp, Log, TEXT("HP : %.1f / %.1f"), CurrentHealth, MaxHealth);
}

bool UStatComponent::IsAlive() const
{
	return bAlive;
}

// 스태미나 
float UStatComponent::GetCurrentStamina_Implementation() const
{
	return 0.0f;
}

float UStatComponent::GetMaxStamina_Implementation() const
{
	return 0.0f;
}

bool UStatComponent::ConsumeStamina_Implementation(float InAmount)
{
	bool bResult = false;
	InAmount = FMath::Max(InAmount, 0.0f);

	if (CurrentStamina >= InAmount)
	{
		CurrentStamina -= InAmount;

		UWorld* World = GetWorld();
		if (World)
		{
			/*FTimerManager& TimerManager = GetWorld()->GetTimerManager();
			TimerManager.SetTimer(
				StaminaAutoRecoveryTimerHandle,
				this,
				&UStatComponent::StaminaAutoRecovertyPerTick,
				StaminaRecoveryData.TickInterval,
				true,
				StaminaRecoveryData.CoolTime
			);*/
		}

		OnStaminaChange.Broadcast(CurrentStamina, MaxStamina);	

		if (CurrentStamina < 1)
		{
			OnStaminaEmpty.Broadcast();
		}

		bResult = true;
		UE_LOG(LogTemp, Log, TEXT("Stamina : %.1f / %.1f"), CurrentStamina, MaxStamina);
	}

	return bResult;
}

void UStatComponent::RecoverStamina_Implementation(float InAmount)
{
}
// 칼로리
float UStatComponent::GetCurrentCalories_Implementation() const
{
	return CurrentCalories;
}

bool UStatComponent::DecreaseCalories_Implementation(float inAmount)
{
	bool bCaloriesDecreased = false;
	inAmount = FMath::Max(inAmount, 0.0f);

	if (CurrentCalories >= inAmount)
	{
		CurrentCalories -= inAmount;

		UWorld* World = GetWorld();
		if (World)
		{
			/*FTimerManager& TimerManager = GetWorld()->GetTimerManager();
			TimerManager.SetTimer(
				StaminaAutoRecoveryTimerHandle,
				this,
				&UStatComponent::StaminaAutoRecovertyPerTick,
				StaminaRecoveryData.TickInterval,
				true,
				StaminaRecoveryData.CoolTime
			);*/
		}

		OnStaminaChange.Broadcast(CurrentCalories, MaxCalories);

		if (CurrentCalories < 1)
		{
			OnStaminaEmpty.Broadcast();
		}

		bCaloriesDecreased = true;
		//DEBUG
		UE_LOG(LogTemp, Log, TEXT("HP : %.1f / %.1f"), CurrentCalories, MaxCalories);
	}
	

	return bCaloriesDecreased;
}

void UStatComponent::RecoverCalories_Implementation(float InAmount)
{
}
// 수분
float UStatComponent::GetCurrentHydration_Implementation() const
{
	return 0.0f;
}

bool UStatComponent::DecreaseHydration_Implementation(float InAmount)
{
	return false;
}

void UStatComponent::RecoverHydration_Implementation(float InAmount)
{
}
// 체온
float UStatComponent::GetCurrentTemperature_Implementation() const
{
	return 0.0f;
}

bool UStatComponent::IncreaseTemperature_Implementation(float InAmount)
{
	return false;
}

void UStatComponent::DecreaseTemperature_Implementation(float InAmount)
{
}
// 상태이상
EStatusEffectType UStatComponent::GetCurrentStatusEffect_Implementation()
{
	return EStatusEffectType::None;
}

void UStatComponent::RemoveStatusEffect_Implementation(EStatusEffectType InStatusEffectType)
{
}

void UStatComponent::SetStatusEffect_Implementation(EStatusEffectType InStatusEffectType)
{
}

void UStatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UStatComponent::TestInflictDamage()
{
	Execute_InflictDamage(this, DebugDamage);
}

void UStatComponent::TestConsumeStamina()
{
	Execute_ConsumeStamina(this, DebugStamina);
}
