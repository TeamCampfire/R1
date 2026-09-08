#include "Component/CraftingComponent.h"

#include "Character/ActionCharacter.h"
#include "Component/InventoryComponent.h"
#include "Component/StatComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Interface/HealthInterface.h"
#include "Item/ItemPickup.h"
#include "Item/PlaceableItem/Workbench.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UCraftingComponent::UCraftingComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Recipes.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Crafting recipes are missing. Assign Recipes on the owning Blueprint's CraftingComponent."));
	}
	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(CraftingTimer, this,
			&UCraftingComponent::TickCrafting, UpdateInterval, true);
	}
}

void UCraftingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(CraftingTimer);
	Super::EndPlay(EndPlayReason);
}

UInventoryComponent* UCraftingComponent::Inventory() const
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
}

bool UCraftingComponent::CanPlayerCraft() const
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	const AActionCharacter* Character = Controller ? Cast<AActionCharacter>(Controller->GetPawn()) : nullptr;
	const IHealthInterface* Health = Character ? Cast<IHealthInterface>(Character->GetStatComponent()) : nullptr;
	return Health && Health->IsAlive();
}

bool UCraftingComponent::CanUseWorkbench(AWorkbench* Bench) const
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	return IsValid(Bench) && Controller && Controller->GetPawn()
		&& IInteractableInterface::Execute_CanInteract(Bench, Controller->GetPawn());
}

float UCraftingComponent::GetServerTime() const
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

bool UCraftingComponent::CollectIngredientCosts(UItemDataBase* Item, TMap<UItemDataBase*, int32>& OutCosts) const
{
	OutCosts.Reset();
	if (!Item || Item->CraftingCost.IsEmpty() || !FMath::IsFinite(Item->CraftingSeconds) || Item->CraftingSeconds <= 0.f)
	{
		return false;
	}
	for (const FCraftIngredient& Cost : Item->CraftingCost)
	{
		UItemDataBase* Material = Cost.Item.LoadSynchronous();
		if (!Material || Cost.Amount <= 0) return false;
		int32& Total = OutCosts.FindOrAdd(Material);
		if (Total > MAX_int32 - Cost.Amount) return false;
		Total += Cost.Amount;
	}
	return true;
}

int32 UCraftingComponent::GetMaximum(UItemDataBase* Item, AWorkbench* Bench) const
{
	const UInventoryComponent* PlayerInventory = Inventory();
	if (!CanPlayerCraft() || !PlayerInventory || !Item) return 0;
	// 맨손 레시피도 작업대를 지정했다면 거리를 검사한다.
	if (Bench && !CanUseWorkbench(Bench)) return 0;
	if (Item->bRequiresWorkbench && !Bench) return 0;
	const UCraftingComponent* Target = Bench ? Bench->GetCraftingComponent() : this;
	if (!Target || !Target->Recipes.Contains(Item)) return 0;
	TMap<UItemDataBase*, int32> Required;
	if (!CollectIngredientCosts(Item, Required)) return 0;
	int32 Maximum = MAX_int32;
	for (const auto& Ingredient : Required)
	{
		Maximum = FMath::Min(Maximum, PlayerInventory->GetItemCount(Ingredient.Key) / Ingredient.Value);
	}
	return FMath::Max(0, Maximum);
}

bool UCraftingComponent::HasQueueSpace() const
{
	// 완료된 주문도 슬롯을 차지한다. 미회수 보관량이 무한히 늘어나는 것을 막는다.
	TSet<FGuid> Orders;
	for (const FCraftingOrder& Order : Queue) Orders.Add(Order.Id);
	for (const FCraftingOrder& Order : CompletedOrders) Orders.Add(Order.Id);
	return Orders.Num() < MaxQueueSize;
}

void UCraftingComponent::Server_Enqueue_Implementation(UItemDataBase* Item, int32 Count, AWorkbench* Bench)
{
	if (Count <= 0 || Count > GetMaximum(Item, Bench)) return;
	UCraftingComponent* Target = Bench ? Bench->GetCraftingComponent() : this;
	if (!Target || !Target->HasQueueSpace()) return;
	TMap<UItemDataBase*, int32> Required;
	if (!CollectIngredientCosts(Item, Required)) return;

	// 서버 게임 스레드에서 검사 후 선불 차감. 동일 재료는 합산했으므로 중복 예약되지 않는다.
	for (const auto& Ingredient : Required)
	{
		Inventory()->ConsumeItemCount(Ingredient.Key, Ingredient.Value * Count);
	}
	FCraftingOrder Order;
	Order.Id = FGuid::NewGuid();
	Order.Item = Item;
	Order.Remaining = Count;
	Order.Requester = Cast<APlayerController>(GetOwner());
	Target->Queue.Add(Order);
	if (Target->Queue.Num() == 1) Target->StartNextItem();
	Target->NotifyChanged();
}

void UCraftingComponent::StartNextItem()
{
	if (!Queue.IsEmpty() && Queue[0].Item)
	{
		Queue[0].FinishTime = GetServerTime() + FMath::Max(0.1f, Queue[0].Item->CraftingSeconds);
	}
}

bool UCraftingComponent::DropItem(UItemDataBase* Item, int32& Count) const
{
	if (!Item || Count <= 0) return Count <= 0;
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	const AActor* Origin = Controller ? Cast<AActor>(Controller->GetPawn()) : GetOwner();
	if (!Origin) return false;
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Location = Origin->GetActorLocation() + Origin->GetActorForwardVector() * 100.f + FVector(0, 0, 50.f);
	// 스택 불가 아이템도 월드 픽업 하나에 합칠 수 있다. 획득 시 AddItem이 슬롯으로 나눈다.
	AItemPickup* Pickup = GetWorld()->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), Location, FRotator::ZeroRotator, Parameters);
	if (!Pickup) return false;
	Pickup->InitializeFromItem(Item, Count);
	Count = 0;
	return true;
}

bool UCraftingComponent::GiveOrDrop(UItemDataBase* Item, int32& Count, APlayerController* Recipient)
{
	APawn* Pawn = IsValid(Recipient) ? Recipient->GetPawn() : nullptr;
	UInventoryComponent* PlayerInventory = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!PlayerInventory) return false;
	int32 Remainder = Count;
	PlayerInventory->AddItem(Item, Count, Remainder);
	Count = Remainder;
	return Count <= 0 || DropItem(Item, Count);
}

void UCraftingComponent::TickCrafting()
{
	if (Queue.IsEmpty() || !Queue[0].Item || GetServerTime() < Queue[0].FinishTime) return;
	AWorkbench* Bench = Cast<AWorkbench>(GetOwner());
	APlayerController* Recipient = Queue[0].Requester.Get();
	int32 Count = 1;
	if (!Bench)
	{
		// 사망/리스폰 중에는 개인 제작을 보존하고 지급 가능한 시점에 재시도한다.
		if (!CanPlayerCraft() || !GiveOrDrop(Queue[0].Item, Count, Recipient)) return;
	}
	else
	{
		const UCraftingComponent* Personal = Recipient ? Recipient->FindComponentByClass<UCraftingComponent>() : nullptr;
		if (Personal && Personal->CanPlayerCraft() && Personal->CanUseWorkbench(Bench))
		{
			GiveOrDrop(Queue[0].Item, Count, Recipient);
		}
		if (Count > 0)
		{
			FCraftingOrder* Completed = CompletedOrders.FindByPredicate(
				[this](const FCraftingOrder& Order) { return Order.Id == Queue[0].Id; });
			if (Completed) Completed->Remaining += Count;
			else
			{
				FCraftingOrder Output = Queue[0];
				Output.Remaining = Count;
				Output.FinishTime = 0.f;
				CompletedOrders.Add(Output);
			}
		}
	}
	if (--Queue[0].Remaining <= 0) Queue.RemoveAt(0);
	StartNextItem();
	NotifyChanged();
}

void UCraftingComponent::Server_CollectCompleted_Implementation(AWorkbench* Bench)
{
	if (!CanPlayerCraft() || !CanUseWorkbench(Bench)) return;
	Bench->GetCraftingComponent()->CollectCompleted(Cast<APlayerController>(GetOwner()));
}

void UCraftingComponent::CollectCompleted(APlayerController* Recipient)
{
	if (!GetOwner()->HasAuthority()) return;
	for (int32 Index = CompletedOrders.Num() - 1; Index >= 0; --Index)
	{
		FCraftingOrder& Order = CompletedOrders[Index];
		GiveOrDrop(Order.Item, Order.Remaining, Recipient);
		if (Order.Remaining <= 0) CompletedOrders.RemoveAt(Index);
	}
	NotifyChanged();
}

void UCraftingComponent::DropContents()
{
	if (!GetOwner()->HasAuthority()) return;
	for (FCraftingOrder& Order : CompletedOrders) DropItem(Order.Item, Order.Remaining);
	for (const FCraftingOrder& Order : Queue)
	{
		TMap<UItemDataBase*, int32> Costs;
		if (!CollectIngredientCosts(Order.Item, Costs)) continue;
		for (const auto& Cost : Costs)
		{
			int32 Count = Cost.Value * Order.Remaining;
			if (!DropItem(Cost.Key, Count))
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to drop crafting contents from %s"), *GetOwner()->GetName());
			}
		}
	}
	Queue.Reset();
	CompletedOrders.Reset();
}

void UCraftingComponent::NotifyChanged()
{
	GetOwner()->ForceNetUpdate();
	OnCraftingChanged.Broadcast();
}

void UCraftingComponent::OnRep_Crafting()
{
	OnCraftingChanged.Broadcast();
}

void UCraftingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 컨트롤러 자체는 소유 클라이언트에만 존재. 작업대는 relevancy 범위의 모두에게 공유.
	DOREPLIFETIME(UCraftingComponent, Queue);
	DOREPLIFETIME(UCraftingComponent, CompletedOrders);
}
