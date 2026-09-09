#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"

class UItemDataBase;
class UInventoryComponent;
class AWorkbench;
class APlayerController;

USTRUCT(BlueprintType)
struct FCraftingOrder
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid Id;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UItemDataBase> Item = nullptr;

	UPROPERTY(BlueprintReadOnly)
	int32 Remaining = 0;

	// 선두 주문의 다음 한 개가 완료되는 서버 시각. 대기 주문은 0.
	UPROPERTY(BlueprintReadOnly)
	float FinishTime = 0.f;

	// 지급 대상은 서버만 사용한다. 작업대의 네트워크 Owner는 바꾸지 않는다.
	UPROPERTY(NotReplicated)
	TWeakObjectPtr<APlayerController> Requester;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftingChanged);

// 컨트롤러에 붙으면 개인 큐, 작업대에 붙으면 공유 큐. 진행 로직은 같은 컴포넌트를 사용한다.
// [wdk59] 개인 또는 작업대 소유의 제작 큐, 재료 검증 및 결과물 지급·보관을 담당한다.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class R1_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxQueueSize = 64;
	UCraftingComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	TArray<TObjectPtr<UItemDataBase>> Recipes;

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingChanged OnCraftingChanged;

	const TArray<FCraftingOrder>& GetQueue() const { return Queue; }
	const TArray<FCraftingOrder>& GetCompletedOrders() const { return CompletedOrders; }
	bool HasQueueSpace() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	int32 GetMaximum(UItemDataBase* Item, AWorkbench* Bench) const;

	// 반드시 플레이어 소유 컴포넌트를 통해 요청한다.
	UFUNCTION(Server, Reliable)
	void Server_Enqueue(UItemDataBase* Item, int32 Count, AWorkbench* Bench);

	UFUNCTION(Server, Reliable)
	void Server_CollectCompleted(AWorkbench* Bench);

	float GetServerTime() const;
	UInventoryComponent* Inventory() const;
	bool CollectIngredientCosts(UItemDataBase* Item, TMap<UItemDataBase*, int32>& OutCosts) const;

	// 파괴된 작업대의 미회수 결과물과 미완료 재료를 월드에 반환한다.
	void DropContents();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(ReplicatedUsing = OnRep_Crafting, BlueprintReadOnly, Category = "Crafting")
	TArray<FCraftingOrder> Queue;

	UPROPERTY(ReplicatedUsing = OnRep_Crafting, BlueprintReadOnly, Category = "Crafting")
	TArray<FCraftingOrder> CompletedOrders;

	UPROPERTY(EditDefaultsOnly, Category = "Crafting", meta = (ClampMin = "0.05"))
	float UpdateInterval = 0.1f;

private:
	void TickCrafting();
	void StartNextItem();
	void NotifyChanged();
	bool CanUseWorkbench(AWorkbench* Bench) const;
	bool CanPlayerCraft() const;
	bool GiveOrDrop(UItemDataBase* Item, int32& Count, APlayerController* Recipient);
	bool DropItem(UItemDataBase* Item, int32& Count) const;
	void CollectCompleted(APlayerController* Recipient);

	UFUNCTION()
	void OnRep_Crafting();

	FTimerHandle CraftingTimer;
};
