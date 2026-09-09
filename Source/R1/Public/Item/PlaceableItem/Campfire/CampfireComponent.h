#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/PlaceableItem/Campfire/CampfireTypes.h"
#include "Item/ItemInstance.h"
#include "Component/InventoryComponent.h"
#include "CampfireComponent.generated.h"

class UCampfireConfigDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCampfireStateChanged);

// 모닥불의 서버 연소/요리, 슬롯 이동과 복제 상태를 담당하는 컴포넌트
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class R1_API UCampfireComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCampfireComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 슬롯 정보 확인
	UFUNCTION(BlueprintPure, Category = "Campfire")
	FItemInstance GetSlot(const FCampfireSlotRef& Slot) const;

	// 아이템을 올릴 수 있는 슬롯인지 확인
	UFUNCTION(BlueprintPure, Category = "Campfire")
	bool CanAcceptItem(const FCampfireSlotRef& Slot, const UItemDataBase* Item) const;

	// 굽기 진행도 확인
	UFUNCTION(BlueprintPure, Category = "Campfire")
	float GetCookingProgress() const;

	// 연료 연소도 확인
	UFUNCTION(BlueprintPure, Category = "Campfire")
	float GetFuelProgress() const;

	// 아이템 이동: 인벤토리 -> 모닥불
	bool MoveFromInventory(UInventoryComponent* Inventory, const FInventorySlotRef& From, const FCampfireSlotRef& To, int32 Count, bool bHalfSplit);
	// 아이템 이동: 모닥불 -> 인벤토리
	bool MoveToInventory(UInventoryComponent* Inventory, const FCampfireSlotRef& From, const FInventorySlotRef& To, int32 Count, bool bHalfSplit);
	// 아이템 이동 우클릭 조작: 인벤토리 -> 모닥불
	bool QuickMoveFromInventory(UInventoryComponent* Inventory, const FInventorySlotRef& From);
	// 아이템 이동 우클릭 조작: 모닥불 -> 인벤토리
	bool QuickMoveToInventory(UInventoryComponent* Inventory, const FCampfireSlotRef& From);

	// 모닥불 점화 상태 설정
	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void SetLit(bool bNewLit);

private:

	// 클라이언트에서 복제된 모닥불 상태를 수신하면 상태 변경 델리게이트 호출
	UFUNCTION()
	void OnRep_State();

	// 모닥불 연소/굽기 전용 Tick
	void TickCampfire();

	// 연료 소비
	bool PrepareFuel();

	// 굽기 진행도와 연결된 아이템이 실제로 조리 입력 슬롯에 있는 아이템과 일치하는지 확인 처리
	void SyncProgressItems();

	// 연료 부산물 또는 굽기 결과물을 넣을 산출 슬롯 찾기
	int32 FindOutputSlot(UItemDataBase* Item) const;

	// 산출 슬롯에 산출물 넣기
	bool AddOutput(UItemDataBase* Item);

	// 모닥불 상태 변경 처리
	void NotifyStateChanged();

	// 지정한 모닥불 슬롯의 포인터를 FindSlot으로 읽기 전용으로 받아와서 수정 가능한 포인터로 반환
	FItemInstance* FindMutableSlot(const FCampfireSlotRef& Slot);

	// 지정한 모닥불 슬롯을 읽기 전용 포인터로 반환
	const FItemInstance* FindSlot(const FCampfireSlotRef& Slot) const;

	// 연소 중인 연료의 연소 완료 처리
	bool CompleteCurrentFuel();

public:

	UPROPERTY(BlueprintAssignable, Category = "Campfire")
	FOnCampfireStateChanged OnCampfireStateChanged;	// 모닥불 상태 변화에 대한 델리게이트

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UCampfireConfigDataAsset> Config;	// 모닥불 허용 아이템 정보를 저장하는 데이터 에셋

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	FItemInstance InputSlot;	// 구워지는 아이템 슬롯

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	FItemInstance FuelSlot;		// 연료 아이템 슬롯

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	TArray<FItemInstance> OutputSlots;	// 연료 부산물과 조리 결과물이 공용으로 사용하는 산출  슬롯

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	bool bIsLit = false;		// 모닥불 점화 상태 저장

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	float RemainingFuelTime = 0.f;		// 현재 연소 중인 연료의 남은 연소 시간

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	float CurrentFuelDuration = 0.f;	// 현재 연소 중인 연료 하나의 전체 연소 시간

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Campfire")
	float CurrentCookingTime = 0.f;		// 현재 굽는 중인 아이템의 구워지는 중인 시간

private:

	// 굽기 진행도는 모닥불을 꺼도 유지
	// Input이 비거나 아이템 종류가 바뀌면 초기화
	UPROPERTY(Transient)
	TObjectPtr<UItemDataBase> ProgressCookingItem;

	// 시작 시 이미 차감한 연료의 완료 결과물
	// 슬롯에서 연료를 빼거나 바꿔도 연소가 완전히 완료될 때까지 보존
	// nullptr이면 연소 완료 시 부산물을 생성하지 않음
	UPROPERTY(Transient)
	TObjectPtr<UItemDataBase> PendingFuelOutputItem;

	FTimerHandle CampfireTimer;	// TickCampfire 갱신에 사용
	static constexpr float UpdateInterval = 0.1f;	// TickCampfire 갱신 주기
};
