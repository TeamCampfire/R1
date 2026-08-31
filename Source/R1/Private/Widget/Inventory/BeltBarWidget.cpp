// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Inventory/BeltBarWidget.h"
#include "Widget/Inventory/InventorySlotWidget.h"
#include "GameFramework/Pawn.h"

void UBeltBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

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

void UBeltBarWidget::NativeDestruct()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UBeltBarWidget::HandleInventoryChanged);
		Inventory->OnSelectionChanged.RemoveDynamic(this, &UBeltBarWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
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
		BeltSlotWidgets);
}

void UBeltBarWidget::HandleSlotDropped(FInventorySlotRef FromSlot, FInventorySlotRef ToSlot)
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

	Inventory->TransferItem(FromSlot, ToSlot, 0);
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
		Inventory->QuickMoveItem(SlotRef);
	}
}

void UBeltBarWidget::HandleSlotDragCancelled(FInventorySlotRef SlotRef)
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->ThrowItem(SlotRef, 0);
	}
}
