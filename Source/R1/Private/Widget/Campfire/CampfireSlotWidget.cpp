#include "Widget/Campfire/CampfireSlotWidget.h"

#include "Campfire/CampfireActor.h"
#include "Campfire/CampfireComponent.h"
#include "Widget/Inventory/InventoryDragDropOperation.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Data/Item/ItemDataBase.h"

void UCampfireSlotWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (SlotTypeIcon)
	{
		SlotTypeIcon->SetBrushFromTexture(SlotTypeTexture);
		SlotTypeIcon->SetVisibility(SlotTypeTexture
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCampfireSlotWidget::InitializeSlot(ACampfireActor* InCampfire, const FCampfireSlotRef& InSlot)
{
	Campfire = InCampfire;
	SlotRef = InSlot;
}

void UCampfireSlotWidget::Refresh(const FItemInstance& Instance)
{
	CachedInstance = Instance;
	if (Instance.IsValid())
	{
		if (IconImage)
		{
			if (UTexture2D* Icon = Instance.ItemData->Icon.LoadSynchronous()) IconImage->SetBrushFromTexture(Icon);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (CountBox) CountBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (CountText) CountText->SetText(FText::AsNumber(Instance.StackCount));
		if (MaxStackText) MaxStackText->SetText(FText::AsNumber(Instance.ItemData->MaxStackSize));
		SetToolTipText(Instance.ItemData->DisplayName);
	}
	else
	{
		if (IconImage) IconImage->SetVisibility(ESlateVisibility::Collapsed);
		if (CountBox) CountBox->SetVisibility(ESlateVisibility::Collapsed);
		SetToolTipText(FText::GetEmpty());
	}
}

FReply UCampfireSlotWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent& Event)
{
	if (CachedInstance.IsValid() && (Event.GetEffectingButton() == EKeys::LeftMouseButton || Event.GetEffectingButton() == EKeys::MiddleMouseButton))
	{
		bMiddleDrag = Event.GetEffectingButton() == EKeys::MiddleMouseButton;
		return FReply::Handled().DetectDrag(TakeWidget(), Event.GetEffectingButton());
	}
	if (CachedInstance.IsValid() && Event.GetEffectingButton() == EKeys::RightMouseButton) return FReply::Handled();
	return FReply::Handled();
}

FReply UCampfireSlotWidget::NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (CachedInstance.IsValid() && Event.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnSlotRightClicked.Broadcast(Campfire.Get(), SlotRef);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(Geometry, Event);
}

void UCampfireSlotWidget::NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Out)
{
	Super::NativeOnDragDetected(Geometry, Event, Out);
	if (!CachedInstance.IsValid()) return;
	UInventoryDragDropOperation* Op = NewObject<UInventoryDragDropOperation>(this);
	Op->SourceType = EItemDragSourceType::Campfire;
	Op->SourceCampfire = Campfire;
	Op->CampfireSourceSlot = SlotRef;
	Op->DraggedItemData = CachedInstance.ItemData;
	Op->bAutoHalfSplitOnEmptyTarget = bMiddleDrag;
	Op->Pivot = EDragPivot::CenterCenter;
	UImage* Visual = NewObject<UImage>(this);
	if (UTexture2D* Icon = CachedInstance.ItemData->Icon.LoadSynchronous()) Visual->SetBrushFromTexture(Icon);
	Visual->SetDesiredSizeOverride({64.f, 64.f});
	Visual->SetRenderOpacity(0.6f);
	Op->DefaultDragVisual = Visual;
	Out = Op;
}

void UCampfireSlotWidget::UpdateHoverVisual(bool bHover, bool bAllowed)
{
	if (SelectionBorder) SelectionBorder->SetBrushColor(bHover
		? (bAllowed ? FLinearColor(0.35f, 0.75f, 0.15f, 0.45f) : FLinearColor(0.9f, 0.08f, 0.05f, 0.5f))
		: FLinearColor::Transparent);
}

void UCampfireSlotWidget::NativeOnDragEnter(const FGeometry& G, const FDragDropEvent& E, UDragDropOperation* Operation)
{
	Super::NativeOnDragEnter(G, E, Operation);
	const UInventoryDragDropOperation* Op = Cast<UInventoryDragDropOperation>(Operation);
	bool bAllowed = false;
	if (Op)
	{
		const UCampfireComponent* Comp = Campfire.IsValid() ? Campfire->GetCampfireComponent() : nullptr;
		bAllowed = Comp && Comp->CanAcceptItem(SlotRef, Op->DraggedItemData);
	}
	UpdateHoverVisual(Op != nullptr, bAllowed);
}

void UCampfireSlotWidget::NativeOnDragLeave(const FDragDropEvent& E, UDragDropOperation* Operation)
{
	Super::NativeOnDragLeave(E, Operation);
	UpdateHoverVisual(false, false);
}

bool UCampfireSlotWidget::NativeOnDrop(const FGeometry&, const FDragDropEvent&, UDragDropOperation* Operation)
{
	UpdateHoverVisual(false, false);
	UInventoryDragDropOperation* Op = Cast<UInventoryDragDropOperation>(Operation);
	if (!Op) return false;

	// 인식한 UI 드롭은 거절 시에도 소비한다. 기존 인벤토리의 DragCancelled=월드 드롭을 막는다.
	const UCampfireComponent* Comp = Campfire.IsValid() ? Campfire->GetCampfireComponent() : nullptr;
	if (!Comp || !Comp->CanAcceptItem(SlotRef, Op->DraggedItemData)) return true;
	if (Op->SourceType == EItemDragSourceType::PlayerInventory)
	{
		OnInventoryDropped.Broadcast(Campfire.Get(), Op->SourceSlotRef, SlotRef, Op->Count, Op->bAutoHalfSplitOnEmptyTarget);
	}
	else if (Op->SourceCampfire == Campfire)
	{
		OnCampfireDropped.Broadcast(Campfire.Get(), Op->CampfireSourceSlot, SlotRef, Op->Count, Op->bAutoHalfSplitOnEmptyTarget);
	}
	return true;
}
