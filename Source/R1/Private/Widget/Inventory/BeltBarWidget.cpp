// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Inventory/BeltBarWidget.h"
#include "Widget/Inventory/InventorySlotWidget.h"
#include "GameFramework/Pawn.h"
#include "Character/ActionPlayerController.h"

void UBeltBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!IsDesignTime() || !SlotWidgetClass)
	{
		return;
	}

	// InventoryWidget::NativePreConstruct와 동일한 이유 — 디자이너 프리뷰 전용 더미 채움.
	const UInventoryComponent* DefaultInventory = GetDefault<UInventoryComponent>();
	TArray<FItemInstance> PreviewBeltSlots;
	PreviewBeltSlots.SetNum(DefaultInventory->BeltSlotCount);

	UInventorySlotWidget::EnsureGridSlots(this, SlotWidgetClass, BeltSlotContainer, EInventorySlotCategory::Belt, PreviewBeltSlots, GridColumns,
		[](UInventorySlotWidget*) {},
		[](const FInventorySlotRef&) { return false; },
		[](const FInventorySlotRef&) { return false; },
		BeltSlotWidgets);
}

void UBeltBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->OnPossessedCharChange.AddDynamic(this, &UBeltBarWidget::RebindInventory);
	}

	RebindInventory();
}

void UBeltBarWidget::NativeDestruct()
{
	UnbindInventoryDelegates();

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->OnPossessedCharChange.RemoveDynamic(this, &UBeltBarWidget::RebindInventory);
	}

	Super::NativeDestruct();
}

void UBeltBarWidget::UnbindInventoryDelegates()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UBeltBarWidget::HandleInventoryChanged);
		Inventory->OnSelectionChanged.RemoveDynamic(this, &UBeltBarWidget::HandleInventoryChanged);
	}
}

void UBeltBarWidget::RebindInventory()
{
	UnbindInventoryDelegates();
	BoundInventory = nullptr;

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UInventoryComponent* Inventory = OwningPawn->FindComponentByClass<UInventoryComponent>())
		{
			BoundInventory = Inventory;
			Inventory->OnInventoryChanged.AddDynamic(this, &UBeltBarWidget::HandleInventoryChanged);
			Inventory->OnSelectionChanged.AddDynamic(this, &UBeltBarWidget::HandleInventoryChanged);
		}
	}

	HandleInventoryChanged();
}

void UBeltBarWidget::HandleInventoryChanged()
{
	UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !SlotWidgetClass)
	{
		return;
	}

	UInventorySlotWidget::EnsureGridSlots(this, SlotWidgetClass, BeltSlotContainer, EInventorySlotCategory::Belt, Inventory->BeltSlots, GridColumns,
		[this](UInventorySlotWidget* SlotWidget)
		{
			SlotWidget->OnSlotDropped.AddDynamic(this, &UBeltBarWidget::HandleSlotDropped);
			SlotWidget->OnSlotClicked.AddDynamic(this, &UBeltBarWidget::HandleSlotClicked);
			SlotWidget->OnSlotRightClicked.AddDynamic(this, &UBeltBarWidget::HandleSlotRightClicked);
			SlotWidget->OnSlotDragCancelled.AddDynamic(this, &UBeltBarWidget::HandleSlotDragCancelled);
		},
		[Inventory](const FInventorySlotRef& SlotRef)
		{
			return Inventory->IsSlotSelected(SlotRef);
		},
		[Inventory](const FInventorySlotRef& SlotRef)
		{
			// HeldBeltIndex는 UseBeltSlot에서 Weapon/Tool일 때만 설정되므로, 소비/기타/장비(의류)
			// 슬롯은 여기서 걸릴 일이 없다 — 카테고리 체크 없이 인덱스만 비교해도 안전하다.
			return Inventory->HeldBeltIndex != INDEX_NONE && Inventory->HeldBeltIndex == SlotRef.Index;
		},
		BeltSlotWidgets);
}

void UBeltBarWidget::HandleSlotDropped(FInventorySlotRef FromSlot, FInventorySlotRef ToSlot, int32 Count, bool bAutoHalfSplitOnEmptyTarget)
{
	UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	if (FromSlot.Category == ToSlot.Category && FromSlot.Index == ToSlot.Index)
	{
		return;
	}

	Inventory->Server_TransferItem(FromSlot, ToSlot, Count, bAutoHalfSplitOnEmptyTarget);
}

void UBeltBarWidget::HandleSlotClicked(FInventorySlotRef SlotRef)
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->SelectSlot(SlotRef);
	}
}

void UBeltBarWidget::HandleSlotRightClicked(FInventorySlotRef SlotRef)
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->Server_QuickMoveItem(SlotRef);
	}
}

void UBeltBarWidget::HandleSlotDragCancelled(FInventorySlotRef SlotRef)
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->Server_ThrowItem(SlotRef, 0);
	}
}
