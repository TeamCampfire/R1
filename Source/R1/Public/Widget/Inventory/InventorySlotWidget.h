/// 최초작성 : 2026.08.30
/// 작 성 자 : 최 요 환
/// 간단설명 : 인벤토리 슬롯 하나를 표현하는 재사용 위젯

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/InventoryComponent.h"
#include "Item/ItemInstance.h"
#include "InventorySlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UWidget;
class UPanelWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySlotDropped, FInventorySlotRef, FromSlot, FInventorySlotRef, ToSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotClicked, FInventorySlotRef, SlotRef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotRightClicked, FInventorySlotRef, SlotRef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotDragCancelled, FInventorySlotRef, SlotRef);

/**
 * 슬롯 하나(장비/메인/벨트 공통)를 표현하는 재사용 위젯.
 *
 * 이 위젯 자체는 "지금 몇 번 슬롯을 표현하는지"(SlotRef)와 "무엇을 보여줄지"(Refresh로 받은
 * FItemInstance)만 알고, 실제 이동 로직은 모른다 — 드롭되면 OnSlotDropped로 (어디서 왔는지,
 * 자기 자신)만 알리고, 실제 TransferItem 호출은 이 위젯을 여러 개 관리하는
 * UInventoryWidget이 담당한다.
 *
 * 버튼(UButton)을 쓰지 않는다 — 루트는 Border이고, 클릭/드래그는 UUserWidget의
 * NativeOnMouseButtonDown/NativeOnDragDetected/NativeOnDrop을 직접 오버라이드해서 처리한다.
 *
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다:
 * - RootBorder      : 루트. 반투명 회색 배경.
 * - IconImage       : 아이템 아이콘. 아이콘이 없는 아이템은 자동으로 숨겨진다.
 * - CountBox        : 수량 표시 묶음(현재/최대) 컨테이너 — 빈 슬롯이면 통째로 숨김.
 * - CountText       : 현재 스택 수량.
 * - MaxStackText    : 최대 스택 수량.
 * - SelectionBorder : 하이라이트 테두리 하나를 두 가지 용도로 겸용한다 —
 *                     드래그 중인 아이템이 이 슬롯 위에 올라와 있으면 노란색(우선),
 *                     아니면 이 슬롯이 클릭으로 "선택"된 상태면 파란색.
 * 아이템 이름은 슬롯에 텍스트로 안 찍고 툴팁(마우스 오버)으로 대신한다.
 */
UCLASS()
class R1_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSlotRef(const FInventorySlotRef& InSlotRef);
	const FInventorySlotRef& GetSlotRef() const { return SlotRef; }

	void Refresh(const FItemInstance& Instance);

	// 클릭으로 이 슬롯이 선택됐는지(파란 테두리) — InventoryComponent::IsSlotSelected를 보고
	// 소유 위젯(UInventoryWidget/UBeltBarWidget)이 매 갱신마다 호출해준다.
	void SetClickSelected(bool bInSelected);

	// Container에 Slots.Num()개의 슬롯 위젯을 채운다(최초 1회 생성, 이후엔 내용만 갱신) —
	// 장비/메인/벨트 패널을 각각 다른 UUserWidget이 갖고 있어도 이 함수 하나로 공유한다.
	// Container가 UniformGridPanel이면 GridColumns 기준으로 Row/Column을 배정한다.
	// OnSlotCreated는 새로 생성된 슬롯 위젯마다 한 번씩 호출된다 — 호출부가 자기 클래스의
	// HandleSlotDropped/HandleSlotClicked를 OnSlotDropped/OnSlotClicked.AddDynamic으로 직접 바인딩하는 용도.
	// IsSelectedFn은 매 갱신(Refresh)마다 슬롯별로 호출되어 파란 선택 테두리를 켤지 결정한다.
	static void EnsureGridSlots(
		UWidget* OwningWidget,
		TSubclassOf<UInventorySlotWidget> SlotWidgetClass,
		UPanelWidget* Container,
		EInventorySlotCategory Category,
		const TArray<FItemInstance>& Slots,
		int32 GridColumns,
		TFunctionRef<void(UInventorySlotWidget*)> OnSlotCreated,
		TFunctionRef<bool(const FInventorySlotRef&)> IsSelectedFn,
		TArray<TObjectPtr<UInventorySlotWidget>>& OutWidgets);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventorySlotDropped OnSlotDropped;

	// 이 슬롯이 (드래그가 아니라) 클릭됐을 때 — 아이템이 있는 슬롯에서만 발생한다.
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventorySlotClicked OnSlotClicked;

	// 우클릭(빠른 이동/장착) — 아이템이 있는 슬롯에서만 발생한다.
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventorySlotRightClicked OnSlotRightClicked;

	// 이 슬롯에서 시작한 드래그가 유효한 드롭 대상(슬롯) 없이 끝났을 때 — 월드에 드랍하는 용도.
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventorySlotDragCancelled OnSlotDragCancelled;

	// UInventoryDragDropOperation::DragCancelled에서 호출된다.
	void NotifyDragCancelled();

protected:
	//~ Begin UUserWidget Interface
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	//~ End UUserWidget Interface

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CountBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MaxStackText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SelectionBorder;

private:
	// 드래그 하이라이트(노란색)와 클릭 선택(파란색) 중 우선순위(드래그 우선)를 따져
	// SelectionBorder 색상을 갱신한다.
	void UpdateSelectionVisual();

	FInventorySlotRef SlotRef;

	// 드래그 시작/시각화에 쓰려고 Refresh 시점의 인스턴스를 들고 있는다.
	FItemInstance CachedInstance;

	bool bIsDragHovering = false;
	bool bIsClickSelected = false;
};
