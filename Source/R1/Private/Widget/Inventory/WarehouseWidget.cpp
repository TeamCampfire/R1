// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Inventory/WarehouseWidget.h"
#include "Widget/Inventory/InventorySlotWidget.h"
#include "Widget/Inventory/InventoryWidget.h"
#include "Widget/Inventory/BeltBarWidget.h"
#include "Widget/Inventory/DetailInfoWidget.h"
#include "Component/WarehouseInventoryComponent.h"
#include "Components/PanelWidget.h"
#include "GameFramework/Pawn.h"
#include "Character/ActionPlayerController.h"
#include "Character/ActionCharacter.h"
#include "Components/TextBlock.h"

void UWarehouseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WarehouseLabel)
	{
		DefaultWarehouseLabel = WarehouseLabel->GetText();
	}

	// 시작은 항상 숨김 — MainHUDWidget::OpenWarehousePanel이 열 때만 보이게 한다.
	SetVisibility(ESlateVisibility::Collapsed);
}

void UWarehouseWidget::NativeDestruct()
{
	UnbindDelegates();

	Super::NativeDestruct();
}

void UWarehouseWidget::OpenWarehouse(UWarehouseInventoryComponent* Warehouse, UInventoryWidget* PlayerInventoryWidget, UBeltBarWidget* PlayerBeltBarWidget)
{
	UnbindDelegates();

	BoundWarehouse = Warehouse;

	// Looting: 창고 UI 라벨 바꾸기
	if (WarehouseLabel)
	{
		// 창고 컴포넌트를 달고 있는 게 플레이어 캐릭터이면 캐릭터 기억
		const AActionCharacter* Character =
			Warehouse
			? Cast<AActionCharacter>(Warehouse->GetOwner())
			: nullptr;

		const bool bIsCorpseStorage = Character && Character->GetCorpseStorageComponent() == Warehouse;	// 시체 창고인지 확인

		// 시체 창고면 UI 라벨 내용 시체로 바꾸기
		// 일반 창고면 원래 내용으로 두기
		// -> 시체 창고를 열었다가 일반 창고를 열었을 때 라벨이 시체로 유지되는 것을 방지하기 위해서
		//    창고열 때마다 SetText해주기
		WarehouseLabel->SetText(
			bIsCorpseStorage
			? FText::FromString(TEXT("시체"))
			: DefaultWarehouseLabel);
	}

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		BoundPlayerInventory = OwningPawn->FindComponentByClass<UInventoryComponent>();
	}

	BoundInventoryWidget = PlayerInventoryWidget;
	BoundBeltBarWidget = PlayerBeltBarWidget;
	BindCrossDropOnPlayerWidgets();

	if (Warehouse)
	{
		Warehouse->OnInventoryChanged.AddDynamic(this, &UWarehouseWidget::HandleInventoryChanged);
		Warehouse->OnSelectionChanged.AddDynamic(this, &UWarehouseWidget::HandleInventoryChanged);
	}

	if (PlayerInventoryWidget)
	{
		if (UDetailInfoWidget* DetailInfo = PlayerInventoryWidget->GetDetailInfoWidget())
		{
			DetailInfo->BindWarehouse(Warehouse);
		}
	}

	// 이 위젯의 호스트 슬롯(WBP_MainHUD_cyh)이 화면 전체를 채우도록 앵커돼 있으므로(내부 콘텐츠는
	// 우측에만 그려짐), Visible로 켜면 콘텐츠가 없는 빈 영역까지 이 위젯이 히트테스트를 가로채서
	// 뒤에 있는 InventoryWidget/BeltBarWidget 슬롯이 전부 클릭·드래그를 못 받게 된다.
	// SelfHitTestInvisible로 켜야 빈 영역은 그대로 통과시키고 실제 자식(그리드/라벨)만 반응한다.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	HandleInventoryChanged();
}

void UWarehouseWidget::CloseWarehouse()
{
	UnbindDelegates();
	SetVisibility(ESlateVisibility::Collapsed);

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->SetWarehouseInputState(false);
	}
}

void UWarehouseWidget::UnbindDelegates()
{
	if (UWarehouseInventoryComponent* Warehouse = BoundWarehouse.Get())
	{
		Warehouse->OnInventoryChanged.RemoveDynamic(this, &UWarehouseWidget::HandleInventoryChanged);
		Warehouse->OnSelectionChanged.RemoveDynamic(this, &UWarehouseWidget::HandleInventoryChanged);
	}

	if (UInventoryWidget* InvWidget = BoundInventoryWidget.Get())
	{
		if (UDetailInfoWidget* DetailInfo = InvWidget->GetDetailInfoWidget())
		{
			DetailInfo->UnbindWarehouse();
		}
	}

	UnbindCrossDropOnPlayerWidgets();

	BoundPlayerInventory = nullptr;
	BoundWarehouse = nullptr;
}

void UWarehouseWidget::BindCrossDropOnPlayerWidgets()
{
	if (UInventoryWidget* InvWidget = BoundInventoryWidget.Get())
	{
		for (UInventorySlotWidget* SlotWidget : InvWidget->GetMainSlotWidgets())
		{
			if (SlotWidget)
			{
				SlotWidget->OnSlotDroppedCross.AddDynamic(this, &UWarehouseWidget::HandleSlotDroppedCross);
				SlotWidget->OnSlotClicked.AddDynamic(this, &UWarehouseWidget::HandlePlayerSlotClicked);
			}
		}
	}

	if (UBeltBarWidget* BeltWidget = BoundBeltBarWidget.Get())
	{
		for (UInventorySlotWidget* SlotWidget : BeltWidget->GetBeltSlotWidgets())
		{
			if (SlotWidget)
			{
				SlotWidget->OnSlotDroppedCross.AddDynamic(this, &UWarehouseWidget::HandleSlotDroppedCross);
				SlotWidget->OnSlotClicked.AddDynamic(this, &UWarehouseWidget::HandlePlayerSlotClicked);
			}
		}
	}
}

void UWarehouseWidget::UnbindCrossDropOnPlayerWidgets()
{
	if (UInventoryWidget* InvWidget = BoundInventoryWidget.Get())
	{
		for (UInventorySlotWidget* SlotWidget : InvWidget->GetMainSlotWidgets())
		{
			if (SlotWidget)
			{
				SlotWidget->OnSlotDroppedCross.RemoveDynamic(this, &UWarehouseWidget::HandleSlotDroppedCross);
				SlotWidget->OnSlotClicked.RemoveDynamic(this, &UWarehouseWidget::HandlePlayerSlotClicked);
			}
		}
	}

	if (UBeltBarWidget* BeltWidget = BoundBeltBarWidget.Get())
	{
		for (UInventorySlotWidget* SlotWidget : BeltWidget->GetBeltSlotWidgets())
		{
			if (SlotWidget)
			{
				SlotWidget->OnSlotDroppedCross.RemoveDynamic(this, &UWarehouseWidget::HandleSlotDroppedCross);
				SlotWidget->OnSlotClicked.RemoveDynamic(this, &UWarehouseWidget::HandlePlayerSlotClicked);
			}
		}
	}

	BoundInventoryWidget = nullptr;
	BoundBeltBarWidget = nullptr;
}

void UWarehouseWidget::HandleInventoryChanged()
{
	RebuildSlots();
}

void UWarehouseWidget::RebuildSlots()
{
	UWarehouseInventoryComponent* Warehouse = BoundWarehouse.Get();
	if (!SlotWidgetClass || !Warehouse)
	{
		return;
	}

	auto NoSelection = [](const FInventorySlotRef&) { return false; };

	auto IsSelectedFn = [Warehouse](const FInventorySlotRef& SlotRef)
	{
		return Warehouse->IsSlotSelected(SlotRef.Index);
	};

	auto BindWarehouseDropped = [this](UInventorySlotWidget* SlotWidget)
	{
		SlotWidget->OnSlotDropped.AddDynamic(this, &UWarehouseWidget::HandleSlotDropped);
		SlotWidget->OnSlotDroppedCross.AddDynamic(this, &UWarehouseWidget::HandleSlotDroppedCross);
		SlotWidget->OnSlotClicked.AddDynamic(this, &UWarehouseWidget::HandleSlotClicked);
		SlotWidget->OnSlotRightClicked.AddDynamic(this, &UWarehouseWidget::HandleSlotRightClicked);
	};

	// Category는 EnsureGridSlots가 FInventorySlotRef를 만들기 위한 형식상의 값일 뿐이다 —
	// 창고는 UInventoryComponent::EInventorySlotCategory 개념이 없으므로 Main을 자리채움으로
	// 쓴다. HandleSlotDroppedCross는 이 Category를 쓰지 않고 .Index만 창고 슬롯 번호로 읽는다.
	UInventorySlotWidget::EnsureGridSlots(this, SlotWidgetClass, WarehouseSlotContainer, EInventorySlotCategory::Main,
		Warehouse->StorageSlots, GridColumns, BindWarehouseDropped, IsSelectedFn, NoSelection, WarehouseSlotWidgets, WarehouseContainerId);
}

void UWarehouseWidget::HandleSlotDropped(FInventorySlotRef FromSlot, FInventorySlotRef ToSlot, int32 Count, bool bAutoHalfSplitOnEmptyTarget)
{
	// ContainerId가 같을 때만 발생하므로 항상 창고 그리드 내부 이동이다.
	if (FromSlot.Index == ToSlot.Index)
	{
		return;
	}

	UInventoryComponent* PlayerInventory = BoundPlayerInventory.Get();
	UWarehouseInventoryComponent* Warehouse = BoundWarehouse.Get();
	if (!PlayerInventory || !Warehouse)
	{
		return;
	}

	PlayerInventory->Server_TransferWithinWarehouse(Warehouse, FromSlot.Index, ToSlot.Index, Count, bAutoHalfSplitOnEmptyTarget);
}

void UWarehouseWidget::HandleSlotClicked(FInventorySlotRef SlotRef)
{
	if (UWarehouseInventoryComponent* Warehouse = BoundWarehouse.Get())
	{
		Warehouse->SelectSlot(SlotRef.Index);
	}

	// HandlePlayerSlotClicked의 반대 방향 — 창고 슬롯을 새로 선택했으니 플레이어 쪽(메인/벨트)
	// 선택은 풀어서 두 그리드에 동시에 파란 테두리가 남지 않게 한다.
	if (UInventoryComponent* PlayerInventory = BoundPlayerInventory.Get())
	{
		PlayerInventory->ClearSelection();
	}
}

void UWarehouseWidget::HandlePlayerSlotClicked(FInventorySlotRef SlotRef)
{
	// 플레이어 쪽 슬롯이 새로 선택됐으니 창고 쪽 선택은 풀어서, DetailInfoWidget이 항상 최근에
	// 클릭된 슬롯 하나만 보여주게 한다(UInventoryWidget/UBeltBarWidget이 자기 쪽 선택은 이미
	// 각자의 HandleSlotClicked에서 Inventory->SelectSlot으로 처리하므로 여기선 창고만 정리한다).
	if (UWarehouseInventoryComponent* Warehouse = BoundWarehouse.Get())
	{
		Warehouse->ClearSelection();
	}
}

void UWarehouseWidget::HandleSlotRightClicked(FInventorySlotRef SlotRef)
{
	// 창고 슬롯 우클릭 → 플레이어 인벤토리로 빠른 이동. 벨트/메인 우선순위는
	// UInventoryComponent::QuickMoveFromWarehouse가 카테고리를 보고 결정한다.
	UInventoryComponent* PlayerInventory = BoundPlayerInventory.Get();
	UWarehouseInventoryComponent* Warehouse = BoundWarehouse.Get();
	if (!PlayerInventory || !Warehouse)
	{
		return;
	}

	PlayerInventory->Server_QuickMoveFromWarehouse(Warehouse, SlotRef.Index);
}

void UWarehouseWidget::HandleSlotDroppedCross(int32 FromContainerId, FInventorySlotRef FromSlot, int32 ToContainerId, FInventorySlotRef ToSlot, int32 Count)
{
	UInventoryComponent* PlayerInventory = BoundPlayerInventory.Get();
	UWarehouseInventoryComponent* Warehouse = BoundWarehouse.Get();
	if (!PlayerInventory || !Warehouse)
	{
		return;
	}

	const bool bToWarehouse = (FromContainerId == PlayerContainerId);
	const FInventorySlotRef& PlayerSlot = bToWarehouse ? FromSlot : ToSlot;
	const int32 WarehouseIndex = bToWarehouse ? ToSlot.Index : FromSlot.Index;

	PlayerInventory->Server_TransferWithWarehouse(Warehouse, PlayerSlot, WarehouseIndex, Count, bToWarehouse);
}
