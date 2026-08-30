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

	// 드래그를 시작한 슬롯 위젯 — 유효한 드롭 대상 없이(슬롯 밖) 드래그가 끝났을 때
	// 그 사실을 알려주기 위해 들고 있는다. UInventorySlotWidget::NativeOnDragDetected에서 설정.
	UPROPERTY()
	TWeakObjectPtr<UInventorySlotWidget> SourceWidget;

protected:
	//~ Begin UDragDropOperation Interface
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
	//~ End UDragDropOperation Interface
};
