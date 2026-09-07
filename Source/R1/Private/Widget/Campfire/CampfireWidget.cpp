#include "Widget/Campfire/CampfireWidget.h"

#include "Item/PlaceableItem/Campfire/Campfire.h"
#include "Item/PlaceableItem/Campfire/CampfireComponent.h"
#include "Character/ActionPlayerController.h"
#include "Widget/Campfire/CampfireSlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UCampfireWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ToggleFireButton) ToggleFireButton->OnClicked.AddDynamic(this, &UCampfireWidget::HandleToggleFire);
}

void UCampfireWidget::NativeDestruct()
{
	UnbindCampfire();
	Super::NativeDestruct();
}

void UCampfireWidget::BindCampfire(ACampfire* InCampfire)
{
	UnbindCampfire();
	BoundCampfire = InCampfire;
	if (UCampfireComponent* Comp = InCampfire ? InCampfire->GetCampfireComponent() : nullptr)
	{
		Comp->OnCampfireStateChanged.AddDynamic(this, &UCampfireWidget::Refresh);
		InitializeSlots();
		Refresh();
	}
}

void UCampfireWidget::UnbindCampfire()
{
	if (BoundCampfire.IsValid())
	{
		if (UCampfireComponent* Comp = BoundCampfire->GetCampfireComponent())
			Comp->OnCampfireStateChanged.RemoveDynamic(this, &UCampfireWidget::Refresh);
	}
	BoundCampfire.Reset();
}

void UCampfireWidget::InitializeSlots()
{
	const auto Initialize = [this](UCampfireSlotWidget* SlotWidget, const FCampfireSlotRef& Ref)
	{
		if (!SlotWidget || !BoundCampfire.IsValid()) return;
		SlotWidget->InitializeSlot(BoundCampfire.Get(), Ref);
		SlotWidget->OnSlotRightClicked.AddUniqueDynamic(this, &UCampfireWidget::HandleSlotRightClicked);
		SlotWidget->OnInventoryDropped.AddUniqueDynamic(this, &UCampfireWidget::HandleInventoryDropped);
		SlotWidget->OnCampfireDropped.AddUniqueDynamic(this, &UCampfireWidget::HandleCampfireDropped);
	};
	Initialize(FuelSlot, { ECampfireSlotType::Fuel, 0 });
	Initialize(InputSlot, { ECampfireSlotType::Input, 0 });
	Initialize(FuelOutputSlot, { ECampfireSlotType::Output, 0 });
	Initialize(CookingOutputSlot, { ECampfireSlotType::Output, 1 });
}

void UCampfireWidget::Refresh()
{
	UCampfireComponent* Comp = BoundCampfire.IsValid() ? BoundCampfire->GetCampfireComponent() : nullptr;
	if (!Comp) return;
	if (FuelSlot) FuelSlot->Refresh(Comp->GetSlot({ ECampfireSlotType::Fuel, 0 }));
	if (InputSlot) InputSlot->Refresh(Comp->GetSlot({ ECampfireSlotType::Input, 0 }));
	if (FuelOutputSlot) FuelOutputSlot->Refresh(Comp->GetSlot({ ECampfireSlotType::Output, 0 }));
	if (CookingOutputSlot) CookingOutputSlot->Refresh(Comp->GetSlot({ ECampfireSlotType::Output, 1 }));
	if (CookingProgressBar) CookingProgressBar->SetPercent(Comp->GetCookingProgress());
	if (FuelProgressBar) FuelProgressBar->SetPercent(Comp->GetFuelProgress());
	if (ToggleFireText) ToggleFireText->SetText(Comp->bIsLit ? FText::FromString(TEXT("끄기")) : FText::FromString(TEXT("켜기")));
}

void UCampfireWidget::HandleToggleFire()
{
	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
		PC->Server_SetCampfireLit(BoundCampfire.Get(), !(BoundCampfire.IsValid() && BoundCampfire->GetCampfireComponent()->bIsLit));
}

void UCampfireWidget::HandleSlotRightClicked(ACampfire* Campfire, FCampfireSlotRef CampfireSlotRef)
{
	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer())) PC->Server_QuickMoveCampfireToInventory(Campfire, CampfireSlotRef);
}

void UCampfireWidget::HandleInventoryDropped(ACampfire* Campfire, FInventorySlotRef From, FCampfireSlotRef To, int32 Count, bool bHalfSplit)
{
	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer())) PC->Server_MoveInventoryToCampfire(Campfire, From, To, Count, bHalfSplit);
}

void UCampfireWidget::HandleCampfireDropped(ACampfire*, FCampfireSlotRef, FCampfireSlotRef, int32, bool)
{
	// 입력/연료의 허용 아이템이 서로 다르고 슬롯도 각 1개이므로 내부 슬롯 간 이동은 의도적으로 거절한다.
}
