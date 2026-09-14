// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystem/ItemPickupPoolSubsystem.h"
#include "Item/ItemPickup.h"
#include "Data/Item/ItemDataBase.h"

TSubclassOf<AItemPickup> UItemPickupPoolSubsystem::GetDefaultPickupClass() const
{
	// 블루프린트 BP_ItemPickup이 존재하면 우선 사용하고, 없으면 C++ 베이스 클래스로 폴백
	if (UClass* LoadedBPClass = StaticLoadClass(AItemPickup::StaticClass(), nullptr, TEXT("/Game/Blueprint/Item/BP_ItemPickup.BP_ItemPickup_C")))
	{
		return LoadedBPClass;
	}

	return AItemPickup::StaticClass();
}

void UItemPickupPoolSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() != NM_Client)
	{
		EnsurePoolSize(GetDefaultPickupClass(), 20);
	}
}

void UItemPickupPoolSubsystem::Deinitialize()
{
	for (auto& Pair : InactivePool)
	{
		for (TObjectPtr<AItemPickup>& Pickup : Pair.Value.Pickups)
		{
			if (IsValid(Pickup))
			{
				Pickup->Destroy();
			}
		}
		Pair.Value.Pickups.Empty();
	}
	InactivePool.Empty();

	for (TWeakObjectPtr<AItemPickup>& WeakPickup : ActivePool)
	{
		if (WeakPickup.IsValid())
		{
			WeakPickup->Destroy();
		}
	}
	ActivePool.Empty();

	Super::Deinitialize();
}

void UItemPickupPoolSubsystem::PrewarmPool(TSubclassOf<AItemPickup> PickupClass, int32 Count)
{
	UWorld* World = GetWorld();
	if (!World || Count <= 0)
	{
		return;
	}

	if (World->GetNetMode() == NM_Client)
	{
		return;
	}

	TSubclassOf<AItemPickup> EffectiveClass = PickupClass ? PickupClass : GetDefaultPickupClass();
	FItemPickupArray& ArrayForClass = InactivePool.FindOrAdd(EffectiveClass);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector InactiveHoldingLocation(0.f, 0.f, -10000.f);

	for (int32 i = 0; i < Count; ++i)
	{
		AItemPickup* NewPickup = World->SpawnActor<AItemPickup>(EffectiveClass, InactiveHoldingLocation, FRotator::ZeroRotator, SpawnParams);
		if (NewPickup)
		{
#if WITH_EDITOR
			NewPickup->SetFolderPath(FName(TEXT("Pool_ItemPickups")));
			NewPickup->SetActorLabel(FString::Printf(TEXT("[Inactive] %s"), *NewPickup->GetName()));
#endif
			NewPickup->SetIsFromDropPool(true);
			NewPickup->SetPoolActive(false);
			ArrayForClass.Pickups.Add(NewPickup);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[ItemPickupPool] Prewarmed %d pickups of class %s (Current Inactive: %d)"),
		Count, *EffectiveClass->GetName(), ArrayForClass.Pickups.Num());
}

void UItemPickupPoolSubsystem::EnsurePoolSize(TSubclassOf<AItemPickup> PickupClass, int32 TargetSize)
{
	UWorld* World = GetWorld();
	if (!World || TargetSize <= 0)
	{
		return;
	}

	if (World->GetNetMode() == NM_Client)
	{
		return;
	}

	TSubclassOf<AItemPickup> EffectiveClass = PickupClass ? PickupClass : GetDefaultPickupClass();
	FItemPickupArray& ArrayForClass = InactivePool.FindOrAdd(EffectiveClass);

	int32 Needed = TargetSize - ArrayForClass.Pickups.Num();
	if (Needed > 0)
	{
		PrewarmPool(EffectiveClass, Needed);
	}
}

AItemPickup* UItemPickupPoolSubsystem::AcquirePickup(TSubclassOf<AItemPickup> PickupClass, const FVector& Location, const FRotator& Rotation, UItemDataBase* InItemData, int32 InCount)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TSubclassOf<AItemPickup> EffectiveClass = PickupClass ? PickupClass : GetDefaultPickupClass();
	FItemPickupArray& ArrayForClass = InactivePool.FindOrAdd(EffectiveClass);

	AItemPickup* PooledPickup = nullptr;
	while (ArrayForClass.Pickups.Num() > 0)
	{
		TObjectPtr<AItemPickup> Candidate = ArrayForClass.Pickups.Pop();
		if (IsValid(Candidate))
		{
			PooledPickup = Candidate;
			break;
		}
	}

	if (PooledPickup)
	{
		PooledPickup->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
		PooledPickup->InitializeFromItem(InItemData, InCount);
		PooledPickup->SetIsFromDropPool(true);
		PooledPickup->SetPoolActive(true);
#if WITH_EDITOR
		if (InItemData)
		{
			PooledPickup->SetActorLabel(FString::Printf(TEXT("Pickup_%s (%s)"), *(InItemData->DisplayName.IsEmpty() ? InItemData->GetName() : InItemData->DisplayName.ToString()), *PooledPickup->GetName()));
		}
#endif
		ActivePool.Add(PooledPickup);
		UE_LOG(LogTemp, Log, TEXT("[ItemPickupPool] Reused pooled pickup: %s for Item: %s (Remaining Inactive: %d)"),
			*PooledPickup->GetName(), InItemData ? *InItemData->GetName() : TEXT("None"), ArrayForClass.Pickups.Num());
		return PooledPickup;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AItemPickup* NewPickup = World->SpawnActor<AItemPickup>(EffectiveClass, Location, Rotation, SpawnParams);
	if (NewPickup)
	{
#if WITH_EDITOR
		NewPickup->SetFolderPath(FName(TEXT("Pool_ItemPickups")));
		if (InItemData)
		{
			NewPickup->SetActorLabel(FString::Printf(TEXT("Pickup_%s (%s)"), *(InItemData->DisplayName.IsEmpty() ? InItemData->GetName() : InItemData->DisplayName.ToString()), *NewPickup->GetName()));
		}
#endif
		NewPickup->InitializeFromItem(InItemData, InCount);
		NewPickup->SetIsFromDropPool(true);
		NewPickup->SetPoolActive(true);
		ActivePool.Add(NewPickup);
		UE_LOG(LogTemp, Log, TEXT("[ItemPickupPool] Spawned new pickup: %s for Item: %s"),
			*NewPickup->GetName(), InItemData ? *InItemData->GetName() : TEXT("None"));
	}

	return NewPickup;
}

void UItemPickupPoolSubsystem::ReturnPickup(AItemPickup* Pickup)
{
	if (!IsValid(Pickup))
	{
		return;
	}

	ActivePool.Remove(Pickup);
	Pickup->SetPoolActive(false);

	// 비활성화된 액터는 안전 구역으로 이동 및 데이터 초기화
	Pickup->SetActorLocation(FVector(0.f, 0.f, -10000.f));
	Pickup->InitializeFromItem(nullptr, 1);

#if WITH_EDITOR
	Pickup->SetActorLabel(FString::Printf(TEXT("[Inactive] %s"), *Pickup->GetName()));
#endif

	FItemPickupArray& ArrayForClass = InactivePool.FindOrAdd(Pickup->GetClass());
	ArrayForClass.Pickups.Add(Pickup);
	UE_LOG(LogTemp, Log, TEXT("[ItemPickupPool] Returned pickup: %s to pool (Total Inactive: %d)"),
		*Pickup->GetName(), ArrayForClass.Pickups.Num());
}
