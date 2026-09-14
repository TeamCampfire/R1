// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ItemPickupPoolSubsystem.generated.h"

class AItemPickup;
class UItemDataBase;

USTRUCT()
struct FItemPickupArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AItemPickup>> Pickups;
};

/**
 * World subsystem that manages an object pool for dynamic AItemPickup actors
 * (such as item drops from broken barrels, crafting overflow, or player drops).
 */
UCLASS()
class R1_API UItemPickupPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/**
	 * Returns the project's unified default pickup class (BP_ItemPickup if found, else AItemPickup).
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	TSubclassOf<AItemPickup> GetDefaultPickupClass() const;

	/**
	 * Pre-spawns additional inactive AItemPickup actors into the pool.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void PrewarmPool(TSubclassOf<AItemPickup> PickupClass, int32 Count);

	/**
	 * Ensures the inactive pool has at least TargetSize actors ready.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void EnsurePoolSize(TSubclassOf<AItemPickup> PickupClass, int32 TargetSize);

	/**
	 * Acquires a pooled AItemPickup actor initialized with the given item data and count.
	 * If no inactive actor is available in the pool, spawns a new one.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	AItemPickup* AcquirePickup(TSubclassOf<AItemPickup> PickupClass, const FVector& Location, const FRotator& Rotation, UItemDataBase* InItemData, int32 InCount);

	/**
	 * Returns an active AItemPickup actor back to the inactive pool.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void ReturnPickup(AItemPickup* Pickup);

private:
	UPROPERTY(Transient)
	TMap<TSubclassOf<AItemPickup>, FItemPickupArray> InactivePool;

	TSet<TWeakObjectPtr<AItemPickup>> ActivePool;
};
