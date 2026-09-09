


#include "Widget/Inventory/DetailInfoWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Widget/Inventory/InventoryDragDropOperation.h"
#include "Component/WarehouseInventoryComponent.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/PanelWidget.h"
#include "Data/Item/ItemDataBase.h"
#include "Data/Item/EquipmentItemData.h"
#include "Data/Item/HeldItemData.h"
#include "Data/Item/ConsumableItemData.h"
#include "GameFramework/Pawn.h"
#include "Character/ActionPlayerController.h"

void UDetailInfoWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->OnPossessedCharChange.AddDynamic(this, &UDetailInfoWidget::RebindInventory);
	}

	if (DiscardButton)
	{
		DiscardButton->OnClicked.AddDynamic(this, &UDetailInfoWidget::HandleDiscardClicked);
	}

	if (UseButton)
	{
		UseButton->OnClicked.AddDynamic(this, &UDetailInfoWidget::HandleUseClicked);
	}

	if (SplitQuantitySlider)
	{
		SplitQuantitySlider->OnValueChanged.AddDynamic(this, &UDetailInfoWidget::HandleSplitQuantityChanged);
	}

	RebindInventory();
}

void UDetailInfoWidget::NativeDestruct()
{
	UnbindInventoryDelegates();
	UnbindWarehouse();

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->OnPossessedCharChange.RemoveDynamic(this, &UDetailInfoWidget::RebindInventory);
	}

	Super::NativeDestruct();
}

void UDetailInfoWidget::BindWarehouse(UWarehouseInventoryComponent* Warehouse)
{
	UnbindWarehouse();

	BoundWarehouse = Warehouse;

	if (Warehouse)
	{
		Warehouse->OnInventoryChanged.AddDynamic(this, &UDetailInfoWidget::HandleWarehouseChanged);
		Warehouse->OnSelectionChanged.AddDynamic(this, &UDetailInfoWidget::HandleWarehouseChanged);
	}

	RefreshDisplay();
}

void UDetailInfoWidget::UnbindWarehouse()
{
	if (UWarehouseInventoryComponent* Warehouse = BoundWarehouse.Get())
	{
		Warehouse->OnInventoryChanged.RemoveDynamic(this, &UDetailInfoWidget::HandleWarehouseChanged);
		Warehouse->OnSelectionChanged.RemoveDynamic(this, &UDetailInfoWidget::HandleWarehouseChanged);
	}

	BoundWarehouse = nullptr;
	RefreshDisplay();
}

void UDetailInfoWidget::HandleWarehouseChanged()
{
	RefreshDisplay();
}

void UDetailInfoWidget::UnbindInventoryDelegates()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UDetailInfoWidget::HandleInventoryChanged);
		Inventory->OnSelectionChanged.RemoveDynamic(this, &UDetailInfoWidget::HandleInventoryChanged);
	}
}

void UDetailInfoWidget::RebindInventory()
{
	UnbindInventoryDelegates();
	BoundInventory = nullptr;

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UInventoryComponent* Inventory = OwningPawn->FindComponentByClass<UInventoryComponent>())
		{
			BoundInventory = Inventory;
			Inventory->OnInventoryChanged.AddDynamic(this, &UDetailInfoWidget::HandleInventoryChanged);
			Inventory->OnSelectionChanged.AddDynamic(this, &UDetailInfoWidget::HandleInventoryChanged);
		}
	}

	HandleInventoryChanged();
}

void UDetailInfoWidget::HandleInventoryChanged()
{
	RefreshDisplay();
}

void UDetailInfoWidget::RefreshDisplay()
{
	// 창고 쪽 선택이 있으면 그걸 우선 보여준다 — UWarehouseWidget::HandlePlayerSlotClicked가
	// 플레이어 슬롯을 클릭할 때마다 창고 선택을 지워주므로, 실제로는 둘 중 하나만 선택된 상태다.
	UWarehouseInventoryComponent* Warehouse = BoundWarehouse.Get();
	UInventoryComponent* Inventory = BoundInventory.Get();

	const bool bShowingWarehouseItem = Warehouse && Warehouse->bHasSelection;
	const FItemInstance Selected = bShowingWarehouseItem
		? Warehouse->GetSelectedItemInstance()
		: (Inventory ? Inventory->GetSelectedItemInstance() : FItemInstance());

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
			// bMatchSize=true — SizeBox_1의 슬롯 정렬을 Center로 바꿔 늘어나지 않게 했으므로,
			// 브러시의 ImageSize도 실제 텍스처 해상도를 반영해야 "원본 크기"로 보인다
			// (false로 두면 WBP 디자이너의 placeholder 크기가 그대로 남는다).
			IconImage->SetBrushFromTexture(LoadedIcon, true);
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
	// 창고 아이템은 분할 드래그 대상이 아니다(NativeOnDragDetected가 SelectedSlotRef 기준으로
	// 플레이어 인벤토리에서만 꺼내오므로) — 항상 숨긴다.
	const bool bCanSplit = !bShowingWarehouseItem && (ItemData->MaxStackSize > 1) && (Selected.StackCount > 1);
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
	RebuildActionButtons(Selected, bShowingWarehouseItem);
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
			AddStatTextRow(Modifier.StatType, Modifier.Value);
		}
	}
	else if (const UHeldItemData* HeldData = Cast<UHeldItemData>(Selected.ItemData))
	{
		for (const FEquipmentStatModifier& Modifier : HeldData->StatModifiers)
		{
			AddStatTextRow(Modifier.StatType, Modifier.Value);
		}

		// Damage/OreGathering/WoodGathering/FleshGathering은 StatModifiers 배열이 아니라
		// UHeldItemData의 전용 필드라 EEquipmentStatType으로 표현할 수 없다 — 요청된 고정
		// 순서대로 나열하되, 0은 "이 도구에는 해당 없음"을 뜻하므로(헤더 주석 참고) 건너뛴다.
		if (HeldData->Damage != 0.f)
		{
			AddNamedStatRow(NSLOCTEXT("DetailInfoWidget", "StatDamage", "Damage"), HeldData->Damage);
		}
		if (HeldData->OreGathering != 0.f)
		{
			AddNamedStatRow(NSLOCTEXT("DetailInfoWidget", "StatOreGathering", "Ore Gathering"), HeldData->OreGathering);
		}
		if (HeldData->WoodGathering != 0.f)
		{
			AddNamedStatRow(NSLOCTEXT("DetailInfoWidget", "StatWoodGathering", "Wood Gathering"), HeldData->WoodGathering);
		}
		if (HeldData->FleshGathering != 0.f)
		{
			AddNamedStatRow(NSLOCTEXT("DetailInfoWidget", "StatFleshGathering", "Flesh Gathering"), HeldData->FleshGathering);
		}
		/// 최대 내구도는 포함 확정시 주석 해제
		//if (HeldData->MaxDurability != 0.f)
		//{
		//	AddNamedStatRow(NSLOCTEXT("DetailInfoWidget", "MaxDurability", "Max Durability"), HeldData->MaxDurability);
		//}
		
	}
	else if (const UConsumableItemData* ConsumableData = Cast<UConsumableItemData>(Selected.ItemData))
	{
		for (const FItemEffect& Effect : ConsumableData->Effects)
		{
			AddEffectTextRow(Effect);
		}
	}
}

void UDetailInfoWidget::AddStatTextRow(EEquipmentStatType StatType, float Value)
{
	const FText StatLabel = StaticEnum<EEquipmentStatType>()->GetDisplayNameTextByValue(static_cast<int64>(StatType));
	AddNamedStatRow(StatLabel, Value);
}

void UDetailInfoWidget::AddNamedStatRow(const FText& Label, float Value)
{
	if (!InfoRowsContainer)
	{
		return;
	}

	// UParameterBarWidget을 런타임에 새로 만들어 붙이는 방식은 이 프로젝트 환경에서 내부
	// 구조가 제대로 안 그려지는 문제가 있어(원인 미확정) 포기하고, 소비 효과(AddEffectTextRow)와
	// 동일한, 이미 검증된 텍스트 방식으로 통일했다.
	UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>();
	Row->SetText(FText::Format(NSLOCTEXT("DetailInfoWidget", "StatRowFormat", "{0}: {1}"), Label, FText::AsNumber(Value)));

	InfoRowsContainer->AddChild(Row);
}

void UDetailInfoWidget::AddEffectTextRow(const FItemEffect& Effect)
{
	if (!InfoRowsContainer)
	{
		return;
	}

	UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>();
	const FText EffectLabel = StaticEnum<EItemEffectType>()->GetDisplayNameTextByValue(static_cast<int64>(Effect.EffectType));
	Row->SetText(FText::Format(NSLOCTEXT("DetailInfoWidget", "EffectRowFormat", "{0} +{1}"), EffectLabel, FText::AsNumber(Effect.Magnitude)));

	/// 텍스트 색상을 지정하고 싶다면 아래 코드를 수정해서 사용.
	//Row->SetColorAndOpacity(FSlateColor(GetEffectColor(Effect.EffectType)));

	InfoRowsContainer->AddChild(Row);
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

void UDetailInfoWidget::RebuildActionButtons(const FItemInstance& Selected, bool bIsWarehouseItem)
{
	// 버튼은 런타임에 만들지 않고 WBP에 미리 배치해둔 걸(UseButton 등) 카테고리에 따라
	// 보이거나 숨기기만 한다 — 나중에 액션 버튼이 늘어나도 같은 방식으로 추가하면 된다.
	if (UseButton)
	{
		const bool bIsConsumable = !bIsWarehouseItem && Selected.ItemData && Selected.ItemData->Category == EItemCategory::Consumable;
		UseButton->SetVisibility(bIsConsumable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 버리기(ThrowItem)도 SelectedSlotRef 기준으로 플레이어 인벤토리에서만 동작하므로 창고
	// 아이템을 보고 있을 때는 숨긴다.
	if (DiscardButton)
	{
		DiscardButton->SetVisibility(bIsWarehouseItem ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UDetailInfoWidget::HandleDiscardClicked()
{
	UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !Inventory->bHasSelection)
	{
		return;
	}

	Inventory->Server_ThrowItem(Inventory->SelectedSlotRef, 0);
}

void UDetailInfoWidget::HandleUseClicked()
{
	UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !Inventory->bHasSelection)
	{
		return;
	}

	Inventory->Server_UseSelectedItem(Inventory->SelectedSlotRef);
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

	// 상세창의 분할 드래그에도 출처와 아이템을 넣어 모닥불이 허용 여부와 지정 수량 처리 가능하게 가공
	DragOp->SourceType = EItemDragSourceType::PlayerInventory;
	DragOp->DraggedItemData = Selected.ItemData;
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
