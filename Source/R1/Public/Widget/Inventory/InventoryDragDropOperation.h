/// 최초작성 : 2026.08.30
/// 작 성 자 : 최 요 환
/// 간단설명 : 인벤토리 슬롯 드래그앤드롭 시 어느 슬롯에서 시작됐는지 들고 다니는 페이로드

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Component/InventoryComponent.h"
#include "InventoryDragDropOperation.generated.h"

class UInventorySlotWidget;

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

protected:
	//~ Begin UDragDropOperation Interface
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
	//~ End UDragDropOperation Interface
};
