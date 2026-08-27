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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

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
