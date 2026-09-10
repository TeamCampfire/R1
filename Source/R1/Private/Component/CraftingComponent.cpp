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

		// Total이 정수 범위를 벗어나게 되면 실패 처리
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
	// 서버가 아니거나 제작큐가 비었거나 제작큐의 첫번째 아이템이 없을 때 종료
	if (!GetOwner()->HasAuthority() || Queue.IsEmpty() || !Queue[0].Item)
		return;

	// 제작 완료 시간 알아오기
	const float Duration = FMath::Max(0.1f, Queue[0].Item->CraftingSeconds);
	Queue[0].FinishTime = GetServerTime() + Duration;

	// Tick 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(CraftingTimer, this, &UCraftingComponent::TickCrafting, Duration, false);
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
	// 제작 중인 아이템의 제작 타이머가 끝나는 순간에 실행

	// 처리할 주문이 없거나 유효한 아이템이 없으면 종료
	if (Queue.IsEmpty() || !Queue[0].Item)
		return;

	AWorkbench* Bench = Cast<AWorkbench>(GetOwner());			// 작업대
	APlayerController* Recipient = Queue[0].Requester.Get();	// 주문자
	int32 Count = 1;	// 이번 Tick에서 완성할 아이템 수량

	if (!Bench)
	{
		// 개인 제작

		if (CanPlayerCraft())
		{
			// 살아 있으면 인벤토리에 지급하고, 남은 수량은 플레이어 주변에 드롭
			if (!GiveOrDrop(Queue[0].Item, Count, Recipient))
			{
				// 지급이나 드롭에 실패한 경우에만 0.1초 뒤에 다시 시도
				GetWorld()->GetTimerManager().SetTimer(
					CraftingTimer,
					this,
					&UCraftingComponent::TickCrafting,
					UpdateInterval,
					false
				);

				return;
			}
		}
		else
		{
			// 사망/리스폰 상태라면 인벤토리를 거치지 않고 바로 드롭
			if (!DropItem(Queue[0].Item, Count))
			{
				// 리스폰 중 Pawn이 없거나 픽업 생성에 실패한 경우 0.1초 뒤에 재시도
				GetWorld()->GetTimerManager().SetTimer(
					CraftingTimer,
					this,
					&UCraftingComponent::TickCrafting,
					UpdateInterval,
					false
				);

				return;
			}
		}
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
	if (!BenchCraftingComp)
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

// 제작 큐에 들어간 주문 취소
void UCraftingComponent::Server_CancelOrder_Implementation(const FGuid& OrderId, AWorkbench* Bench)
{
	// 제작 기능 사용 불가 상태일 때는 실행하지 않음
	if (!CanPlayerCraft())
		return;

	// 작업대 주문: 거리와 작업대 유효성 검사
	if (Bench && !CanUseWorkbench(Bench))
		return;

	// 작업대 주문인 경우에는 작업대의 제작 컴포넌트 사용
	// 작업대 주문이 아니면 플레이어 컨트롤러의 제작 컴포넌트 사용
	UCraftingComponent* TargetComp = Bench ? Bench->GetCraftingComponent() : this;
	if (!TargetComp)
		return;

	// Queue 원소 순회하면서 람다 조건이 처음으로 참이 되는 원소의 인덱스 반환
	// 일치하는 주문 없으면 INDEX_NONE(-1) 반환
	const int32 OrderIndex = TargetComp->Queue.IndexOfByPredicate(
		[&OrderId](const FCraftingOrder& Order)
		{
			return Order.Id == OrderId;
		}
	);

	if (OrderIndex == INDEX_NONE)
		return;

	FCraftingOrder& Order = TargetComp->Queue[OrderIndex];

	// 작업대의 공유 큐에서는 다른 플레이어의 주문을 취소하지 못하게 검사
	APlayerController* RequestingController = Cast<APlayerController>(GetOwner());

	// 작업대의 공유 큐에서는 자신이 요청한 주문만 취소 가능
	if (!RequestingController || (Bench && Order.Requester.Get() != RequestingController))
		return;

	// 주문 아이템이 유효하지 않거나, 남은 제작 수량이 없으면 종료
	if (!Order.Item || Order.Remaining <= 0)
		return;

	// 제작 중인 주문의 레시피, 주문 수량 찾기
	TMap<UItemDataBase*, int32> IngredientCosts;
	if (!CollectIngredientCosts(Order.Item, IngredientCosts))
		return;

	/* 아직 완성되지 않은 아이템 수량만큼 재료 반환 */
	// 모든 반환 대상 아이템에 대해 반환할 수량이 없거나, 반환해야 하는 수량이 int32의 범위를 벗어나면 실패 처리
	for (const TPair<UItemDataBase*, int32>& Ingredient : IngredientCosts)
	{
		const int64 RefundCount64 = static_cast<int64>(Ingredient.Value) * static_cast<int64>(Order.Remaining);

		// 반환할 재료가 없거나 반환해야 하는 개수가 int32의 범위를 넘어가면 반환 불가 처리
		if (RefundCount64 <= 0 || RefundCount64 > MAX_int32)
			return;
	}

	UInventoryComponent* PlayerInventory = Inventory();
	APawn* RequestingPawn = RequestingController->GetPawn();

	UWorld* World = GetWorld();
	const AActor* DropOrigin = Bench ? Cast<AActor>(Bench) : Cast<AActor>(RequestingPawn);	// 인벤토리에 들어가지 않고 남은 아이템들을 드롭할 위치의 기준이 되는 액터 기억

	if (!PlayerInventory || !World || !DropOrigin)
		return;

	struct FPreparedRefund
	{
		UItemDataBase* Item = nullptr;
		int32 Count = 0;
		AItemPickup* DeferredPickup = nullptr;
		FTransform SpawnTransform;
	};

	// 반환 대상 기억용 배열
	TArray<FPreparedRefund> PreparedRefunds;
	PreparedRefunds.Reserve(IngredientCosts.Num());

	// 인벤토리에 들어가지 않고 남은 아이템을 떨어뜨릴 위치
	const FVector BaseLocation = DropOrigin->GetActorLocation()
		+ DropOrigin->GetActorForwardVector() * 100.f
		+ FVector(0.f, 0.f, 50.f);

	// 실제 인벤토리를 변경하기 전에 모든 재료의 드롭 액터를 지연 스폰 상태로 확보
	for (const TPair<UItemDataBase*, int32>& Ingredient : IngredientCosts)
	{
		FPreparedRefund Prepared;
		Prepared.Item = Ingredient.Key;
		Prepared.Count = static_cast<int32>(static_cast<int64>(Ingredient.Value) * static_cast<int64>(Order.Remaining));

		// 여러 종류의 재료 픽업이 정확히 같은 위치에 겹쳐서 생성되지 않도록 분산
		const float Angle = PreparedRefunds.Num() * (2.f * PI / FMath::Max(1, IngredientCosts.Num()));
		const FVector Offset(FMath::Cos(Angle) * 40.f, FMath::Sin(Angle) * 40.f, 0.f);

		Prepared.SpawnTransform = FTransform(FRotator::ZeroRotator, BaseLocation + Offset);
		Prepared.DeferredPickup = World->SpawnActorDeferred<AItemPickup>(
			AItemPickup::StaticClass(),
			Prepared.SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		// 반환 예정 드롭 액터의 사전 생성이 하나라도 실패하면 앞서 준비한 모든 반환 예정 드롭 액터를 제거하고 취소 처리 중단
		if (!Prepared.DeferredPickup)
		{
			for (FPreparedRefund& Existing : PreparedRefunds)
			{
				if (Existing.DeferredPickup)
					Existing.DeferredPickup->Destroy();
			}
			return;
		}

		PreparedRefunds.Add(Prepared);
	}

	// 드롭 액터가 모두 준비된 뒤 인벤토리에 반환하고, 남은 수량만 월드 픽업으로 완성
	// 아이템 종류별로 각각 실행
	for (FPreparedRefund& Prepared : PreparedRefunds)
	{
		int32 RemainingRefund = Prepared.Count;

		// 인벤토리에 반환
		// InventoryComponent::AddItem()에서 RemainingRefund를 참조하여 인벤토리에 넣고 남은 수량으로 변경시켜줌
		PlayerInventory->AddItem(Prepared.Item, Prepared.Count, RemainingRefund);

		if (RemainingRefund > 0)
		{
			// 인벤토리에 안 들어가고 남은 수량 처리
			Prepared.DeferredPickup->InitializeFromItem(Prepared.Item, RemainingRefund);
			Prepared.DeferredPickup->FinishSpawning(Prepared.SpawnTransform);
		}
		else
		{
			// 모든 재료가 인벤토리에 들어갔으면 준비해둔 드롭 액터 제거
			Prepared.DeferredPickup->Destroy();
		}
	}

	// 제작 취소한 주문을 Crafting Component에서 제거
	TargetComp->Queue.RemoveAt(OrderIndex);

	// 현재 제작 중이던 주문을 취소했다면 다음 주문 제작 시작
	// 현재 제작 중이던 주문의 OrderIndex는 항상 0 (제작 큐는 Queue 방식으로 작동하기 때문)
	const bool bCanceledActiveOrder = OrderIndex == 0;
	if (bCanceledActiveOrder)
	{
		TargetComp->StartNextItem();
	}

	TargetComp->NotifyChanged();
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
