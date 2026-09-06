#include "Campfire/CampfireComponent.h"

#include "Component/InventoryComponent.h"
#include "Data/Campfire/CampfireConfigDataAsset.h"
#include "Data/Item/ItemDataBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UCampfireComponent::UCampfireComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UCampfireComponent::BeginPlay()
{
	Super::BeginPlay();
	OutputSlots.SetNum(2);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(CampfireTimer, this,
			&UCampfireComponent::TickCampfire, UpdateInterval, true);
	}
}

void UCampfireComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCampfireComponent, InputSlot);
	DOREPLIFETIME(UCampfireComponent, FuelSlot);
	DOREPLIFETIME(UCampfireComponent, OutputSlots);
	DOREPLIFETIME(UCampfireComponent, bIsLit);
	DOREPLIFETIME(UCampfireComponent, RemainingFuelTime);
	DOREPLIFETIME(UCampfireComponent, CurrentFuelDuration);
	DOREPLIFETIME(UCampfireComponent, CurrentCookingTime);
}

const FItemInstance* UCampfireComponent::FindSlot(const FCampfireSlotRef& Slot) const
{
	switch (Slot.Type)
	{
	case ECampfireSlotType::Input: return Slot.Index == 0 ? &InputSlot : nullptr;
	case ECampfireSlotType::Fuel: return Slot.Index == 0 ? &FuelSlot : nullptr;
	case ECampfireSlotType::Output: return OutputSlots.IsValidIndex(Slot.Index) ? &OutputSlots[Slot.Index] : nullptr;
	default: return nullptr;
	}
}

FItemInstance* UCampfireComponent::FindMutableSlot(const FCampfireSlotRef& Slot)
{
	return const_cast<FItemInstance*>(const_cast<const UCampfireComponent*>(this)->FindSlot(Slot));
}

FItemInstance UCampfireComponent::GetSlot(const FCampfireSlotRef& Slot) const
{
	const FItemInstance* Found = FindSlot(Slot);
	return Found ? *Found : FItemInstance();
}

bool UCampfireComponent::CanAcceptItem(const FCampfireSlotRef& Slot, const UItemDataBase* Item) const
{
	if (!Config || !Item || Slot.Index != 0)
	{
		return false;
	}
	return (Slot.Type == ECampfireSlotType::Input && Config->FindCookingRecipe(Item))
		|| (Slot.Type == ECampfireSlotType::Fuel && Config->FindFuelRecipe(Item));
}

float UCampfireComponent::GetCookingProgress() const
{
	return Config && Config->CookingTime > 0.f
		? FMath::Clamp(CurrentCookingTime / Config->CookingTime, 0.f, 1.f) : 0.f;
}

float UCampfireComponent::GetFuelProgress() const
{
	return CurrentFuelDuration > 0.f
		? FMath::Clamp(RemainingFuelTime / CurrentFuelDuration, 0.f, 1.f) : 0.f;
}

bool UCampfireComponent::CanStoreOutput(int32 OutputIndex, UItemDataBase* Item, int32 Count) const
{
	if (!Item || !OutputSlots.IsValidIndex(OutputIndex) || Count <= 0) return false;
	const FItemInstance& Output = OutputSlots[OutputIndex];
	return !Output.IsValid() || (Output.ItemData == Item && Output.StackCount + Count <= Item->MaxStackSize);
}

bool UCampfireComponent::AddOutput(int32 OutputIndex, UItemDataBase* Item, int32 Count)
{
	if (!CanStoreOutput(OutputIndex, Item, Count)) return false;
	FItemInstance& Output = OutputSlots[OutputIndex];
	if (Output.IsValid()) Output.StackCount += Count;
	else Output = FItemInstance(Item, Count);
	return true;
}

bool UCampfireComponent::ConsumeNextFuel()
{
	if (!Config || !FuelSlot.IsValid()) return false;
	const FCampfireFuelRecipe* FuelRecipe = Config->FindFuelRecipe(FuelSlot.ItemData);
	if (!FuelRecipe) return false;

	--FuelSlot.StackCount;
	if (FuelSlot.StackCount <= 0) FuelSlot = FItemInstance();
	PendingFuelOutputItem = FuelRecipe->AfterItem;
	CurrentFuelDuration = FMath::Max(0.1f, Config->BurningTime);
	RemainingFuelTime = CurrentFuelDuration;
	return true;
}

bool UCampfireComponent::CompleteCurrentFuel()
{
	if (!PendingFuelOutputItem) return true;
	if (!CanStoreOutput(0, PendingFuelOutputItem)) return false;
	AddOutput(0, PendingFuelOutputItem);
	PendingFuelOutputItem = nullptr;
	return true;
}

void UCampfireComponent::TickCampfire()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bIsLit || !Config) return;

	if (RemainingFuelTime <= 0.f && (!CompleteCurrentFuel() || !ConsumeNextFuel()))
	{
		bIsLit = false;
		CurrentCookingTime = 0.f;
		NotifyStateChanged();
		return;
	}

	RemainingFuelTime = FMath::Max(0.f, RemainingFuelTime - UpdateInterval);

	const FCampfireCookingRecipe* CookingRecipe = InputSlot.IsValid()
		? Config->FindCookingRecipe(InputSlot.ItemData) : nullptr;
	const bool bCanCook = CookingRecipe && CookingRecipe->AfterItem
		&& CanStoreOutput(1, CookingRecipe->AfterItem);

	if (bCanCook)
	{
		CurrentCookingTime += UpdateInterval;
		if (CurrentCookingTime >= FMath::Max(0.1f, Config->CookingTime))
		{
			--InputSlot.StackCount;
			if (InputSlot.StackCount <= 0) InputSlot = FItemInstance();
			AddOutput(1, CookingRecipe->AfterItem);
			CurrentCookingTime = 0.f;
		}
	}
	else
	{
		CurrentCookingTime = 0.f;
	}

	NotifyStateChanged();
}

void UCampfireComponent::SetLit(bool bNewLit)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (bNewLit && RemainingFuelTime <= 0.f)
	{
		if (!CompleteCurrentFuel() || !ConsumeNextFuel()) return;
	}
	bIsLit = bNewLit;
	if (!bIsLit) CurrentCookingTime = 0.f;
	NotifyStateChanged();
}

void UCampfireComponent::NotifyStateChanged()
{
	OnCampfireStateChanged.Broadcast();
	if (AActor* Owner = GetOwner()) Owner->ForceNetUpdate();
}

void UCampfireComponent::OnRep_State()
{
	OnCampfireStateChanged.Broadcast();
}

bool UCampfireComponent::MoveFromInventory(UInventoryComponent* Inventory, const FInventorySlotRef& From,
	const FCampfireSlotRef& To, int32 Count, bool bHalfSplit)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Inventory) return false;
	TArray<FItemInstance>& SourceArray = Inventory->GetSlotArray(From.Category);
	if (!SourceArray.IsValidIndex(From.Index) || !SourceArray[From.Index].IsValid()) return false;
	const FItemInstance Source = SourceArray[From.Index];
	if (!CanAcceptItem(To, Source.ItemData)) return false;
	FItemInstance* Target = FindMutableSlot(To);
	if (!Target) return false;

	int32 MoveCount = Count > 0 ? FMath::Min(Count, Source.StackCount) : Source.StackCount;
	if (bHalfSplit && Count <= 0 && !Target->IsValid() && Source.StackCount >= 2) MoveCount = Source.StackCount / 2;
	if (!Target->IsValid()) MoveCount = FMath::Min(MoveCount, Source.ItemData->MaxStackSize);
	if (Target->IsValid())
	{
		if (Target->ItemData != Source.ItemData) return false;
		MoveCount = FMath::Min(MoveCount, Source.ItemData->MaxStackSize - Target->StackCount);
	}
	if (MoveCount <= 0) return false;

	if (Target->IsValid()) Target->StackCount += MoveCount;
	else *Target = FItemInstance(Source.ItemData, MoveCount);
	FItemInstance Remaining = Source;
	Remaining.StackCount -= MoveCount;
	Inventory->SetSlot(From.Category, From.Index, Remaining.StackCount > 0 ? Remaining : FItemInstance());
	NotifyStateChanged();
	return true;
}

bool UCampfireComponent::MoveToInventory(UInventoryComponent* Inventory, const FCampfireSlotRef& From,
	const FInventorySlotRef& To, int32 Count, bool bHalfSplit)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Inventory) return false;
	FItemInstance* SourcePtr = FindMutableSlot(From);
	if (!SourcePtr || !SourcePtr->IsValid() || To.Category == EInventorySlotCategory::Equipment) return false;
	TArray<FItemInstance>& TargetArray = Inventory->GetSlotArray(To.Category);
	if (!TargetArray.IsValidIndex(To.Index)) return false;

	const FItemInstance Source = *SourcePtr;
	const FItemInstance Target = TargetArray[To.Index];
	int32 MoveCount = Count > 0 ? FMath::Min(Count, Source.StackCount) : Source.StackCount;
	if (bHalfSplit && Count <= 0 && !Target.IsValid() && Source.StackCount >= 2) MoveCount = Source.StackCount / 2;
	if (!Target.IsValid()) MoveCount = FMath::Min(MoveCount, Source.ItemData->MaxStackSize);
	if (Target.IsValid())
	{
		if (Target.ItemData != Source.ItemData) return false;
		MoveCount = FMath::Min(MoveCount, Source.ItemData->MaxStackSize - Target.StackCount);
	}
	if (MoveCount <= 0) return false;

	FItemInstance NewTarget = Target.IsValid() ? Target : FItemInstance(Source.ItemData, 0);
	NewTarget.StackCount += MoveCount;
	Inventory->SetSlot(To.Category, To.Index, NewTarget);
	SourcePtr->StackCount -= MoveCount;
	if (SourcePtr->StackCount <= 0) *SourcePtr = FItemInstance();
	NotifyStateChanged();
	return true;
}

bool UCampfireComponent::QuickMoveFromInventory(UInventoryComponent* Inventory, const FInventorySlotRef& From)
{
	if (!Inventory) return false;
	const TArray<FItemInstance>& Slots = Inventory->GetSlotArray(From.Category);
	if (!Slots.IsValidIndex(From.Index) || !Slots[From.Index].IsValid()) return false;
	const UItemDataBase* Item = Slots[From.Index].ItemData;
	const FCampfireSlotRef Target = Config && Config->FindCookingRecipe(Item)
		? FCampfireSlotRef{ ECampfireSlotType::Input, 0 }
		: FCampfireSlotRef{ ECampfireSlotType::Fuel, 0 };
	return CanAcceptItem(Target, Item) && MoveFromInventory(Inventory, From, Target, 0, false);
}

bool UCampfireComponent::QuickMoveToInventory(UInventoryComponent* Inventory, const FCampfireSlotRef& From)
{
	if (!Inventory || !GetSlot(From).IsValid()) return false;
	const FItemInstance Source = GetSlot(From);
	for (int32 i = 0; i < Inventory->MainSlots.Num(); ++i)
	{
		const FItemInstance& Slot = Inventory->MainSlots[i];
		if (Slot.IsValid() && Slot.ItemData == Source.ItemData && Slot.StackCount < Source.ItemData->MaxStackSize)
		{
			if (MoveToInventory(Inventory, From, { EInventorySlotCategory::Main, i }, 0, false) && !GetSlot(From).IsValid()) return true;
		}
	}
	for (int32 i = 0; i < Inventory->MainSlots.Num(); ++i)
	{
		if (!Inventory->MainSlots[i].IsValid()) return MoveToInventory(Inventory, From, { EInventorySlotCategory::Main, i }, 0, false);
	}
	return false;
}
