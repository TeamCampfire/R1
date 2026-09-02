// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Inventory/InventoryWidget.h"
#include "Widget/Inventory/InventorySlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Data/Item/ItemDataBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Character/ActionPlayerController.h"

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

void UInventoryWidget::ClearSelection()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->ClearSelection();
	}
}

void UInventoryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!IsDesignTime() || !SlotWidgetClass)
	{
		return;
	}

	// 디자이너에는 살아있는 UInventoryComponent가 없다(BoundInventory는 PIE에서만 채워짐) —
	// 클래스 디폴트(CDO)의 슬롯 개수만 빌려와 더미 배열로 그리드를 미리 채워서 레이아웃/스케일링을
	// 눈으로 확인할 수 있게 한다. 선택/클릭 관련 콜백은 디자이너에서 의미가 없으므로 전부 무시.
	const UInventoryComponent* DefaultInventory = GetDefault<UInventoryComponent>();
	auto NoOpCreated = [](UInventorySlotWidget*) {};
	auto NoSelection = [](const FInventorySlotRef&) { return false; };

	TArray<FItemInstance> PreviewEquipmentSlots;
	PreviewEquipmentSlots.SetNum(DefaultInventory->EquipmentSlotCount);
	UInventorySlotWidget::EnsureGridSlots(this, SlotWidgetClass, EquipmentSlotContainer, EInventorySlotCategory::Equipment, PreviewEquipmentSlots, GridColumns, NoOpCreated, NoSelection, NoSelection, EquipmentSlotWidgets);

	TArray<FItemInstance> PreviewMainSlots;
	PreviewMainSlots.SetNum(DefaultInventory->MainSlotCount);
	UInventorySlotWidget::EnsureGridSlots(this, SlotWidgetClass, MainSlotContainer, EInventorySlotCategory::Main, PreviewMainSlots, GridColumns, NoOpCreated, NoSelection, NoSelection, MainSlotWidgets);
}

void UInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->OnPossessedCharChange.AddDynamic(this, &UInventoryWidget::RebindInventory);
	}

	RebindInventory();
}

void UInventoryWidget::NativeDestruct()
{
	UnbindInventoryDelegates();

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->OnPossessedCharChange.RemoveDynamic(this, &UInventoryWidget::RebindInventory);
	}

	Super::NativeDestruct();
}

void UInventoryWidget::UnbindInventoryDelegates()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
		Inventory->OnSelectionChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
	}
}

void UInventoryWidget::RebindInventory()
{
	UnbindInventoryDelegates();
	BoundInventory = nullptr;

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

	// 손에 듦(HeldBeltIndex) 강조는 벨트 전용 개념이라 장비/메인 패널에는 해당 없음.
	auto IsHeldFn = [](const FInventorySlotRef&) { return false; };

	UInventorySlotWidget::EnsureGridSlots(this, SlotWidgetClass, EquipmentSlotContainer, EInventorySlotCategory::Equipment, Inventory->EquipmentSlots, GridColumns, BindDropped, IsSelectedFn, IsHeldFn, EquipmentSlotWidgets);
	UInventorySlotWidget::EnsureGridSlots(this, SlotWidgetClass, MainSlotContainer, EInventorySlotCategory::Main, Inventory->MainSlots, GridColumns, BindDropped, IsSelectedFn, IsHeldFn, MainSlotWidgets);
}

void UInventoryWidget::HandleSlotDropped(FInventorySlotRef FromSlot, FInventorySlotRef ToSlot, int32 Count, bool bAutoHalfSplitOnEmptyTarget)
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

	// Count<=0 => 전량 이동, 양수면 분할 드래그(DetailInfoWidget)에서 지정한 수량만.
	// bAutoHalfSplitOnEmptyTarget=true(휠클릭 드래그)면 빈 슬롯에 놓았을 때 절반만 옮긴다.
	Inventory->Server_TransferItem(FromSlot, ToSlot, Count, bAutoHalfSplitOnEmptyTarget);
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
		Inventory->Server_QuickMoveItem(SlotRef);
	}
}

void UInventoryWidget::HandleSlotDragCancelled(FInventorySlotRef SlotRef)
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->Server_ThrowItem(SlotRef, 0);
	}
}
