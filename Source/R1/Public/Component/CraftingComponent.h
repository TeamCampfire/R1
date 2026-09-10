#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"

class UItemDataBase;
class UInventoryComponent;
class AWorkbench;
class APlayerController;
class UCraftingRecipeCatalog;

// 제작 주문 구조체
USTRUCT(BlueprintType)
struct FCraftingOrder
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid Id;	// 개별 제작 주문 식별자

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UItemDataBase> Item = nullptr;	// 제작 아이템

	UPROPERTY(BlueprintReadOnly)
	int32 Remaining = 0;	// 아직 제작이 완료되지 않은 아이템 수량

	// 선두 주문의 다음 한 개가 완료되는 서버 시각
	// 대기 주문은 0
	UPROPERTY(BlueprintReadOnly)
	float FinishTime = 0.f;

	// 지급 대상은 서버가 관리 (리플리케이트 제외)
	// 작업대의 네트워크 Owner는 바꾸지 않음
	UPROPERTY(NotReplicated)
	TWeakObjectPtr<APlayerController> Requester;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftingChanged);

// 개인(컨트롤러) 또는 작업대 소유의 제작 큐
// 재료 검증 및 결과물 지급, 보관을 담당
// 진행 로직은 같은 컴포넌트를 사용
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class R1_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCraftingComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// BP마다 목록을 따로 설정하지 않고 공통 카탈로그의 제작 레시피를 반환
	const TArray<TObjectPtr<UItemDataBase>>& GetRecipes() const;

	// 화면 표시와 서버 요청 검증에서 같은 등록 여부와 제작대 조건 확인
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool IsRecipeAvailable(UItemDataBase* Item, bool bWorkbenchMode) const;

	// 현재 진행 중이거나 대기 중인 제작 주문 반환
	const TArray<FCraftingOrder>& GetQueue() const { return Queue; }

	// 작업대에서 제작을 마치고 회수를 기다리는 주문 반환
	const TArray<FCraftingOrder>& GetCompletedOrders() const { return CompletedOrders; }

	// 진행 중인 주문과 회수 대기 주문을 합쳐 새 주문을 받을 자리가 있는지 확인
	bool HasQueueSpace() const;

	// 현재 보유 재료와 제작 조건을 기준으로 선택 아이템의 최대 제작 가능 수량 반환
	UFUNCTION(BlueprintPure, Category = "Crafting")
	int32 GetMaximum(UItemDataBase* Item, AWorkbench* Bench) const;

	// 플레이어 소유 컴포넌트를 통해 제작 주문을 서버에 요청
	UFUNCTION(Server, Reliable)
	void Server_Enqueue(UItemDataBase* Item, int32 Count, AWorkbench* Bench);

	// 작업대에서 제작 완료된 아이템을 플레이어가 회수하도록 서버에 요청하는 RPC
	UFUNCTION(Server, Reliable)
	void Server_CollectCompleted(AWorkbench* Bench);

	// 파괴된 작업대의 미회수 결과물과 미완료 재료를 월드에 반환
	void DropContents();

protected:

	// 타이머 시작
	virtual void BeginPlay() override;

	// 타이머 해제
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private :

	void TickCrafting();
	void StartNextItem();
	void NotifyChanged();

	bool CanUseWorkbench(AWorkbench* Bench) const;
	bool CanPlayerCraft() const;

	bool GiveOrDrop(UItemDataBase* Item, int32& Count, APlayerController* Recipient);
	bool DropItem(UItemDataBase* Item, int32& Count) const;
	void CollectCompleted(APlayerController* Recipient);

	// 서버에서 변경된 제작 상태가 클라이언트로 복제됐을 때 호출
	// 제작 상태 사용 객체 갱신
	UFUNCTION()
	void OnRep_Crafting();

public :

	static constexpr int32 MaxQueueSize = 64;	// 진행 큐와 완료 목록을 합친 최대 주문 수

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingChanged OnCraftingChanged;	// 제작 큐나 완료 목록 변경 시 발동

	float GetServerTime() const;
	UInventoryComponent* Inventory() const;
	bool CollectIngredientCosts(UItemDataBase* Item, TMap<UItemDataBase*, int32>& OutCosts) const;

	// 공통 제작 레시피 카탈로그를 필요할 때 로드하기 위한 소프트 참조
	// 생성자에서 즉시 로드하지 않아 CDO 생성 중 순환 로딩을 피함
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Crafting")
	TSoftObjectPtr<UCraftingRecipeCatalog> CatalogAsset;

protected :

	UPROPERTY(ReplicatedUsing = OnRep_Crafting, BlueprintReadOnly, Category = "Crafting")
	TArray<FCraftingOrder> Queue;	// 제작 큐 (배열로 구현)

	UPROPERTY(ReplicatedUsing = OnRep_Crafting, BlueprintReadOnly, Category = "Crafting")
	TArray<FCraftingOrder> CompletedOrders;	// 작업대에서 회수를 기다리는 완료 주문

	UPROPERTY(EditDefaultsOnly, Category = "Crafting", meta = (ClampMin = "0.05"))
	float UpdateInterval = 0.1f;	// 서버에서 제작 완료 여부를 검사하는 주기

private:

	// CatalogAsset에서 로드한 공통 레시피 카탈로그의 런타임 캐시
	UPROPERTY(Transient)
	TObjectPtr<UCraftingRecipeCatalog> RecipeCatalog;

	FTimerHandle CraftingTimer;
};
