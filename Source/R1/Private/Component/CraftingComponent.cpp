#include "Component/CraftingComponent.h"

#include "Character/ActionCharacter.h"
#include "Component/InventoryComponent.h"
#include "Component/StatComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "Data/Crafting/CraftingRecipeCatalog.h"
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

const TArray<TObjectPtr<UItemDataBase>>& UCraftingComponent::GetRecipes() const
{
	// 아이템이 제작대 BP를 참조할 수 있으므로 생성자에서는 목록을 로드하지 않음
	if (!RecipeCatalog)
		const_cast<UCraftingComponent*>(this)->RecipeCatalog = CatalogAsset.LoadSynchronous();

	static const TArray<TObjectPtr<UItemDataBase>> Empty;
	return RecipeCatalog ? RecipeCatalog->Recipes : Empty;
}

bool UCraftingComponent::IsRecipeAvailable(UItemDataBase* Item, bool bWorkbenchMode) const
{
	return Item && GetRecipes().Contains(Item) && (!Item->bRequiresWorkbench || bWorkbenchMode);
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetRecipes().IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Crafting recipes are missing. DA_CraftingRecipes."));
	}

	// 서버에서 제작 Tick 타이머 시작
	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(CraftingTimer, this, &UCraftingComponent::TickCrafting, UpdateInterval, true);
	}
}

void UCraftingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머 해제
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(CraftingTimer);

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
	return IsValid(Bench) && Controller && Controller->GetPawn() && IInteractableInterface::Execute_CanInteract(Bench, Controller->GetPawn());
}

float UCraftingComponent::GetServerTime() const
{
	// 네트워크에서 일관된 완료 시각을 계산하도록 GameState의 동기화된 서버 시간을 우선 사용
	// GameState가 아직 없으면 현재 World 시간을 대신 사용

	const AGameStateBase* GameState = GetWorld()->GetGameState();

	return GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

// 중복 재료를 합산하고 잘못된 수량과 제작 시간을 걸러 최대 제작량 및 서버 차감에 같은 비용을 사용
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

		if (!Material || Cost.Amount <= 0)
			return false;

		int32& Total = OutCosts.FindOrAdd(Material);
		if (Total > MAX_int32 - Cost.Amount)
			return false;

		Total += Cost.Amount;
	}
	return true;
}

int32 UCraftingComponent::GetMaximum(UItemDataBase* Item, AWorkbench* Bench) const
{
	const UInventoryComponent* PlayerInventory = Inventory();
	if (!CanPlayerCraft() || !PlayerInventory || !Item)
		return 0;

	// 맨손 레시피도 작업대를 지정했다면 거리 검사
	if (Bench && !CanUseWorkbench(Bench))
		return 0;

	if (!IsRecipeAvailable(Item, Bench != nullptr))
		return 0;

	if (Bench && !Bench->GetCraftingComponent())
		return 0;

	TMap<UItemDataBase*, int32> Required;
	if (!CollectIngredientCosts(Item, Required))
		return 0;

	int32 Maximum = MAX_int32;	// 가지고 있는 아이템으로 최대 제작 가능 수량

	// 보유한 재료들 수를 필요한 재료들 수로 나눈 것의 최솟값과 한 번에 만들 수 있는 최대 아이템 수 중 작은 값
	for (const auto& Ingredient : Required)
	{
		Maximum = FMath::Min(Maximum, PlayerInventory->GetItemCount(Ingredient.Key) / Ingredient.Value);
	}

	return FMath::Max(0, Maximum);
}

bool UCraftingComponent::HasQueueSpace() const
{
	// 완료된 주문도 슬롯 차지: 미회수 보관량이 무한히 늘어나는 것 방지
	TSet<FGuid> Orders;

	for (const FCraftingOrder& Order : Queue)
		Orders.Add(Order.Id);

	for (const FCraftingOrder& Order : CompletedOrders)
		Orders.Add(Order.Id);

	return Orders.Num() < MaxQueueSize;
}

// 서버가 레시피, 재료, 작업대 거리와 큐 여유를 재검사한 후 재료를 선불 차감하고 주문을 등록
void UCraftingComponent::Server_Enqueue_Implementation(UItemDataBase* Item, int32 Count, AWorkbench* Bench)
{
	if (Count <= 0 || Count > GetMaximum(Item, Bench))
		return;

	UCraftingComponent* Target = Bench ? Bench->GetCraftingComponent() : this;
	if (!Target || !Target->HasQueueSpace())
		return;

	TMap<UItemDataBase*, int32> Required;
	if (!CollectIngredientCosts(Item, Required))
		return;

	// 서버 게임 스레드에서 검사 후 선불 차감
	// 동일 재료는 합산되므로 중복 예약되지 않음
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

	if (Target->Queue.Num() == 1)
		Target->StartNextItem();

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
	if (!Item || Count <= 0)
		return Count <= 0;

	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	const AActor* Origin = Controller ? Cast<AActor>(Controller->GetPawn()) : GetOwner();
	if (!Origin)
		return false;

	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Location = Origin->GetActorLocation() + Origin->GetActorForwardVector() * 100.f + FVector(0, 0, 50.f);

	// 스택 불가 아이템도 월드 픽업 하나에 합칠 수 있으니 한번에 스폰 (획득 시 AddItem이 슬롯으로 나눔)
	AItemPickup* Pickup = GetWorld()->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), Location, FRotator::ZeroRotator, Parameters);
	if (!Pickup) return
		false;
	Pickup->InitializeFromItem(Item, Count);
	Count = 0;

	return true;
}

bool UCraftingComponent::GiveOrDrop(UItemDataBase* Item, int32& Count, APlayerController* Recipient)
{
	APawn* Pawn = IsValid(Recipient) ? Recipient->GetPawn() : nullptr;
	UInventoryComponent* PlayerInventory = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!PlayerInventory)
		return false;

	int32 Remainder = Count;
	PlayerInventory->AddItem(Item, Count, Remainder);

	Count = Remainder;
	return Count <= 0 || DropItem(Item, Count);
}

// 제작 큐의 첫 번째 주문에서 아이템 1개를 완성 처리
// - 개인 제작: 플레이어 인벤토리에 지급, 남은 수량은 월드에 드롭
// - 작업대 제작: 주문자가 근처에 있으면 즉시 지급, 지급하지 못한 수량은 작업대의 완료 목록에 보관
void UCraftingComponent::TickCrafting()
{
	// 처리할 주문이 없거나 유효한 아이템이 없으면 종료
	if (Queue.IsEmpty() || !Queue[0].Item)
		return;

	// 첫 번째 주문의 아이템 1개가 아직 완성되지 않았으면 대기
	if (GetServerTime() < Queue[0].FinishTime)
		return;

	AWorkbench* Bench = Cast<AWorkbench>(GetOwner());			// 작업대
	APlayerController* Recipient = Queue[0].Requester.Get();	// 주문자
	int32 Count = 1;	// 이번 Tick에서 완성할 아이템 수량

	if (!Bench)
	{
		// 개인 제작

		// 사망/리스폰 중에는 개인 제작을 보존하고 제작 완료 처리 보류
		// 인벤토리에 들어가지 않는 수량은 플레이어 주변에 드롭
		if (!CanPlayerCraft() || !GiveOrDrop(Queue[0].Item, Count, Recipient))
			return;
	}
	else
	{
		// 제작대
		// 주문자의 개인 제작 컴포넌트를 찾아 생존 여부와 작업대 사용 가능 거리 검사

		const UCraftingComponent* Personal = Recipient ? Recipient->FindComponentByClass<UCraftingComponent>() : nullptr;

		// 주문자가 살아있고, 제작대 근처에 있으면 주문자에게 즉시 지급
		if (Personal && Personal->CanPlayerCraft() && Personal->CanUseWorkbench(Bench))
		{
			GiveOrDrop(Queue[0].Item, Count, Recipient);
		}

		// 주문자에게 지급되지 않은 수량은 작업대의 회수 대기 목록에 보관
		if (Count > 0)
		{
			// 같은 제작 주문에서 이미 완성된 항목이 있는지 찾기
			FCraftingOrder* Completed = CompletedOrders.FindByPredicate(
				[this](const FCraftingOrder& Order)
				{
					return Order.Id == Queue[0].Id;
				}
			);

			if (Completed)
			{
				// 같은 주문에 완성된 주문이 있으면 이미 있는 회수 대기 목록 증가
				Completed->Remaining += Count;
			}
			else
			{
				// 첫 완성품이면 회수 대기 목록에 새로 추가
				FCraftingOrder Output = Queue[0];
				Output.Remaining = Count;
				Output.FinishTime = 0.f;
				CompletedOrders.Add(Output);
			}
		}
	}

	// 현재 주문의 남은 제작 수량을 감소시키고, 모두 완성됐으면 제거
	if (--Queue[0].Remaining <= 0)
		Queue.RemoveAt(0);

	StartNextItem();	// 다음 아이템의 완성 예정 시각 설정
	NotifyChanged();	// 변경 사실 알리기
}

// 지정한 작업대의 제작 완료품을 플레이어가 회수하도록 서버에 요청
void UCraftingComponent::Server_CollectCompleted_Implementation(AWorkbench* Bench)
{
	// 플레이어가 제작 기능을 사용할 수 없거나 해당 작업대와 상호작용할 수 없는 상태면 회수 요청 거부
	if (!CanPlayerCraft() || !CanUseWorkbench(Bench))
		return;

	UCraftingComponent* BenchCraftingComp = Bench->GetCraftingComponent();

	// 작업대에 제작 컴포넌트가 없으면 회수 불가
	if (BenchCraftingComp)
		return;

	// 작업대의 제작 컴포넌트에 보관된 완료품을 요청한 플레이어에게 지급
	BenchCraftingComp->CollectCompleted(Cast<APlayerController>(GetOwner()));
}

// 완료 목록에 보관된 아이템을 지정한 플레이어에게 지급
void UCraftingComponent::CollectCompleted(APlayerController* Recipient)
{
	// 제작 상태와 아이템 지급을 서버에서만 처리
	if (!GetOwner()->HasAuthority())
		return;

	// 뒤에서부터 순회하여 항목 삭제로 인한 배열 인덱스 변경 방지
	for (int32 Index = CompletedOrders.Num() - 1; Index >= 0; --Index)
	{
		FCraftingOrder& Order = CompletedOrders[Index];

		// 플레이어 인벤토리에 지급하고, 들어가지 않는 수량은 월드에 드롭
		// 지급 또는 드롭된 수량만큼 Order.Remaining 감소
		GiveOrDrop(Order.Item, Order.Remaining, Recipient);

		// 모든 수량이 처리된 완료 주문은 목록에서 제거
		if (Order.Remaining <= 0)
			CompletedOrders.RemoveAt(Index);
	}

	// 완료 목록 변경 사실을 서버와 UI에 알림
	NotifyChanged();
}

// 작업대 파괴 시 완료 아이템과 아직 제작하지 않은 주문의 재료를 월드에 떨어뜨리기
void UCraftingComponent::DropContents()
{
	if (!GetOwner()->HasAuthority())
		return;

	for (FCraftingOrder& Order : CompletedOrders)
		DropItem(Order.Item, Order.Remaining);

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

	// 컨트롤러 자체는 소유 클라이언트에만 존재
	// 작업대는 서버와 연결된 클라이언트에게 공유.
	DOREPLIFETIME(UCraftingComponent, Queue);
	DOREPLIFETIME(UCraftingComponent, CompletedOrders);
}
