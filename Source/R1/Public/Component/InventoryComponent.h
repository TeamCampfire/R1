/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 메인 인벤토리 컴포넌트 클래스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/ItemInstance.h"
#include "InventoryComponent.generated.h"

/**
 * 인벤토리 데이터 매니저의 1차 스캐폴딩.
 *
 * 일반슬롯/장비창/퀵슬롯 분리, MoveSlot, SplitStack, Equip/Unequip, 
 * UseItem, DropItem, SortInventory 같은 API는 추가 작업 필요
 */

class UItemDataBase;
class AItemPickup;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

/**
 * MoveSlot의 결과 — 대상 슬롯이 비어있었는지/같은 아이템이라 합쳐졌는지/다른
 * 아이템이라 서로 자리를 바꿨는지를 구분해서 돌려준다. 드래그앤드롭 UI에서
 * 결과에 따라 다른 사운드/이펙트를 재생하고 싶을 때 이 값을 쓰면 된다.
 */
UENUM(BlueprintType)
enum class EMoveSlotResult : uint8
{
	Failed,
	Moved,
	Merged,
	Swapped
};

/**
 * 인벤토리 데이터 매니저.
 * 
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class R1_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ItemData/Count를 인벤토리에 넣는다. 스택 가능한 아이템은 같은 종류의 기존
	// 슬롯에 먼저 채우고, 남으면 빈 슬롯에 새로 놓는다. 다 못 넣으면 못 넣은
	// 수량을 OutRemainder로 돌려준다(0이면 전부 성공, 반환값 true).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(UItemDataBase* ItemData, int32 Count, int32& OutRemainder);

	// 인벤토리 정보 Log 출력하는 테스트 함수
	void PrintInventoryInfo();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// 슬롯 개수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 SlotCount = 24;

	// 슬롯
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FItemInstance> Slots;

	// 슬로 변경 감지 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;
		
};
