/// 최초작성 : 2026.09.06
/// 작 성 자 : 최 요 환
/// 간단설명 : 창고(Warehouse) 액터가 소지하는 저장 컴포넌트

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Component/InventoryComponent.h"
#include "Item/ItemInstance.h"
#include "WarehouseInventoryComponent.generated.h"

/**
 * 창고 액터가 소지하는 저장 컴포넌트.
 *
 * UInventoryComponent(Main/Belt/Equipment 3풀, COND_OwnerOnly 리플리케이션)를 그대로 쓰지 않고
 * 별도 클래스로 뺀 이유: 창고는 레벨에 배치되는 액터라 특정 플레이어를 네트워크 Owner로 갖지
 * 않는다. COND_OwnerOnly로는 아무 클라이언트에도 슬롯이 리플리케이트되지 않으므로, 창고 슬롯은
 * 처음부터 전체 공개(COND_None)로 리플리케이트한다 — 누구나 조준해서 열면 내용물이 보여야 하므로
 * 이 편이 의도에도 맞는다.
 *
 * 플레이어 인벤토리(UInventoryComponent)와의 이동은 이 컴포넌트가 아니라 UInventoryComponent
 * 쪽에 있는 TransferWithWarehouse()/Server_TransferWithWarehouse()가 담당한다 — Server RPC는
 * 호출하는 클라이언트가 네트워크 Owner인 액터의 컴포넌트에서만 실행되므로(GetNetConnection()이
 * Owner 체인을 타고 올라가 PlayerController를 찾아야 함), Owner가 없는 창고 액터의 컴포넌트에
 * Server RPC를 두면 어떤 클라이언트도 호출할 수 없다. 그래서 이동 요청 RPC는 항상 "요청하는
 * 플레이어가 실제로 소유한" 쪽 컴포넌트, 즉 플레이어의 UInventoryComponent에 둔다.
 */

class UItemDataBase;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class R1_API UWarehouseInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWarehouseInventoryComponent();

	// ItemData/Count를 StorageSlots에 넣는다. UInventoryComponent::AddItem과 동일한 규칙
	// (기존 스택에 먼저 채우고 남으면 빈 슬롯에) — 다 못 넣으면 남은 수량을 OutRemainder로 돌려준다.
	UFUNCTION(BlueprintCallable, Category = "Warehouse")
	bool AddItem(UItemDataBase* ItemData, int32 Count, int32& OutRemainder);

	// 외부(주로 UInventoryComponent::TransferWithWarehouse)가 서버 권위 하에 창고 슬롯 하나를
	// 직접 갱신할 때 쓰는 공개 진입점. GetOwner()->HasAuthority()가 아니면 무시된다.
	UFUNCTION(BlueprintCallable, Category = "Warehouse")
	void SetSlotItem(int32 Index, const FItemInstance& NewValue);

	// 슬롯 하나를 "선택"(클릭) 상태로 표시한다 — UInventoryComponent::SelectSlot과 동일한 용도(정보
	// 패널 표시용). 실제 이동과는 무관한, 서버로 리플리케이트되지 않는 순수 로컬 상태다.
	UFUNCTION(BlueprintCallable, Category = "Warehouse")
	void SelectSlot(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Warehouse")
	void ClearSelection();

	UFUNCTION(BlueprintPure, Category = "Warehouse")
	bool IsSlotSelected(int32 Index) const { return bHasSelection && SelectedIndex == Index; }

	// 현재 선택된 슬롯의 아이템 정보. 선택이 없거나 그 슬롯이 비어있으면 무효(FItemInstance())를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Warehouse")
	FItemInstance GetSelectedItemInstance() const;

protected:
	virtual void BeginPlay() override;

	//~ Begin UActorComponent Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent Interface

private:
	UFUNCTION()
	void OnRep_StorageSlots();

public:
	// 창고 슬롯 개수.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Warehouse")
	int32 StorageSlotCount = 30;

	// 이 창고에 상호작용 가능한 최대 거리(cm) — Server_TransferWithWarehouse의 서버측 검증에서 쓴다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Warehouse")
	float MaxInteractDistance = 400.f;

	// 창고 저장 슬롯 — 누구나 열어볼 수 있어야 하므로 COND_None(전체 공개)로 리플리케이트한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_StorageSlots, Category = "Warehouse")
	TArray<FItemInstance> StorageSlots;

	// 슬롯 변경 감지 델리게이트 — UInventoryComponent::OnInventoryChanged와 동일한 용도.
	UPROPERTY(BlueprintAssignable, Category = "Warehouse")
	FOnInventoryChanged OnInventoryChanged;

	// 현재 선택(클릭)된 슬롯이 있는지. SelectSlot/ClearSelection으로만 바뀐다.
	UPROPERTY(BlueprintReadOnly, Category = "Warehouse")
	bool bHasSelection = false;

	// 선택된 슬롯 인덱스. bHasSelection이 false면 의미 없음.
	UPROPERTY(BlueprintReadOnly, Category = "Warehouse")
	int32 SelectedIndex = INDEX_NONE;

	// 선택 변경 감지 델리게이트(슬롯 내용물이 아니라 "무엇을 선택했는지"만 바뀔 때) —
	// UDetailInfoWidget이 창고 선택도 같이 보여주기 위해 구독한다.
	UPROPERTY(BlueprintAssignable, Category = "Warehouse")
	FOnInventorySelectionChanged OnSelectionChanged;
};
