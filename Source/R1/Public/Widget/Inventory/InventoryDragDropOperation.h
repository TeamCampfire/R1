/// 최초작성 : 2026.08.30
/// 작 성 자 : 최 요 환
/// 간단설명 : 인벤토리 슬롯 드래그앤드롭 시 어느 슬롯에서 시작됐는지 들고 다니는 페이로드

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Component/InventoryComponent.h"
#include "Item/PlaceableItem/Campfire/CampfireTypes.h"
#include "InventoryDragDropOperation.generated.h"

class UInventorySlotWidget;
class ACampfire;
class UItemDataBase;

/**
 * 인벤토리 슬롯 드래그 시작 지점을 들고 다니는 페이로드.
 * 드래그 비주얼(반투명 아이콘)은 UInventorySlotWidget이 DefaultDragVisual로 직접 채운다 —
 * 이 클래스는 "어디서 시작됐는지"만 들고 있는다.
 */
UCLASS()
class R1_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	EItemDragSourceType SourceType = EItemDragSourceType::PlayerInventory;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UItemDataBase> DraggedItemData;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	FInventorySlotRef SourceSlotRef;

	// 옮길 수량. 0 이하이면 슬롯 전체(TransferItem 기준)를 뜻한다 — 일반 슬롯 드래그는 항상 0,
	// DetailInfoWidget의 분할 드래그만 여기에 양수 값(CurrentSplitCount)을 채워 넣는다.
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int32 Count = 0;

	// 휠클릭(가운데 버튼)으로 시작한 드래그인지 — true면 빈 슬롯에 놓았을 때 TransferItem이
	// (Count가 0 이하라는 전제하에) 전량 이동 대신 절반만 떼어 옮긴다. InventorySlotWidget::
	// NativeOnMouseButtonDown이 MiddleMouseButton으로 드래그를 감지했을 때만 true로 채워진다.
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	bool bAutoHalfSplitOnEmptyTarget = false;

	// 드래그를 시작한 슬롯 위젯 — 유효한 드롭 대상 없이(슬롯 밖) 드래그가 끝났을 때
	// 그 사실을 알려주기 위해 들고 있는다. UInventorySlotWidget::NativeOnDragDetected에서 설정.
	UPROPERTY()
	TWeakObjectPtr<UInventorySlotWidget> SourceWidget;

	// 드래그를 시작한 슬롯이 속한 컨테이너 식별자(UInventorySlotWidget::ContainerId 그대로 복사).
	// 같은 화면에 서로 다른 인벤토리(예: 플레이어 인벤토리 vs 창고)를 동시에 표시할 때, 드롭된
	// 슬롯의 ContainerId와 비교해서 같은 컨테이너 내부 이동인지 컨테이너를 넘나드는 이동인지
	// 구분하는 용도 — 기본값 0은 항상 "일반 인벤토리 화면"을 뜻하므로 창고가 없는 기존 위젯
	// (WBP_Inventory, WBP_BeltBar 등)은 전부 0끼리만 비교되어 동작이 그대로 유지된다.
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int32 SourceContainerId = 0;

	// 모닥불 구분

	UPROPERTY(BlueprintReadWrite, Category = "Campfire")
	FCampfireSlotRef CampfireSourceSlot;

	UPROPERTY()
	TWeakObjectPtr<ACampfire> SourceCampfire;

protected:
	//~ Begin UDragDropOperation Interface
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
	//~ End UDragDropOperation Interface
};
