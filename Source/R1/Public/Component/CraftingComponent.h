#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"

class UItemDataBase;
class UInventoryComponent;
class AWorkbench;
class AItemPickup;

/** 한 번의 제작 요청. Remaining은 아직 지급하지 않은 결과물 개수다. */
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

	// 선두 주문의 다음 결과물 완료 시각(서버 시간). 대기 주문은 0이다.
	UPROPERTY(BlueprintReadOnly)
	float FinishTime = 0.f;
};

/** 컨트롤러가 소유하는 서버 권위 제작 대기열. UI를 닫아도 제작은 계속된다. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class R1_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	static constexpr int32 MaxQueueSize = 64;

	UCraftingComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting")
	TArray<TObjectPtr<UItemDataBase>> Recipes;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Crafting")
	TArray<FCraftingOrder> Queue;

	// 동일 재료를 합산한 뒤 min(보유량 / 개당 필요량)을 구한다. 잘못된 레시피는 0이다.
	UFUNCTION(BlueprintPure, Category="Crafting")
	int32 GetMaximum(UItemDataBase* Item, AWorkbench* Bench) const;

	// 클라이언트 수량을 신뢰하지 않고 서버에서 재료와 작업대 조건을 다시 검사한다.
	UFUNCTION(Server, Reliable)
	void Server_Enqueue(UItemDataBase* Item, int32 Count, AWorkbench* Bench);

	// 미완료 수량의 재료만 반환한다. 이미 지급한 결과물은 취소 대상이 아니다.
	UFUNCTION(Server, Reliable)
	void Server_Cancel(FGuid Id);

	float GetServerTime() const;
	UInventoryComponent* Inventory() const;

private:
	void LoadDefaultRecipes();
	void StartNextItem();
	AItemPickup* SpawnOverflowPickup() const;
	bool GiveOrDrop(UItemDataBase* Item, int32& Count);
	bool RefundOrder(const FCraftingOrder& Order);
};
