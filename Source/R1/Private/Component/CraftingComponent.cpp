#include "Component/CraftingComponent.h"

#include "Component/InventoryComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Item/ItemPickup.h"
#include "Item/PlaceableItem/Workbench.h"
#include "Net/UnrealNetwork.h"

namespace Crafting
{
	constexpr float MinimumDuration = 0.1f;
	constexpr float DropDistance = 100.f;

	// 중복 재료 행을 합쳐 최대 수량 검사와 실제 차감이 같은 비용을 사용하게 한다.
	bool CollectIngredientCosts(UItemDataBase* Item, TMap<UItemDataBase*, int32>& OutCosts)
	{
		OutCosts.Reset();
		if (!Item || Item->CraftingCost.IsEmpty())
		{
			return false;
		}
		for (const FCraftIngredient& Cost : Item->CraftingCost)
		{
			UItemDataBase* Material = Cost.Item.LoadSynchronous();
			if (!Material || Cost.Amount <= 0)
			{
				return false;
			}
			int32& Total = OutCosts.FindOrAdd(Material);
			if (Total > MAX_int32 - Cost.Amount)
			{
				return false;
			}
			Total += Cost.Amount;
		}
		return true;
	}
}

UCraftingComponent::UCraftingComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Recipes.IsEmpty())
	{
		LoadDefaultRecipes();
	}
}

void UCraftingComponent::LoadDefaultRecipes()
{
	// 디자이너가 Recipes를 지정한 경우 기본 목록을 덮어쓰지 않는다. 횃불은 별도 담당자가 연결한다.
	const TCHAR* Paths[] = {
		TEXT("ConsumableItem/DA_Item_Consumable_Bandage"),
		TEXT("HeldItem/DA_Item_Held_StoneHatchet_2"),
		TEXT("HeldItem/DA_Item_Held_StonePickaxe_2"),
		TEXT("Placeable/DA_Item_Placeable_Campfire"),
		TEXT("HeldItem/DA_Item_Held_FishingRod_2"),
		TEXT("Placeable/DA_Item_Placeable_SleepingBag"),
		TEXT("Placeable/DA_Item_Placeable_Workbench"),
		TEXT("HeldItem/DA_Item_Held_Hatchet"),
		TEXT("HeldItem/DA_Item_Held_Pickaxe"),
		TEXT("HeldItem/DA_Item_Held_Hammer")
	};
	for (const TCHAR* Path : Paths)
	{
		const FString AssetPath = FString(TEXT("/Game/Data/Item/")) + Path;
		if (UItemDataBase* Item = LoadObject<UItemDataBase>(nullptr, *AssetPath))
		{
			Recipes.Add(Item);
		}
	}
}

UInventoryComponent* UCraftingComponent::Inventory() const
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
}

float UCraftingComponent::GetServerTime() const
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

int32 UCraftingComponent::GetMaximum(UItemDataBase* Item, AWorkbench* Bench) const
{
	const UInventoryComponent* PlayerInventory = Inventory();
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!PlayerInventory || !Item || !Recipes.Contains(Item))
	{
		return 0;
	}
	if (Item->bRequiresWorkbench && (!IsValid(Bench) || !Bench->CanInteract_Implementation(Controller->GetPawn())))
	{
		return 0;
	}
	TMap<UItemDataBase*, int32> Required;
	if (!Crafting::CollectIngredientCosts(Item, Required))
	{
		return 0;
	}

	// 예: 목재 450/200=2, 석재 100/100=1이면 돌 도끼는 최대 1개다.
	int32 Maximum = MAX_int32;
	for (const auto& Ingredient : Required)
	{
		Maximum = FMath::Min(Maximum, PlayerInventory->GetItemCount(Ingredient.Key) / Ingredient.Value);
	}
	return Maximum;
}

void UCraftingComponent::Server_Enqueue_Implementation(UItemDataBase* Item, int32 Count, AWorkbench* Bench)
{
	if (Count <= 0 || Queue.Num() >= MaxQueueSize || Count > GetMaximum(Item, Bench))
	{
		return;
	}
	TMap<UItemDataBase*, int32> Required;
	if (!Crafting::CollectIngredientCosts(Item, Required))
	{
		return;
	}
	// 등록 시 재료를 선불 차감하여 같은 재료로 중복 제작을 예약하지 못하게 한다.
	// GetMaximum 검사로 각 곱셈 결과가 보유량(int32) 이하임이 보장된다.
	for (const auto& Ingredient : Required)
	{
		Inventory()->ConsumeItemCount(Ingredient.Key, Ingredient.Value * Count);
	}
	FCraftingOrder Order;
	Order.Id = FGuid::NewGuid();
	Order.Item = Item;
	Order.Remaining = Count;
	Queue.Add(Order);
	if (Queue.Num() == 1)
	{
		StartNextItem();
	}
}

void UCraftingComponent::StartNextItem()
{
	if (!Queue.IsEmpty() && Queue[0].Item)
	{
		Queue[0].FinishTime = GetServerTime() + FMath::Max(Crafting::MinimumDuration, Queue[0].Item->CraftingSeconds);
	}
}

AItemPickup* UCraftingComponent::SpawnOverflowPickup() const
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		return nullptr;
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Location = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * Crafting::DropDistance;
	return GetWorld()->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), Location, FRotator::ZeroRotator, Parameters);
}

bool UCraftingComponent::GiveOrDrop(UItemDataBase* Item, int32& Count)
{
	if (UInventoryComponent* PlayerInventory = Inventory())
	{
		int32 Remainder = Count;
		PlayerInventory->AddItem(Item, Count, Remainder);
		Count = Remainder;
	}
	if (Count <= 0)
	{
		return true;
	}
	if (AItemPickup* Pickup = SpawnOverflowPickup())
	{
		Pickup->InitializeFromItem(Item, Count);
		Count = 0;
		return true;
	}
	return false;
}

bool UCraftingComponent::RefundOrder(const FCraftingOrder& Order)
{
	UInventoryComponent* PlayerInventory = Inventory();
	TMap<UItemDataBase*, int32> Required;
	if (!PlayerInventory || !Crafting::CollectIngredientCosts(Order.Item, Required))
	{
		return false;
	}

	// 먼저 반환용 픽업을 확보한다. 생성 실패 시 인벤토리와 주문을 변경하지 않는다.
	TArray<AItemPickup*> RefundPickups;
	for (int32 Index = 0; Index < Required.Num(); ++Index)
	{
		AItemPickup* Pickup = SpawnOverflowPickup();
		if (!Pickup)
		{
			for (AItemPickup* ReservedPickup : RefundPickups)
			{
				ReservedPickup->Destroy();
			}
			return false;
		}
		RefundPickups.Add(Pickup);
	}

	int32 PickupIndex = 0;
	for (const auto& Ingredient : Required)
	{
		int32 Remainder = 0;
		PlayerInventory->AddItem(Ingredient.Key, Ingredient.Value * Order.Remaining, Remainder);
		AItemPickup* Pickup = RefundPickups[PickupIndex++];
		if (Remainder > 0)
		{
			Pickup->InitializeFromItem(Ingredient.Key, Remainder);
		}
		else
		{
			Pickup->Destroy();
		}
	}
	return true;
}

void UCraftingComponent::Server_Cancel_Implementation(FGuid Id)
{
	const int32 Index = Queue.IndexOfByPredicate([Id](const FCraftingOrder& Order) { return Order.Id == Id; });
	if (Index == INDEX_NONE || !RefundOrder(Queue[Index]))
	{
		return;
	}
	Queue.RemoveAt(Index);
	// 대기 주문을 취소해도 현재 진행 중인 주문의 타이머는 초기화하지 않는다.
	if (Index == 0)
	{
		StartNextItem();
	}
}

void UCraftingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	if (!GetOwner()->HasAuthority() || Queue.IsEmpty() || !Inventory())
	{
		return;
	}
	if (!Queue[0].Item || GetServerTime() < Queue[0].FinishTime)
	{
		return;
	}
	// 한 개씩 지급한다. 드롭도 실패하면 주문을 보존하고 다음 틱에서 다시 시도한다.
	int32 Count = 1;
	if (!GiveOrDrop(Queue[0].Item, Count))
	{
		return;
	}
	if (--Queue[0].Remaining <= 0)
	{
		Queue.RemoveAt(0);
	}
	StartNextItem();
}

void UCraftingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 다른 플레이어에게는 제작 목록과 남은 시간을 전송할 필요가 없다.
	DOREPLIFETIME_CONDITION(UCraftingComponent, Queue, COND_OwnerOnly);
}
