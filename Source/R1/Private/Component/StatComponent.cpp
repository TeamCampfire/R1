// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/StatComponent.h"
#include "Net/UnrealNetwork.h"

UStatComponent::UStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UStatComponent::InitializeStat()
{
	CurrentHealth		= MaxHealth;
	CurrentHydration	= MaxHydration;
	CurrentCalories		= MaxCalories;
	bAlive = true;
}

float UStatComponent::GetCurrentHealth_Implementation() const
{
	return CurrentHealth;
}

float UStatComponent::GetMaxHealth_Implementation() const
{
	return MaxHealth;
}

void UStatComponent::InflictDamage_Implementation(float InAmount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	DecreaseParameter(EParameterType::Health, InAmount);
	//DEBUG
	//ㄴUE_LOG(LogTemp, Log, TEXT("HP : %.1f / %.1f"), CurrentHealth, MaxHealth);

}

void UStatComponent::Heal_Implementation(float InAmount)
{
	IncreaseParameter(EParameterType::Health, InAmount);
	//DEBUG
	//UE_LOG(LogTemp, Log, TEXT("HP : %.1f / %.1f"), CurrentHealth, MaxHealth);
}

bool UStatComponent::IsAlive() const
{
	return bAlive;
}


float UStatComponent::GetCurrentCalories_Implementation() const
{
	return CurrentCalories;
}

float UStatComponent::GetMaxCalories_Implementation() const
{
	return MaxCalories;
}

bool UStatComponent::DecreaseCalories_Implementation(float InAmount)
{
	DecreaseParameter(EParameterType::Calories, InAmount);

	//DEBUG
	//UE_LOG(LogTemp, Log, TEXT("Calories : %.1f / %.1f"), CurrentCalories, MaxCalories);
	return false;
}

void UStatComponent::RecoverCalories_Implementation(float InAmount)
{
	IncreaseParameter(EParameterType::Calories, InAmount);
	//DEBUG
	//UE_LOG(LogTemp, Log, TEXT("Calories : %.1f / %.1f"), CurrentCalories, MaxCalories);
}

float UStatComponent::GetCurrentHydration_Implementation() const
{
	return CurrentHydration;
}

float UStatComponent::GetMaxHydration_Implementation() const
{
	return MaxHydration;
}

bool UStatComponent::DecreaseHydration_Implementation(float InAmount)
{
	DecreaseParameter(EParameterType::Hydration, InAmount);

	//DEBUG
	//UE_LOG(LogTemp, Log, TEXT("Hydration : %.1f / %.1f"), CurrentHydration, MaxHydration);
	return false;
}

void UStatComponent::RecoverHydration_Implementation(float InAmount)
{
	IncreaseParameter(EParameterType::Hydration, InAmount);
	//DEBUG
	//UE_LOG(LogTemp, Log, TEXT("Hydration : %.1f / %.1f"), CurrentHydration, MaxHydration);
}

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

EStatusEffect UStatComponent::GetCurrentStatusEffect_Implementation()
{
	return PlayerStatusEffects;
}

void UStatComponent::RemoveStatusEffect_Implementation(EStatusEffect InStatusEffectType)
{
}

void UStatComponent::SetStatusEffect_Implementation(EStatusEffect InStatusEffectType)
{
}

void UStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStatComponent, CurrentHealth);
	DOREPLIFETIME(UStatComponent, CurrentHydration);
	DOREPLIFETIME(UStatComponent, CurrentCalories);
	DOREPLIFETIME(UStatComponent, PlayerStatusEffects);
	DOREPLIFETIME(UStatComponent, bAlive);
}

void UStatComponent::IncreaseParameter(EParameterType InEParameterType, float InAmount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!bAlive) return;

	switch (InEParameterType)
	{
	case EParameterType::Health:
		InAmount = FMath::Max(InAmount, 0.0f);	
		CurrentHealth = FMath::Min(CurrentHealth + InAmount, MaxHealth);
		OnHealthChange.Broadcast(CurrentHealth, MaxHealth);
		break;

	case EParameterType::Hydration:
	{
		InAmount = FMath::Max(InAmount, 0.0f);
		CurrentHydration = FMath::Min(CurrentHydration + InAmount, MaxHydration);

		OnHydrationChange.Broadcast(CurrentHydration, MaxHydration);

		EStatusEffect NewEffect = EStatusEffect::None;

		if (CurrentHydration <= DehydrationThreshold)
		{
			NewEffect = EStatusEffect::Dehydrated;
		}
		else if (CurrentHydration <= ThirstThreshold)
		{
			NewEffect = EStatusEffect::Thirsty;
		}

		const EStatusEffect CurrentEffect =
			EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Dehydrated)
			? EStatusEffect::Dehydrated
			: EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Thirsty)
			? EStatusEffect::Thirsty
			: EStatusEffect::None;

		if (CurrentEffect != NewEffect)
		{
			PlayerStatusEffects &= ~EStatusEffect::Thirsty;
			PlayerStatusEffects &= ~EStatusEffect::Dehydrated;

			PlayerStatusEffects |= NewEffect;

			OnStatusEffectChange.Broadcast();
			UpdateStatusEffectDamage();
		}

		break;
	}
	case EParameterType::Calories:
	{
		InAmount = FMath::Max(InAmount, 0.0f);
		CurrentCalories = FMath::Min(CurrentCalories + InAmount, MaxCalories);

		OnCaloryChange.Broadcast(CurrentCalories, MaxCalories);

		EStatusEffect NewEffect = EStatusEffect::None;

		if (CurrentCalories <= StarvationThreshold)
		{
			NewEffect = EStatusEffect::Starving;
		}
		else if (CurrentCalories <= HungerThreshold)
		{
			NewEffect = EStatusEffect::Hungry;
		}

		const EStatusEffect CurrentEffect =
			EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Starving)
			? EStatusEffect::Starving
			: EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Hungry)
			? EStatusEffect::Hungry
			: EStatusEffect::None;

		if (CurrentEffect != NewEffect)
		{
			PlayerStatusEffects &= ~EStatusEffect::Hungry;
			PlayerStatusEffects &= ~EStatusEffect::Starving;

			PlayerStatusEffects |= NewEffect;

			OnStatusEffectChange.Broadcast();
			UpdateStatusEffectDamage();
		}

		break;
	}
	case EParameterType::Temperature:
		break;

	default:
		break;
	}
}

void UStatComponent::DecreaseParameter(EParameterType InEParameterType, float InAmount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!bAlive) return;

	switch (InEParameterType)
	{
	case EParameterType::Health:
		InAmount = FMath::Max(InAmount, 0.0f);
		CurrentHealth -= InAmount;

		if (CurrentHealth < 0.0f || FMath::IsNearlyZero(CurrentHealth))
		{
			CurrentHealth = 0;
			OnHealthChange.Broadcast(CurrentHealth, MaxHealth);
			if (bAlive)
			{
				OnDeath.Broadcast();
				UE_LOG(LogTemp, Log, TEXT("%s Died"), *this->GetOwner()->GetName());
			}
			bAlive = false;
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[SERVER] %s bAlive = %s"),
				*GetOwner()->GetName(),
				bAlive ? TEXT("TRUE") : TEXT("FALSE")
			);
		}
		else
		{
			OnHealthChange.Broadcast(CurrentHealth, MaxHealth);
		}
		break;

	case EParameterType::Hydration:
	{
		InAmount = FMath::Max(InAmount, 0.0f);
		CurrentHydration = FMath::Max(CurrentHydration - InAmount, 0.0f);

		OnHydrationChange.Broadcast(CurrentHydration, MaxHydration);

		EStatusEffect NewEffect = EStatusEffect::None;

		if (CurrentHydration <= DehydrationThreshold)
		{
			NewEffect = EStatusEffect::Dehydrated;
		}
		else if (CurrentHydration <= ThirstThreshold)
		{
			NewEffect = EStatusEffect::Thirsty;
		}

		const EStatusEffect CurrentEffect =
			EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Dehydrated)
			? EStatusEffect::Dehydrated
			: EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Thirsty)
			? EStatusEffect::Thirsty
			: EStatusEffect::None;

		if (CurrentEffect != NewEffect)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("=== STATUS CHANGED === Owner=%s Before=%d After=%d"),
				*GetNameSafe(GetOwner()),
				static_cast<uint8>(CurrentEffect),
				static_cast<uint8>(NewEffect));

			PlayerStatusEffects &= ~EStatusEffect::Thirsty;
			PlayerStatusEffects &= ~EStatusEffect::Dehydrated;

			PlayerStatusEffects |= NewEffect;

			OnStatusEffectChange.Broadcast();
			UpdateStatusEffectDamage();
		}

		break;
	}

	case EParameterType::Calories:
	{
		InAmount = FMath::Max(InAmount, 0.0f);
		CurrentCalories = FMath::Max(CurrentCalories - InAmount, 0.0f);

		OnCaloryChange.Broadcast(CurrentCalories, MaxCalories);

		EStatusEffect NewEffect = EStatusEffect::None;

		if (CurrentCalories <= StarvationThreshold)
		{
			NewEffect = EStatusEffect::Starving;
		}
		else if (CurrentCalories <= HungerThreshold)
		{
			NewEffect = EStatusEffect::Hungry;
		}

		const EStatusEffect CurrentEffect =
			EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Starving)
			? EStatusEffect::Starving
			: EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Hungry)
			? EStatusEffect::Hungry
			: EStatusEffect::None;

		if (CurrentEffect != NewEffect)
		{
			PlayerStatusEffects &= ~EStatusEffect::Hungry;
			PlayerStatusEffects &= ~EStatusEffect::Starving;

			PlayerStatusEffects |= NewEffect;

			OnStatusEffectChange.Broadcast();
			UpdateStatusEffectDamage();
		}

		break;
	}

	case EParameterType::Temperature:
		break;

	default:
		break;
	}

}

void UStatComponent::DrainSurvivalStats()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	Execute_DecreaseCalories(this, DefaultCaloryDropRate);
	Execute_DecreaseHydration(this, DefaultHydrationDropRate);
}

void UStatComponent::UpdateStatusEffectDamage()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	// Starving
	if (EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Starving))
	{
		if (!GetWorld()->GetTimerManager().IsTimerActive(StarvationDamageTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(
				StarvationDamageTimerHandle,
				this,
				&UStatComponent::ApplyStarvationDamage,
				SurvivalStatUpdateInterval,
				true);
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(StarvationDamageTimerHandle);
	}

	// Dehydrated
	if (EnumHasAnyFlags(PlayerStatusEffects, EStatusEffect::Dehydrated))
	{
		if (!GetWorld()->GetTimerManager().IsTimerActive(DehydrationDamageTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(
				DehydrationDamageTimerHandle,
				this,
				&UStatComponent::ApplyDehydrationDamage,
				SurvivalStatUpdateInterval,
				true);
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(DehydrationDamageTimerHandle);
	}
}

void UStatComponent::ApplyStarvationDamage()
{
	Execute_InflictDamage(this, StarvationDamage);
}

void UStatComponent::ApplyDehydrationDamage()
{
	Execute_InflictDamage(this, DehydrationDamage);
}

void UStatComponent::OnRep_CurrentHealth()
{
	UE_LOG(LogTemp, Warning,
		TEXT("[CLIENT] OnRep_CurrentHealth Owner=%s Health=%.2f"),
		*GetNameSafe(GetOwner()),
		CurrentHealth);

	OnHealthChange.Broadcast(CurrentHealth, MaxHealth);
}

void UStatComponent::OnRep_CurrentHydration()
{
	OnHydrationChange.Broadcast(CurrentHydration, MaxHydration);
}

void UStatComponent::OnRep_CurrentCalories()
{
	OnCaloryChange.Broadcast(CurrentCalories, MaxCalories);
}

void UStatComponent::OnRep_PlayerStatusEffects()
{
	//UE_LOG(LogTemp, Warning,
	//	TEXT("=== ONREP STATUS === Owner=%s Authority=%d Effects=%d"),
	//	*GetNameSafe(GetOwner()),
	//	GetOwner() ? GetOwner()->HasAuthority() : false,
	//	static_cast<uint8>(PlayerStatusEffects));

	OnStatusEffectChange.Broadcast();
}


void UStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// For Debug.
	//GetWorld()->GetTimerManager().SetTimer(
	//	SurvivalStatTimerHandle,
	//	this,
	//	&UStatComponent::DrainSurvivalStats,
	//	SurvivalStatUpdateInterval,
	//	true,
	//	SurvivalStatUpdateInterval
	//);
}

void UStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}




void UStatComponent::TestInflictDamage()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Execute_InflictDamage(this, DebugDamage);
	}
}

void UStatComponent::TestDecreaseHydration()
{
	Execute_DecreaseHydration(this, DehydrationDamage*22);
}

void UStatComponent::TestDecreaseCalories()
{
	Execute_DecreaseCalories(this, StarvationDamage*22);
}

void UStatComponent::TestIncreaseHydration()
{
	Execute_RecoverHydration(this, DehydrationDamage * 22);
}

void UStatComponent::TestIncreaseCalories()
{
	Execute_RecoverCalories(this, StarvationDamage * 22);
}

void UStatComponent::TestAddHungry()
{
	PlayerStatusEffects |= EStatusEffect::Hungry;
	OnStatusEffectChange.Broadcast();
}

void UStatComponent::TestAddStarving()
{
	PlayerStatusEffects |= EStatusEffect::Starving;
	OnStatusEffectChange.Broadcast();
}
