#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Campfire/CampfireTypes.h"
#include "Item/ItemInstance.h"
#include "Component/InventoryComponent.h"
#include "CampfireComponent.generated.h"

class UCampfireConfigDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCampfireStateChanged);

UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class R1_API UCampfireComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCampfireComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Campfire")
	FItemInstance GetSlot(const FCampfireSlotRef& Slot) const;

	UFUNCTION(BlueprintPure, Category = "Campfire")
	bool CanAcceptItem(const FCampfireSlotRef& Slot, const UItemDataBase* Item) const;

	UFUNCTION(BlueprintPure, Category = "Campfire")
	float GetCookingProgress() const;

	UFUNCTION(BlueprintPure, Category = "Campfire")
	float GetFuelProgress() const;

	bool MoveFromInventory(UInventoryComponent* Inventory, const FInventorySlotRef& From,
		const FCampfireSlotRef& To, int32 Count, bool bHalfSplit);
	bool MoveToInventory(UInventoryComponent* Inventory, const FCampfireSlotRef& From,
		const FInventorySlotRef& To, int32 Count, bool bHalfSplit);
	bool QuickMoveFromInventory(UInventoryComponent* Inventory, const FInventorySlotRef& From);
	bool QuickMoveToInventory(UInventoryComponent* Inventory, const FCampfireSlotRef& From);

	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void SetLit(bool bNewLit);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UCampfireConfigDataAsset> Config;

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	FItemInstance InputSlot;

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	FItemInstance FuelSlot;

	// 0 = 연료 부산물, 1 = 조리 결과물.
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	TArray<FItemInstance> OutputSlots;

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	bool bIsLit = false;

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	float RemainingFuelTime = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	float CurrentFuelDuration = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	float CurrentCookingTime = 0.f;

	UPROPERTY(BlueprintAssignable, Category = "Campfire")
	FOnCampfireStateChanged OnCampfireStateChanged;

private:
	UFUNCTION()
	void OnRep_State();

	void TickCampfire();
	bool ConsumeNextFuel();
	bool CanStoreOutput(int32 OutputIndex, UItemDataBase* Item, int32 Count = 1) const;
	bool AddOutput(int32 OutputIndex, UItemDataBase* Item, int32 Count = 1);
	void NotifyStateChanged();
	FItemInstance* FindMutableSlot(const FCampfireSlotRef& Slot);
	const FItemInstance* FindSlot(const FCampfireSlotRef& Slot) const;
	bool CompleteCurrentFuel();

	// 서버에서 현재 타고 있는 연료가 완료됐을 때 만들 부산물. nullptr이면 소비만 한다.
	UPROPERTY(Transient)
	TObjectPtr<UItemDataBase> PendingFuelOutputItem;

	FTimerHandle CampfireTimer;
	static constexpr float UpdateInterval = 0.1f;
};
