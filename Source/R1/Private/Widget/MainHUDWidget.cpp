// Fill out your copyright notice in the Description page of Project Settings.

#include "Widget/MainHUDWidget.h"
#include "Widget/Crafting/CraftingWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Widget/Inventory/InventoryWidget.h"
#include "Widget/Inventory/WarehouseWidget.h"
#include "Component/WarehouseInventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Animation/WidgetAnimation.h"
#include "Widget/DeathScreenOverlayWidget.h"
#include "Character/ActionPlayerController.h"
#include "Character/ActionCharacter.h"
#include "Component/StatComponent.h"
#include "Widget/BuildingSystem/BuildingDurabilityWidget.h"
#include "Widget/Campfire/CampfireWidget.h"
#include "Item/PlaceableItem/Campfire.h"
#include "Component/InteractionComponent.h"
#include "GameFramework/Pawn.h"
#include "Widget/SurvivalStatBarsWidget.h"

void UMainHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//UE_LOG(LogTemp, Warning, TEXT("[InvToggle] MainHUDWidget::NativeOnInitialized. InventoryWidget=%s"), *GetNameSafe(InventoryWidget));

	CachedController = Cast<AActionPlayerController>(GetOwningPlayer());
	if (CachedController)
	{
		CachedController->OnPossessedCharChange.AddDynamic(
			this,
			&UMainHUDWidget::OnPossessedCharChange
		);
	}

	// 캐릭터에 바인드
	BindDelegatesToNewChar();

	// 사망 창 숨기고 시작
	HideDeathScreen();

	// 인벤토리 패널은 토글로 열리는 화면이라 처음엔 닫힌 채로 시작한다.
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 게임 시작 시 모닥불 UI가 보이지 않도록 숨김 처리
	if (CampfireWidget)
		CampfireWidget->SetVisibility(ESlateVisibility::Collapsed);

	// 게임 시작 시 제작창이 보이지 않도록 숨김 처리
	if (CraftingWidget)
		CraftingWidget->SetVisibility(ESlateVisibility::Collapsed);

	// 게임을 시작했을 때 이전 디자인용 테스트 문구가 화면에 표시되지 않도록 숨겨요
	if (true == IsValid(Border_BuildingPlacementMessage))
		Border_BuildingPlacementMessage->SetVisibility(ESlateVisibility::Collapsed);
}

void UMainHUDWidget::NativeDestruct()
{
	// 제작, 모닥불 UI 열려 있으면 닫기
	CloseCraftingPanel();
	CloseCampfire();

	// 위젯이 제거될 때 예약된 타이머가 남아 제거된 위젯을 다시 호출하지 않도록 정리해줍니다
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(BuildingPlacementMessageTimerHandle);

	// 위젯이 제거될 때 실행 중인 건축 실패 메시지 애니메이션도 정지
	if (true == IsValid(Anim_BuildingPlacementMessage))
		StopAnimation(Anim_BuildingPlacementMessage);

	Super::NativeDestruct();
}

void UMainHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	CheckWarehouseAutoClose();
	CheckCampfireAutoClose();
}

// 거리 이탈이나 대상 소멸 시 모닥불 + 인벤토리를 닫고, 입력 상태 복구
void UMainHUDWidget::CheckCampfireAutoClose()
{
	if (!bCampfireSessionOpen) return;

	ACampfire* Campfire = OpenCampfireActor.Get();
	APawn* Pawn = GetOwningPlayerPawn();

	// 서버의 상호작용 허용 조건과 같은 기준으로 검사
	if (!IsValid(Campfire) || !IsValid(Pawn)
		|| !IInteractableInterface::Execute_CanInteract(Campfire, Pawn))
	{
		if (IsInventoryPanelOpen())
		{
			// 인벤토리 닫기 경로에서 모닥불 연결과 선택 상태도 함께 정리한다.
			ToggleInventoryPanel();
			if (CachedController) CachedController->SetInventoryInputState(false);
		}
		else
		{
			CloseCampfire();
		}
	}
}

void UMainHUDWidget::CheckWarehouseAutoClose()
{
	if (!IsWarehousePanelOpen())
	{
		return;
	}

	UWarehouseInventoryComponent* Warehouse = WarehouseWidget->GetBoundWarehouse();
	APawn* OwningPawn = GetOwningPlayerPawn();
	// Looting: WarehouseComponent에 위치 반환 함수를 만들어 사용하기 때문에 더이상 창고 액터를 기억할 필요가 없어서 주석 처리
	//AActor* WarehouseOwner = Warehouse ? Warehouse->GetOwner() : nullptr;
	//if (!Warehouse || !OwningPawn || !WarehouseOwner)
	if (!Warehouse || !OwningPawn)
	{
		return;
	}

	const float DistSq = FVector::DistSquared(OwningPawn->GetActorLocation(), Warehouse->GetInteractionLocation());
	if (DistSq > FMath::Square(Warehouse->MaxInteractDistance))
	{
		CloseWarehousePanel();
	}
}

void UMainHUDWidget::HideBuildingPlacementMessage()
{
	if (true == IsValid(Border_BuildingPlacementMessage)) // 메시지 보더 숨겨요
		Border_BuildingPlacementMessage->SetVisibility(ESlateVisibility::Collapsed);
}

void UMainHUDWidget::ShowDeathScreen()
{
	if (DeathScreenBorder)
		DeathScreenBorder->SetVisibility(ESlateVisibility::Visible);
	if (DeathScreenOverlay)
		DeathScreenOverlay->SetVisibility(ESlateVisibility::Visible);

	if (HUDPanel)
		HUDPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UMainHUDWidget::HideDeathScreen()
{
	if (DeathScreenBorder)
		DeathScreenBorder->SetVisibility(ESlateVisibility::Collapsed);
	if (DeathScreenOverlay)
		DeathScreenOverlay->SetVisibility(ESlateVisibility::Collapsed);

	if (HUDPanel)
		HUDPanel->SetVisibility(ESlateVisibility::Visible);
}

void UMainHUDWidget::OnPossessedCharChange()
{
	// 조종 대상 바뀌면 제작, 모닥불 UI 무조건 닫기
	CloseCraftingPanel();
	CloseCampfire();

	// 조종 대상이 바뀌면(부활로 새 캐릭터를 빙의하는 경우 등) 열려있던 창고 세션은 무조건 끊는다 —
	// UWarehouseWidget은 InventoryWidget/BeltBarWidget과 달리 OnPossessedCharChange를 직접
	// 구독하지 않고 OpenWarehouse가 호출된 시점의 폰에서만 BoundPlayerInventory를 찾아두므로,
	// 여기서 안 끊어주면 창고가 열린 채로 부활했을 때 죽기 전 캐릭터의 인벤토리 컴포넌트를 계속
	// 참조하게 된다(화면에 보이는 메인/벨트 슬롯은 새 캐릭터 걸로 이미 바뀌었는데 창고 이동만
	// 옛 캐릭터 걸로 나가는 불일치가 생김). CloseWarehousePanel은 OpenUIPanelCount를 실제로
	// 감소시키므로 열려있을 때만 호출해야 한다.
	if (IsWarehousePanelOpen())
	{
		CloseWarehousePanel();
	}

	BindDelegatesToNewChar();
	HideDeathScreen();
}

void UMainHUDWidget::OnDeath()
{
	// 사망 시 제작, 모닥불 UI 닫기 처리
	CloseCraftingPanel();
	CloseCampfire();
	ShowDeathScreen();
	CachedController->SetShowMouseCursor(true);		// 마우스 커서 보이기
}

void UMainHUDWidget::OnRespawnClicked()
{
	CachedController->SetShowMouseCursor(false);	// 마우스 커서 숨기기
	CachedController->RequestSpawn();
}

void UMainHUDWidget::BindDelegatesToNewChar()
{
	if (AActionCharacter* Character = Cast<AActionCharacter>(CachedController->GetPawn()))
	{
		if (UStatComponent* StatComp = (Cast<IStatInterface>(Character))->GetStatComponent())
		{
			if (StatComp->OnDeath.IsAlreadyBound(this, &UMainHUDWidget::OnDeath))
			{
				StatComp->OnDeath.RemoveDynamic(this, &UMainHUDWidget::OnDeath);
			}

			StatComp->OnDeath.AddDynamic(this, &UMainHUDWidget::OnDeath);
		}
	}
	if (DeathScreenOverlay->OnRespawnClicked.IsAlreadyBound(this, &UMainHUDWidget::OnRespawnClicked))
	{
		DeathScreenOverlay->OnRespawnClicked.RemoveDynamic(this, &UMainHUDWidget::OnRespawnClicked);
	}
	DeathScreenOverlay->OnRespawnClicked.AddDynamic(this, &UMainHUDWidget::OnRespawnClicked);
}

bool UMainHUDWidget::ToggleInventoryPanel()
{
	//UE_LOG(LogTemp, Warning, TEXT("[InvToggle] ToggleInventoryPanel called. InventoryWidget=%s, CurrentlyOpen=%d"),
	//	*GetNameSafe(InventoryWidget), IsInventoryPanelOpen());

	if (!InventoryWidget)
	{
		return false;
	}

	const bool bNewOpenState = !IsInventoryPanelOpen();

	// 인벤토리창 열 때 제작창 닫기
	if (bNewOpenState)
		CloseCraftingPanel();

	// InventoryWidget의 호스트 슬롯도 화면 전체를 채우도록 앵커돼 있다 — Visible로 켜면 콘텐츠
	// 없는 빈 영역이 뒤(z-order상 InventoryWidget보다 아래인 위젯)로 클릭을 전달하지 않고 가로채
	// 버린다. SelfHitTestInvisible로 켜야 빈 영역은 통과시키고 실제 자식(슬롯/버튼)만 반응한다.
	InventoryWidget->SetVisibility(bNewOpenState ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	if (!bNewOpenState)
	{
		CloseCampfire();

		// 닫을 때는 선택 상태(파란 테두리)도 같이 초기화 — 다음에 열었을 때 예전 선택이 남아있지 않게.
		InventoryWidget->ClearSelection();

		// 창고를 옆에 펼쳐놓고 보다가 인벤토리 토글 키로 닫으면 창고도 같이 닫는다 — 두 패널이
		// 항상 세트로 열리고 닫히는 게 아니라(인벤토리만 먼저 열어뒀을 수도 있음), 인벤토리를
		// 닫는 시점엔 창고만 따로 남겨둘 이유가 없다.
		if (IsWarehousePanelOpen())
		{
			CloseWarehousePanel();
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("[InvToggle] -> new open state=%d, resulting visibility=%d"),
	//	bNewOpenState, (int32)InventoryWidget->GetVisibility());

	return bNewOpenState;
}

// 인벤토리, 모닥불 위젯, 상호작용 컴포넌트의 대상을 맞추고 세션 시작 시 열기 소리 재생
void UMainHUDWidget::OpenCampfire(ACampfire* Campfire)
{
	if (!IsValid(Campfire) || !InventoryWidget || !CampfireWidget)
		return;

	// 안전장치: 모닥불 UI 열 때 제작 UI 닫기
	CloseCraftingPanel();

	const bool bPlayOpenSound = !bCampfireSessionOpen;		// 모닥불 UI를 새로 열 때만 열림음 재상하도록 제어하기 위해 사용
	const bool bInventoryWasOpen = IsInventoryPanelOpen();	// UI 입력 모드 제어를 위해 사용

	InventoryWidget->SetVisibility(ESlateVisibility::Visible);
	InventoryWidget->SetActiveCampfire(Campfire);

	CampfireWidget->BindCampfire(Campfire);
	CampfireWidget->SetVisibility(ESlateVisibility::Visible);

	OpenCampfireActor = Campfire;
	bCampfireSessionOpen = true;

	if (CachedController)
	{
		if (APawn* Pawn = CachedController->GetPawn())
		{
			if (UInteractionComponent* InteractionComp = Pawn->FindComponentByClass<UInteractionComponent>())
			{
				InteractionComp->SetActiveCampfire(Campfire);
			}
		}

		// 인벤토리가 닫혀 있던 경우, UI 입력 상태와 마우스 커서 활성화
		if (!bInventoryWasOpen)
			CachedController->SetInventoryInputState(true);
	}

	// 모닥불 열림음 재생
	if (bPlayOpenSound && CampfireOpenSound && CachedController && CachedController->IsLocalController())
	{
		UGameplayStatics::PlaySound2D(this, CampfireOpenSound);
	}
}

// 위젯 델리게이트와 활성 대상을 해제하고, 열린 세션에 대해서만 닫기 소리를 재생
void UMainHUDWidget::CloseCampfire()
{
	const bool bPlayCloseSound = bCampfireSessionOpen;
	if (InventoryWidget) InventoryWidget->ClearActiveCampfire();
	if (CampfireWidget)
	{
		CampfireWidget->UnbindCampfire();
		CampfireWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CachedController)
	{
		if (APawn* Pawn = CachedController->GetPawn())
		{
			if (UInteractionComponent* Interaction = Pawn->FindComponentByClass<UInteractionComponent>())
			{
				Interaction->SetActiveCampfire(nullptr);
			}
		}
	}
	bCampfireSessionOpen = false;
	OpenCampfireActor.Reset();
	if (bPlayCloseSound && CampfireCloseSound && CachedController && CachedController->IsLocalController())
	{
		UGameplayStatics::PlaySound2D(this, CampfireCloseSound);
	}
}

bool UMainHUDWidget::IsInventoryPanelOpen() const
{
	return InventoryWidget && InventoryWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

void UMainHUDWidget::OpenWarehousePanel(UWarehouseInventoryComponent* Warehouse)
{
	if (!WarehouseWidget)
	{
		return;
	}

	// 안전장치: 창고 UI 열 때 제작 UI 닫기
	CloseCraftingPanel();

	// Rust처럼 창고를 열면 내 인벤토리 패널이 옆에 같이 뜬다 — 이미 열려있으면 손대지 않는다
	// (플레이어가 직접 열어둔 상태였다면 창고를 닫아도 인벤토리는 그대로 남아있어야 하므로).
	if (InventoryWidget && !IsInventoryPanelOpen())
	{
		InventoryWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		bInventoryAutoOpenedForWarehouse = true;
	}

	WarehouseWidget->OpenWarehouse(Warehouse, InventoryWidget, BeltBarWidget);
}

void UMainHUDWidget::CloseWarehousePanel()
{
	if (!WarehouseWidget)
	{
		return;
	}

	WarehouseWidget->CloseWarehouse();

	if (bInventoryAutoOpenedForWarehouse && InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
		InventoryWidget->ClearSelection();
	}
	bInventoryAutoOpenedForWarehouse = false;
}

bool UMainHUDWidget::IsWarehousePanelOpen() const
{
	return WarehouseWidget && WarehouseWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

UWarehouseInventoryComponent* UMainHUDWidget::GetOpenWarehouse() const
{
	return (WarehouseWidget && IsWarehousePanelOpen()) ? WarehouseWidget->GetBoundWarehouse() : nullptr;
}

void UMainHUDWidget::ShowBuildingPlacementMessage(const FText& Message, float DisplayDuration)
{
	if (false == IsValid(Border_BuildingPlacementMessage) || false == IsValid(Text_BuildingPlacementMessage)) return;

	// 전달받은 설치 실패 문구로 텍스트를 갱신
	Text_BuildingPlacementMessage->SetText(Message);

	// 안내 UI가 게임 입력을 막지 않도록 마우스 입력을 받지 않는 Visibility 상태로 표시해요
	Border_BuildingPlacementMessage->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 연속 좌클릭 시 기존 페이드를 이어서 재생하지 않고 완전히 첨부터 진행
	if (true == IsValid(Anim_BuildingPlacementMessage))
	{
		StopAnimation(Anim_BuildingPlacementMessage);
		PlayAnimation(Anim_BuildingPlacementMessage, 0.0f, 1, EUMGSequencePlayMode::Forward);
	}

	UWorld* World = GetWorld();
	if (false == IsValid(World)) return;

	FTimerManager& TimerManager = World->GetTimerManager();

	// 좌클릭을 연속으로 누른 경우 이전 타이머를 제거하여 메시지가 중간에 갑자기 사라지지 않게 해요
	TimerManager.ClearTimer(BuildingPlacementMessageTimerHandle);

	// 잘못된 시간이 전달되어 즉시 사라지지 않도록 최소 표시 시간을 보장..
	const float SafeDisplayDuration = FMath::Max(DisplayDuration, 0.1f);

	TimerManager.SetTimer(
		BuildingPlacementMessageTimerHandle,
		this,
		&UMainHUDWidget::HideBuildingPlacementMessage,
		SafeDisplayDuration,
		false);
}

void UMainHUDWidget::ShowBuildingDurability(float CurrentDurability, float MaxDurability)
{
	if (false == IsValid(BuildingDurabilityWidget)) return;

	BuildingDurabilityWidget->UpdateDurability(CurrentDurability, MaxDurability);
	BuildingDurabilityWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();

	// 연속 공격 시 이전 타이머 때문에 UI가 일찍 사라지지 않도록 초기화
	TimerManager.ClearTimer(BuildingDurabilityTimerHandle);

	// 잘못된 시간이 들어와도 UI가 즉시 사라지지 않도록 최소 시간을 보장합니다.
	const float SafeDisplayDuration = FMath::Max(0.1f, 1.5f); //1.5초는 최소 UI 떠있는 시간

	TimerManager.SetTimer(
		BuildingDurabilityTimerHandle,
		this,
		&UMainHUDWidget::HideBuildingDurability,
		SafeDisplayDuration,
		false
	);
}

void UMainHUDWidget::HideBuildingDurability()
{
	// 기존 자동 숨김 타이머를 정리
	GetWorld()->GetTimerManager().ClearTimer(BuildingDurabilityTimerHandle);

	if (true == IsValid(BuildingDurabilityWidget))
		BuildingDurabilityWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UMainHUDWidget::AddPickupNotification(UItemDataBase* ItemData, int32 GainedAmount, int32 NewTotalCount)
{
	if (!SurvivalStatBarsWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PickupNotif] SurvivalStatBarsWidget not bound on MainHUDWidget — check the widget name in WBP_MainHUD matches the UPROPERTY name."));
		return;
	}
	SurvivalStatBarsWidget->AddPickupNotification(ItemData, GainedAmount, NewTotalCount);
}

void UMainHUDWidget::OpenCraftingPanel(UCraftingComponent* Crafting, AWorkbench* Bench)
{
	if (!CachedController)
	{
		UE_LOG(LogTemp, Error, TEXT("No CachedController"));
	}
	if (!CachedController->IsLocalController())
	{
		UE_LOG(LogTemp, Error, TEXT("No IsLocalController"));
	}
	if (!Crafting)
	{
		UE_LOG(LogTemp, Error, TEXT("No Crafting"));
	}
	if (!CraftingWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("No CraftingWidget"));
	}
	if (!CachedController || !CachedController->IsLocalController() || !Crafting || !CraftingWidget)
		return;

	CloseCraftingPanel();
	if (IsInventoryPanelOpen())
	{
		ToggleInventoryPanel();
		CachedController->SetInventoryInputState(false);
	}

	// HUD에 배치된 제작창 재사용
	// 매번 현재 제작 대상 연결
	CraftingWidget->BindCrafting(Crafting, Bench);
	CraftingWidget->SetIsFocusable(true);
	CraftingWidget->SetVisibility(ESlateVisibility::Visible);
	CachedController->ApplyUIInputState(true);
	CraftingWidget->SetKeyboardFocus();
}

void UMainHUDWidget::CloseCraftingPanel()
{
	if (!IsCraftingPanelOpen()) return;

	CraftingWidget->SetVisibility(ESlateVisibility::Collapsed);
	CraftingWidget->UnbindCrafting();
	if (CachedController) CachedController->ApplyUIInputState(false);
}

bool UMainHUDWidget::IsCraftingPanelOpen() const
{
	return CraftingWidget
		&& CraftingWidget->GetVisibility() != ESlateVisibility::Collapsed
		&& CraftingWidget->GetVisibility() != ESlateVisibility::Hidden;
}
