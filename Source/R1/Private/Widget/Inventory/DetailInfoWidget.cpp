


#include "Widget/Inventory/DetailInfoWidget.h"
#include "Widget/ParameterBarWidget.h"
#include "Widget/Inventory/InventoryDragDropOperation.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/PanelWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Data/Item/ItemDataBase.h"
#include "Data/Item/EquipmentItemData.h"
#include "Data/Item/ConsumableItemData.h"
#include "GameFramework/Pawn.h"

void UDetailInfoWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UInventoryComponent* Inventory = OwningPawn->FindComponentByClass<UInventoryComponent>())
		{
			BoundInventory = Inventory;
			Inventory->OnInventoryChanged.AddDynamic(this, &UDetailInfoWidget::HandleInventoryChanged);
			Inventory->OnSelectionChanged.AddDynamic(this, &UDetailInfoWidget::HandleInventoryChanged);
		}
	}

	if (DiscardButton)
	{
		DiscardButton->OnClicked.AddDynamic(this, &UDetailInfoWidget::HandleDiscardClicked);
	}

	if (SplitQuantitySlider)
	{
		SplitQuantitySlider->OnValueChanged.AddDynamic(this, &UDetailInfoWidget::HandleSplitQuantityChanged);
	}

	HandleInventoryChanged();
}

void UDetailInfoWidget::NativeDestruct()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UDetailInfoWidget::HandleInventoryChanged);
		Inventory->OnSelectionChanged.RemoveDynamic(this, &UDetailInfoWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

void UDetailInfoWidget::HandleInventoryChanged()
{
	RefreshDisplay();
}

void UDetailInfoWidget::RefreshDisplay()
{
	UInventoryComponent* Inventory = BoundInventory.Get();
	const FItemInstance Selected = Inventory ? Inventory->GetSelectedItemInstance() : FItemInstance();

	if (RootPanel)
	{
		RootPanel->SetVisibility(Selected.IsValid() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (!Selected.IsValid())
	{
		return;
	}

	UItemDataBase* ItemData = Selected.ItemData;

	if (TitleText)
	{
		TitleText->SetText(ItemData->DisplayName);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(ItemData->Description);
	}

	if (IconImage)
	{
		if (UTexture2D* LoadedIcon = ItemData->Icon.LoadSynchronous())
		{
			IconImage->SetBrushFromTexture(LoadedIcon);
			IconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 분할 드래그 아이콘도 IconImage와 동일하게 선택된 아이템의 아이콘을 그대로 보여준다.
	// WBP 쪽 placeholder 틴트(노란색)는 SetBrushFromTexture가 안 지워주므로 흰색으로 리셋해야
	// 실제 아이콘 텍스처가 원래 색 그대로 보인다.
	if (SplitDragIcon)
	{
		if (UTexture2D* LoadedIcon = ItemData->Icon.LoadSynchronous())
		{
			SplitDragIcon->SetBrushFromTexture(LoadedIcon);
			SplitDragIcon->SetBrushTintColor(FLinearColor::White);
			SplitDragIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			SplitDragIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 분할 가능 = 스택형 아이템(MaxStackSize > 1)이고 현재 2개 이상 들고 있을 때만.
	const bool bCanSplit = (ItemData->MaxStackSize > 1) && (Selected.StackCount > 1);
	if (SplitPanel)
	{
		SplitPanel->SetVisibility(bCanSplit ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (bCanSplit && SplitQuantitySlider)
	{
		SplitQuantitySlider->SetMinValue(1.f);
		SplitQuantitySlider->SetMaxValue(static_cast<float>(Selected.StackCount - 1));
		CurrentSplitCount = FMath::Clamp(CurrentSplitCount, 1, Selected.StackCount - 1);
		SplitQuantitySlider->SetValue(static_cast<float>(CurrentSplitCount));
	}

	if (bCanSplit && SplitQuantityText)
	{
		SplitQuantityText->SetText(FText::AsNumber(CurrentSplitCount));
	}

	RebuildInfoRows(Selected);
	RebuildActionButtons(Selected);
}

void UDetailInfoWidget::RebuildInfoRows(const FItemInstance& Selected)
{
	if (!InfoRowsContainer)
	{
		return;
	}

	InfoRowsContainer->ClearChildren();

	if (const UEquipmentItemData* EquipmentData = Cast<UEquipmentItemData>(Selected.ItemData))
	{
		for (const FEquipmentStatModifier& Modifier : EquipmentData->StatModifiers)
		{
			AddStatBarRow(Modifier.StatType, Modifier.Value);
		}
	}
	else if (const UConsumableItemData* ConsumableData = Cast<UConsumableItemData>(Selected.ItemData))
	{
		for (const FItemEffect& Effect : ConsumableData->Effects)
		{
			AddEffectTextRow(Effect);
		}
	}
}

void UDetailInfoWidget::AddStatBarRow(EEquipmentStatType StatType, float Value)
{
	if (!InfoRowsContainer)
	{
		return;
	}

	UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

	UTextBlock* Label = NewObject<UTextBlock>(this);
	Label->SetText(StaticEnum<EEquipmentStatType>()->GetDisplayNameTextByValue(static_cast<int64>(StatType)));
	Row->AddChildToHorizontalBox(Label);

	if (StatBarWidgetClass)
	{
		if (UParameterBarWidget* Bar = CreateWidget<UParameterBarWidget>(this, StatBarWidgetClass))
		{
			Bar->UpdateParameterBar(Value, GetStatBarReferenceMax(StatType));

			if (UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(Bar))
			{
				BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
	}

	InfoRowsContainer->AddChild(Row);
}

void UDetailInfoWidget::AddEffectTextRow(const FItemEffect& Effect)
{
	if (!InfoRowsContainer)
	{
		return;
	}

	UTextBlock* Row = NewObject<UTextBlock>(this);
	const FText EffectLabel = StaticEnum<EItemEffectType>()->GetDisplayNameTextByValue(static_cast<int64>(Effect.EffectType));
	Row->SetText(FText::Format(NSLOCTEXT("DetailInfoWidget", "EffectRowFormat", "{0} +{1}"), EffectLabel, FText::AsNumber(Effect.Magnitude)));

	/// 텍스트 색상을 지정하고 싶다면 아래 코드를 수정해서 사용.
	//Row->SetColorAndOpacity(FSlateColor(GetEffectColor(Effect.EffectType)));

	InfoRowsContainer->AddChild(Row);
}

float UDetailInfoWidget::GetStatBarReferenceMax(EEquipmentStatType StatType)
{
	// 실제 스탯의 진짜 상한이 아니라, 바가 얼마나 채워질지 가늠하기 위한 순전히 시각적인
	// 기준값이다 — 밸런스가 잡히면 여기 값만 조정하면 되고 구조는 안 건드려도 된다.
	switch (StatType)
	{
		case EEquipmentStatType::Defense:				return 50.f;
		case EEquipmentStatType::MovementSpeedMult:	return 2.f;
		case EEquipmentStatType::HarvestDamage:		return 100.f;
		case EEquipmentStatType::WoodGatheringMult:	return 3.f;
		case EEquipmentStatType::OreGatheringMult:		return 3.f;
		case EEquipmentStatType::FleshGatheringMult:	return 3.f;
		case EEquipmentStatType::DurabilityLossMult:	return 2.f;
		default:										return 1.f;
	}
}

FLinearColor UDetailInfoWidget::GetEffectColor(EItemEffectType EffectType)
{
	switch (EffectType)
	{
		case EItemEffectType::Heal:				return FLinearColor(1.f, 0.25f, 0.25f);
		case EItemEffectType::RestoreHunger:	return FLinearColor(1.f, 0.65f, 0.1f);
		case EItemEffectType::RestoreThirst:	return FLinearColor(0.2f, 0.6f, 1.f);
		default:								return FLinearColor::White;
	}
}

void UDetailInfoWidget::RebuildActionButtons(const FItemInstance& Selected)
{
	if (!ActionButtonsContainer)
	{
		return;
	}

	ActionButtonsContainer->ClearChildren();

	if (Selected.ItemData->Category != EItemCategory::Consumable)
	{
		return;
	}

	UButton* UseButton = NewObject<UButton>(this);
	UseButton->OnClicked.AddDynamic(this, &UDetailInfoWidget::HandleUseClicked);

	UTextBlock* UseButtonLabel = NewObject<UTextBlock>(this);
	UseButtonLabel->SetText(NSLOCTEXT("DetailInfoWidget", "UseButtonLabel", "사용"));
	UseButton->AddChild(UseButtonLabel);

	ActionButtonsContainer->AddChild(UseButton);
}

void UDetailInfoWidget::HandleDiscardClicked()
{
	UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !Inventory->bHasSelection)
	{
		return;
	}

	Inventory->ThrowItem(Inventory->SelectedSlotRef);
}

void UDetailInfoWidget::HandleUseClicked()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->UseSelectedItem();
	}
}

void UDetailInfoWidget::HandleSplitQuantityChanged(float Value)
{
	CurrentSplitCount = FMath::RoundToInt(Value);

	if (SplitQuantityText)
	{
		SplitQuantityText->SetText(FText::AsNumber(CurrentSplitCount));
	}
}

FReply UDetailInfoWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// SplitDragIcon 위에서 누른 좌클릭만 드래그 시작으로 처리한다 — 그 외 좌클릭(버튼 등)은
	// 여기서 가로채지 않고 그대로 넘겨서 각 위젯이 원래대로 처리하게 둔다.
	if (SplitDragIcon && SplitDragIcon->IsHovered() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UDetailInfoWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !Inventory->bHasSelection || CurrentSplitCount <= 0)
	{
		return;
	}

	const FItemInstance Selected = Inventory->GetSelectedItemInstance();
	if (!Selected.IsValid())
	{
		return;
	}

	UInventoryDragDropOperation* DragOp = NewObject<UInventoryDragDropOperation>(this);
	DragOp->SourceSlotRef = Inventory->SelectedSlotRef;
	DragOp->Count = CurrentSplitCount;
	DragOp->Pivot = EDragPivot::CenterCenter;

	UImage* DragVisual = NewObject<UImage>(this);
	if (UTexture2D* Icon = Selected.ItemData->Icon.LoadSynchronous())
	{
		DragVisual->SetBrushFromTexture(Icon);
	}
	DragVisual->SetDesiredSizeOverride(FVector2D(64.f, 64.f));
	DragVisual->SetRenderOpacity(0.6f);
	DragOp->DefaultDragVisual = DragVisual;

	OutOperation = DragOp;
}
