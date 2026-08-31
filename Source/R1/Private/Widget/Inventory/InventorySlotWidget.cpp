// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Inventory/InventorySlotWidget.h"
#include "Widget/Inventory/InventoryDragDropOperation.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/UniformGridSlot.h"
#include "Data/Item/ItemDataBase.h"

void UInventorySlotWidget::SetSlotRef(const FInventorySlotRef& InSlotRef)
{
	SlotRef = InSlotRef;
}

void UInventorySlotWidget::EnsureGridSlots(
	UWidget* OwningWidget,
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass,
	UPanelWidget* Container,
	EInventorySlotCategory Category,
	const TArray<FItemInstance>& Slots,
	int32 GridColumns,
	TFunctionRef<void(UInventorySlotWidget*)> OnSlotCreated,
	TFunctionRef<bool(const FInventorySlotRef&)> IsSelectedFn,
	TFunctionRef<bool(const FInventorySlotRef&)> IsHeldFn,
	TArray<TObjectPtr<UInventorySlotWidget>>& OutWidgets)
{
	if (!Container || !SlotWidgetClass || !OwningWidget)
	{
		return;
	}

	// 슬롯 개수는 BeginPlay 이후 안 바뀌므로, 처음 한 번만 위젯을 생성하고 그 뒤로는 내용만 갱신한다.
	if (OutWidgets.Num() != Slots.Num())
	{
		Container->ClearChildren();
		OutWidgets.Reset();

		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(OwningWidget, SlotWidgetClass);
			if (!SlotWidget)
			{
				continue;
			}

			SlotWidget->SetSlotRef(FInventorySlotRef{ Category, i });
			OnSlotCreated(SlotWidget);

			UPanelSlot* PanelSlot = Container->AddChild(SlotWidget);
			if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(PanelSlot))
			{
				GridSlot->SetRow(i / GridColumns);
				GridSlot->SetColumn(i % GridColumns);
			}

			OutWidgets.Add(SlotWidget);
		}
	}

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (OutWidgets.IsValidIndex(i))
		{
			OutWidgets[i]->Refresh(Slots[i]);
			OutWidgets[i]->SetClickSelected(IsSelectedFn(FInventorySlotRef{ Category, i }));
			OutWidgets[i]->SetHeldHighlighted(IsHeldFn(FInventorySlotRef{ Category, i }));
		}
	}
}

void UInventorySlotWidget::Refresh(const FItemInstance& Instance)
{
	CachedInstance = Instance;

	if (Instance.IsValid())
	{
		if (IconImage)
		{
			UTexture2D* Icon = Instance.ItemData->Icon.LoadSynchronous();
			if (Icon)
			{
				IconImage->SetBrushFromTexture(Icon);
				IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				// 아이콘이 아직 없는 테스트 아이템 — 빈 칸으로 남기고 숨긴다(툴팁으로 이름 확인 가능).
				IconImage->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		if (CountBox)
		{
			CountBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		if (CountText)
		{
			CountText->SetText(FText::AsNumber(Instance.StackCount));
		}

		if (MaxStackText)
		{
			MaxStackText->SetText(FText::AsNumber(Instance.ItemData->MaxStackSize));
		}

		SetToolTipText(Instance.ItemData->DisplayName);
	}
	else
	{
		if (IconImage)
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (CountBox)
		{
			CountBox->SetVisibility(ESlateVisibility::Collapsed);
		}

		SetToolTipText(FText::GetEmpty());
	}
}

void UInventorySlotWidget::SetClickSelected(bool bInSelected)
{
	if (bIsClickSelected == bInSelected)
	{
		return;
	}

	bIsClickSelected = bInSelected;
	UpdateSelectionVisual();
}

void UInventorySlotWidget::SetHeldHighlighted(bool bInHeld)
{
	if (bIsHeldHighlighted == bInHeld)
	{
		return;
	}

	bIsHeldHighlighted = bInHeld;
	UpdateSelectionVisual();
}

void UInventorySlotWidget::NotifyDragCancelled()
{
	OnSlotDragCancelled.Broadcast(SlotRef);
}

void UInventorySlotWidget::UpdateSelectionVisual()
{
	if (!SelectionBorder)
	{
		return;
	}

	// 드래그 하이라이트(노란색)가 클릭 선택/손에 듦(파란색, 둘 다 같은 색이라 OR로 묶는다)보다 우선한다.
	if (bIsDragHovering)
	{
		SelectionBorder->SetBrushColor(FLinearColor(1.f, 0.85f, 0.1f, 0.3f));
	}
	else if (bIsClickSelected || bIsHeldHighlighted)
	{
		SelectionBorder->SetBrushColor(FLinearColor(0.2f, 0.55f, 1.f, 0.3f));
	}
	else
	{
		SelectionBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
	}
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭 선택은 빈 슬롯도 허용한다 — 아이템이 있으면 드래그 후보로도 등록하고,
	// 없으면(빈 슬롯) 옮길 게 없으니 그냥 눌림만 소비해서 뗄 때(Up) 선택 클릭으로 이어지게 한다.
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (CachedInstance.IsValid())
		{
			bPendingMiddleButtonDrag = false;
			return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
		}
		return FReply::Handled();
	}

	// 휠클릭(가운데 버튼) 드래그 — 좌클릭 드래그와 동일하게 등록하되, 이 드래그가 빈 슬롯에
	// 놓였을 때 TransferItem이 절반만 옮기도록 NativeOnDragDetected에 표시를 남겨둔다.
	if (CachedInstance.IsValid() && InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bPendingMiddleButtonDrag = true;
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::MiddleMouseButton);
	}

	// 우클릭(빠른 이동)은 옮길 아이템이 있는 슬롯에서만 의미가 있다.
	if (CachedInstance.IsValid() && InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		// 드래그가 없는 액션이라 눌린 걸 소비만 해두고 실제 액션은 뗄 때(NativeOnMouseButtonUp) 발생시킨다.
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventorySlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 드래그로 이어지지 않고 그냥 눌렀다 뗀 경우(클릭) — 빈 슬롯이어도 이 슬롯을 선택 상태로
	// 알린다. InventoryComponent::SelectSlot/GetSelectedItemInstance가 애초에 슬롯 내용물과
	// 무관하게 동작해서(빈 슬롯이면 GetSelectedItemInstance가 무효 인스턴스를 돌려줌), 여기서
	// CachedInstance 유효성을 따로 검사할 필요가 없다.
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnSlotClicked.Broadcast(SlotRef);
		return FReply::Handled();
	}

	if (CachedInstance.IsValid() && InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnSlotRightClicked.Broadcast(SlotRef);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!CachedInstance.IsValid())
	{
		return;
	}

	UInventoryDragDropOperation* DragOp = NewObject<UInventoryDragDropOperation>(this);
	DragOp->SourceSlotRef = SlotRef;
	DragOp->SourceWidget = this;
	DragOp->bAutoHalfSplitOnEmptyTarget = bPendingMiddleButtonDrag;
	DragOp->Pivot = EDragPivot::CenterCenter;

	UImage* DragVisual = NewObject<UImage>(this);
	UTexture2D* Icon = CachedInstance.ItemData->Icon.LoadSynchronous();

	if (Icon)
	{
		DragVisual->SetBrushFromTexture(Icon);
	}

	DragVisual->SetDesiredSizeOverride(FVector2D(64.f, 64.f));
	DragVisual->SetRenderOpacity(0.6f);
	DragOp->DefaultDragVisual = DragVisual;

	OutOperation = DragOp;
}

void UInventorySlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	if (Cast<UInventoryDragDropOperation>(InOperation))
	{
		bIsDragHovering = true;
		UpdateSelectionVisual();
	}
}

void UInventorySlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	bIsDragHovering = false;
	UpdateSelectionVisual();
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	bIsDragHovering = false;
	UpdateSelectionVisual();

	if (UInventoryDragDropOperation* DragOp = Cast<UInventoryDragDropOperation>(InOperation))
	{
		OnSlotDropped.Broadcast(DragOp->SourceSlotRef, SlotRef, DragOp->Count, DragOp->bAutoHalfSplitOnEmptyTarget);
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}
