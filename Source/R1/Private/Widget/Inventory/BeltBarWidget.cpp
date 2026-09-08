// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Inventory/BeltBarWidget.h"
#include "Widget/Inventory/InventorySlotWidget.h"
#include "GameFramework/Pawn.h"
#include "Character/ActionPlayerController.h"
#include "Component/InteractionComponent.h"
#include "Framework/MainHUD.h"
#include "Widget/MainHUDWidget.h"

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
			SlotWidget->OnCampfireItemDropped.AddDynamic(this, &UBeltBarWidget::HandleCampfireItemDropped);
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
	AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer());
	if (PC)
	{
		UInteractionComponent* Interaction = PC->GetPawn()
			? PC->GetPawn()->FindComponentByClass<UInteractionComponent>() : nullptr;
		if (ACampfire* Campfire = Interaction ? Interaction->GetActiveCampfire() : nullptr)
		{
			PC->Server_QuickMoveInventoryToCampfire(Campfire, SlotRef);
			return;
		}
	}

	UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	// 창고가 열려있으면 우클릭은 메인↔벨트/장착(QuickMoveItem) 대신 "창고로 보내기"로 동작한다.
	if (AMainHUD* HUD = PC ? PC->GetHUD<AMainHUD>() : nullptr)
	{
		if (UMainHUDWidget* MainHudWidget = HUD->GetMainHudWidget())
		{
			if (UWarehouseInventoryComponent* OpenWarehouse = MainHudWidget->GetOpenWarehouse())
			{
				Inventory->Server_QuickMoveToWarehouse(OpenWarehouse, SlotRef);
				return;
			}
		}
	}

	Inventory->Server_QuickMoveItem(SlotRef);
}

void UBeltBarWidget::HandleCampfireItemDropped(ACampfire* Campfire, FCampfireSlotRef FromSlot,
	FInventorySlotRef ToSlot, int32 Count, bool bAutoHalfSplitOnEmptyTarget)
{
	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->Server_MoveCampfireToInventory(Campfire, FromSlot, ToSlot, Count, bAutoHalfSplitOnEmptyTarget);
	}
}

void UBeltBarWidget::HandleSlotDragCancelled(FInventorySlotRef SlotRef)
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->Server_ThrowItem(SlotRef, 0);
	}
}
