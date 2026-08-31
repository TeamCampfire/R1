// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Inventory/InventoryWidget.h"
#include "Widget/Inventory/InventorySlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Data/Item/ItemDataBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

UInventoryWidget* UInventoryWidget::ShowInventoryTestWidget(UObject* WorldContextObject, TSubclassOf<UInventoryWidget> WidgetClass)
{
	UE_LOG(LogTemp, Warning, TEXT("[InvTest] ShowInventoryTestWidget called. WorldContextObject=%s WidgetClass=%s"),
		*GetNameSafe(WorldContextObject), *GetNameSafe(*WidgetClass));

	if (!WorldContextObject || !WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InvTest] Aborting: WorldContextObject or WidgetClass is null"));
		return nullptr;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	UE_LOG(LogTemp, Warning, TEXT("[InvTest] PlayerController=%s"), *GetNameSafe(PC));
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InvTest] Aborting: PlayerController is null"));
		return nullptr;
	}

	UInventoryWidget* Widget = CreateWidget<UInventoryWidget>(PC, WidgetClass);
	UE_LOG(LogTemp, Warning, TEXT("[InvTest] CreateWidget result=%s"), *GetNameSafe(Widget));
	if (Widget)
	{
		Widget->AddToViewport();
		UE_LOG(LogTemp, Warning, TEXT("[InvTest] AddToViewport called"));
	}

	return Widget;
}

void UInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UInventoryComponent* Inventory = OwningPawn->FindComponentByClass<UInventoryComponent>())
		{
			BoundInventory = Inventory;
			Inventory->OnInventoryChanged.AddDynamic(this, &UInventoryWidget::HandleInventoryChanged);
			// 선택 상태만 바뀌어도(슬롯 내용물은 그대로) 파란 테두리를 다시 그려야 하므로 같이 구독한다.
			Inventory->OnSelectionChanged.AddDynamic(this, &UInventoryWidget::HandleInventoryChanged);
		}
	}

	HandleInventoryChanged();
}

void UInventoryWidget::NativeDestruct()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
		Inventory->OnSelectionChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

void UInventoryWidget::HandleInventoryChanged()
{
	RefreshStateText();
	RebuildSlots();
}

void UInventoryWidget::RefreshStateText()
{
	if (!InventoryStateText)
	{
		return;
	}

	UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		InventoryStateText->SetText(FText::FromString(TEXT("(InventoryComponent 없음)")));
		return;
	}

	auto AppendArray = [](FString& Out, const TCHAR* Label, const TArray<FItemInstance>& Slots)
	{
		Out += FString::Printf(TEXT("[%s]\n"), Label);
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (Slots[i].IsValid())
			{
				Out += FString::Printf(TEXT("  %d: %s x%d\n"), i, *Slots[i].ItemData->DisplayName.ToString(), Slots[i].StackCount);
			}
			else
			{
				Out += FString::Printf(TEXT("  %d: -\n"), i);
			}
		}
	};

	FString Result;
	AppendArray(Result, TEXT("Equipment"), Inventory->EquipmentSlots);
	AppendArray(Result, TEXT("Main"), Inventory->MainSlots);
	AppendArray(Result, TEXT("Belt"), Inventory->BeltSlots);
	Result += FString::Printf(TEXT("HeldBeltIndex: %d\n"), Inventory->HeldBeltIndex);

	InventoryStateText->SetText(FText::FromString(Result));
}

void UInventoryWidget::RebuildSlots()
{
	UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !SlotWidgetClass)
	{
		return;
	}

	auto BindDropped = [this](UInventorySlotWidget* SlotWidget)
	{
		SlotWidget->OnSlotDropped.AddDynamic(this, &UInventoryWidget::HandleSlotDropped);
		SlotWidget->OnSlotClicked.AddDynamic(this, &UInventoryWidget::HandleSlotClicked);
		SlotWidget->OnSlotRightClicked.AddDynamic(this, &UInventoryWidget::HandleSlotRightClicked);
		SlotWidget->OnSlotDragCancelled.AddDynamic(this, &UInventoryWidget::HandleSlotDragCancelled);
	};

	auto IsSelectedFn = [Inventory](const FInventorySlotRef& SlotRef)
	{
		return Inventory->IsSlotSelected(SlotRef);
	};

	UInventorySlotWidget::EnsureGridSlots(this, SlotWidgetClass, EquipmentSlotContainer, EInventorySlotCategory::Equipment, Inventory->EquipmentSlots, GridColumns, BindDropped, IsSelectedFn, EquipmentSlotWidgets);
	UInventorySlotWidget::EnsureGridSlots(this, SlotWidgetClass, MainSlotContainer, EInventorySlotCategory::Main, Inventory->MainSlots, GridColumns, BindDropped, IsSelectedFn, MainSlotWidgets);
}

void UInventoryWidget::HandleSlotDropped(FInventorySlotRef FromSlot, FInventorySlotRef ToSlot)
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

	// Count<=0 => 전량 이동.
	Inventory->TransferItem(FromSlot, ToSlot, 0);
}

void UInventoryWidget::HandleSlotClicked(FInventorySlotRef SlotRef)
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->SelectSlot(SlotRef);
	}
}

void UInventoryWidget::HandleSlotRightClicked(FInventorySlotRef SlotRef)
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->QuickMoveItem(SlotRef);
	}
}

void UInventoryWidget::HandleSlotDragCancelled(FInventorySlotRef SlotRef)
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->ThrowItem(SlotRef, 0);
	}
}
