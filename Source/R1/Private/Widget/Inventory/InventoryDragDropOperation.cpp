// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Inventory/InventoryDragDropOperation.h"
#include "Widget/Inventory/InventorySlotWidget.h"

void UInventoryDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	Super::DragCancelled_Implementation(PointerEvent);

	if (UInventorySlotWidget* Widget = SourceWidget.Get())
	{
		Widget->NotifyDragCancelled();
	}
}
