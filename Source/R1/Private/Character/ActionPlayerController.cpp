// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ActionPlayerController.h"
#include "Component/CraftingComponent.h"
#include "Item/PlaceableItem/Workbench.h"
#include "Character/ActionCharacter.h"
#include "Component/StatComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Framework/MainHUD.h"
#include "Framework/GameMode/TestGameMode.h"
#include "Widget/Multiplayer/MultiplayerMenuWidget.h"
#include "Vehicle/WheeledVehicleBase.h"
#include "Widget/MainHUDWidget.h"

#include "BuildingSystem/Component/BuildingPlacementComponent.h"
#include "Data/Building/BuildingPartDefinition.h"
#include "Data/Item/PlaceableItemData.h"
#include "Component/InventoryComponent.h"
#include "Item/PlaceableItem/Campfire.h"
#include "Component/CampfireComponent.h"
#include "Component/InventoryComponent.h"
#include "Widget/MainHUDWidget.h"
#include "Interface/InteractableInterface.h"

AActionPlayerController::AActionPlayerController()
{
	// 제작 컴포넌트 생성
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));

	// 빌딩 배치 컴포넌트 생성
	BuildingPlacementComponent = CreateDefaultSubobject<UBuildingPlacementComponent>(TEXT("BuildingPlacementComp"));
}

UInventoryComponent* AActionPlayerController::GetPlayerInventory() const
{
	return GetPawn() ? GetPawn()->FindComponentByClass<UInventoryComponent>() : nullptr;
}

// 모닥불 이동 및 점화 RPC에서 현재 폰과 상호작용 가능 거리를 공통으로 검사
bool AActionPlayerController::CanUseCampfire(ACampfire* Campfire) const
{
	return GetPawn() && IsValid(Campfire)
		&& IInteractableInterface::Execute_CanInteract(Campfire, GetPawn());
}

void AActionPlayerController::Client_OpenCampfire_Implementation(ACampfire* Campfire)
{
	if (AMainHUD* MainHUD = GetHUD<AMainHUD>())
	{
		if (UMainHUDWidget* Widget = MainHUD->GetMainHudWidget()) Widget->OpenCampfire(Campfire);
	}
}

// 공유 모닥불의 서버 재고 변경을 플레이어 컨트롤러에 중계 요청
void AActionPlayerController::Server_MoveInventoryToCampfire_Implementation(ACampfire* Campfire,
	FInventorySlotRef From, FCampfireSlotRef To, int32 Count, bool bHalfSplit)
{
	if (CanUseCampfire(Campfire))
		Campfire->GetCampfireComponent()->MoveFromInventory(GetPlayerInventory(), From, To, Count, bHalfSplit);
}

void AActionPlayerController::Server_MoveCampfireToInventory_Implementation(ACampfire* Campfire,
	FCampfireSlotRef From, FInventorySlotRef To, int32 Count, bool bHalfSplit)
{
	if (CanUseCampfire(Campfire)) Campfire->GetCampfireComponent()->MoveToInventory(GetPlayerInventory(), From, To, Count, bHalfSplit);
}

void AActionPlayerController::Server_QuickMoveInventoryToCampfire_Implementation(ACampfire* Campfire, FInventorySlotRef From)
{
	if (CanUseCampfire(Campfire)) Campfire->GetCampfireComponent()->QuickMoveFromInventory(GetPlayerInventory(), From);
}

void AActionPlayerController::Server_QuickMoveCampfireToInventory_Implementation(ACampfire* Campfire, FCampfireSlotRef From)
{
	if (CanUseCampfire(Campfire)) Campfire->GetCampfireComponent()->QuickMoveToInventory(GetPlayerInventory(), From);
}

void AActionPlayerController::Server_SetCampfireLit_Implementation(ACampfire* Campfire, bool bLit)
{
	if (CanUseCampfire(Campfire)) Campfire->GetCampfireComponent()->SetLit(bLit);
}

void AActionPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 게임 입력 모드로 설정
	if (IsLocalPlayerController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		FlushPressedKeys();
	}

	UEnhancedInputLocalPlayerSubsystem* SubSystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	if (SubSystem)
	{
		if (DefaultMappingContext)
		{
			SubSystem->AddMappingContext(DefaultMappingContext, GameInputPriority);
		}

		// UI 컨텍스트는 여기서 한 번 추가되면 이후 껐다 켰다 하지 않는다(SetInventoryInputState 참고).
		if (UIMappingContext)
		{
			SubSystem->AddMappingContext(UIMappingContext, UIInputPriority);
		}

		// 옵션 UI(단축키 리바인딩)가 조회/저장할 수 있도록 Enhanced Input User Settings를
		// 지연 생성/로드하고, 여기 등록된 매핑 컨텍스트들의 키 매핑을 등록해둔다.
		// (DefaultEngine.ini의 bEnableUserSettings=True가 켜져 있어야 유효한 객체가 반환된다.)
		if (UEnhancedInputUserSettings* UserSettings = SubSystem->GetUserSettings())
		{
			bool bDefaultRegistered = false;
			bool bUIRegistered = false;

			if (DefaultMappingContext)
			{
				bDefaultRegistered = UserSettings->RegisterInputMappingContext(DefaultMappingContext);
			}
			if (UIMappingContext)
			{
				bUIRegistered = UserSettings->RegisterInputMappingContext(UIMappingContext);
			}

			int32 MappingCount = 0;
			if (const UEnhancedPlayerMappableKeyProfile* Profile = UserSettings->GetActiveKeyProfile())
			{
				for (const TPair<FName, FKeyMappingRow>& RowPair : Profile->GetPlayerMappingRows())
				{
					MappingCount += RowPair.Value.Mappings.Num();
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("[KeyRebind] IMC 등록 — Default=%s(%s) UI=%s(%s) 등록후 매핑 %d개"),
				*GetNameSafe(DefaultMappingContext), bDefaultRegistered ? TEXT("성공") : TEXT("실패"),
				*GetNameSafe(UIMappingContext), bUIRegistered ? TEXT("성공") : TEXT("실패"),
				MappingCount);
		}
	}
}

void AActionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Esc: 게임 메뉴 토글
		EIC->BindAction(IA_GameMenuToggle, ETriggerEvent::Started, this, &AActionPlayerController::OnGameMenuTogglePressed);

		// 인벤토리 토글 — 컨트롤러에 바인딩해서 어떤 폰을 조종 중이든(캐릭터든 나중의 탈것이든)
		// 항상 눌리게 한다(IA_InventoryToggle 선언부 주석 참고).
		EIC->BindAction(IA_InventoryToggle, ETriggerEvent::Started, this, &AActionPlayerController::OnInventoryTogglePressed);

		// Q: 제작 UI 토글
		EIC->BindAction(IA_CraftingToggle, ETriggerEvent::Started, this, &AActionPlayerController::OnCraftingTogglePressed);
	}
}

void AActionPlayerController::OnConfirmBuildingPlacement()
{
	if (true == IsValid(BuildingPlacementComponent))
		BuildingPlacementComponent->ConfirmPlacement();
}

void AActionPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AActionCharacter* NewCharacter = Cast<AActionCharacter>(InPawn);

	// Vehicle 등의 Pawn이면 Character 변경 이벤트를 발생시키지 않는다.
	if (!NewCharacter) return;

	UE_LOG(LogTemp, Warning,
		TEXT("=== POSSESS === World=%s NetMode=%d PC=%p PCName=%s Pawn=%p PawnName=%s IsLocal=%s StatComp=%p"),
		*GetNameSafe(GetWorld()),
		GetWorld() ? static_cast<int32>(GetWorld()->GetNetMode()) : -1,
		this,
		*GetNameSafe(this),
		InPawn,
		*GetNameSafe(InPawn),
		IsLocalController() ? TEXT("TRUE") : TEXT("FALSE"),
		NewCharacter ? NewCharacter->GetStatComponent() : nullptr
	);

	OnPossessedCharChange.Broadcast();
}

void AActionPlayerController::PossessChar(AActionCharacter* InNewChar)
{
	if (!InNewChar) return;

	Possess(InNewChar);
	//OnPossessedCharChange.Broadcast();
}

void AActionPlayerController::OnRotateBuildingPart()
{
	if (true == IsValid(BuildingPlacementComponent))
		BuildingPlacementComponent->RotateBuildingPart();
}

void AActionPlayerController::OnStartPlacement(UBuildingPartDefinition* Definition)
{
	if(true == IsValid(BuildingPlacementComponent))
		BuildingPlacementComponent->StartPlacement(Definition);
}

void AActionPlayerController::OnStopPlacement()
{
	if (true == IsValid(BuildingPlacementComponent))
		BuildingPlacementComponent->StopPlacement();
}

void AActionPlayerController::OnStartPlaceablePlacement(UPlaceableItemData* ItemData, const FInventorySlotRef& SourceSlot, const FGuid& SourceInstanceID)
{
	if (true == IsValid(BuildingPlacementComponent))
		BuildingPlacementComponent->StartPlaceablePlacement(ItemData, SourceSlot, SourceInstanceID);
}

bool AActionPlayerController::TryCancelPlacement()
{
	if (false == IsValid(BuildingPlacementComponent) || false == BuildingPlacementComponent->IsPlacing())
		return false;

	BuildingPlacementComponent->StopPlacement();
	return true;
}

bool AActionPlayerController::TryConfirmPlacement()
{
	if (false == IsValid(BuildingPlacementComponent) || false == BuildingPlacementComponent->IsPlacing())
		return false;

	BuildingPlacementComponent->ConfirmPlacement();
	return true;
}

void AActionPlayerController::SetInventoryInputState(bool bOpen)
{
	ApplyUIInputState(bOpen);
}

void AActionPlayerController::SetOptionsInputState(bool bOpen)
{
	ApplyUIInputState(bOpen);
}

void AActionPlayerController::SetGameMenuInputState(bool bOpen)
{
	ApplyUIInputState(bOpen);
}

void AActionPlayerController::SetWarehouseInputState(bool bOpen)
{
	ApplyUIInputState(bOpen);
}

void AActionPlayerController::Client_OpenWarehouse_Implementation(UWarehouseInventoryComponent* Warehouse)
{
	if (!Warehouse)
	{
		return;
	}

	AMainHUD* HUD = GetHUD<AMainHUD>();
	UMainHUDWidget* MainHudWidget = HUD ? HUD->GetMainHudWidget() : nullptr;
	if (!MainHudWidget)
	{
		return;
	}

	// 같은 창고를 조준한 채 상호작용 키를 다시 누르면(닫기 버튼 없이) UI를 닫는다 — Interact
	// 자체는 서버 권위라 서버는 클라이언트의 UI 상태를 모르므로, 열지/닫을지는 여기 클라이언트
	// 쪽에서만 판단한다.
	if (MainHudWidget->GetOpenWarehouse() == Warehouse)
	{
		MainHudWidget->CloseWarehousePanel();
		return;
	}

	MainHudWidget->OpenWarehousePanel(Warehouse);
	SetWarehouseInputState(true);
}

void AActionPlayerController::ApplyUIInputState(bool bOpen)
{
	FlushPressedKeys();	// UI 토글 순간 눌려있던 키가 계속 적용되는 것 방지

	// 열려있는 UI 패널이 하나도 없다가 하나 생길 때(0→1)만 게임 입력을 끄고,
	// 마지막 하나가 닫힐 때(1→0)만 게임 입력을 복구한다 — 인벤토리를 연 채로 옵션을
	// 열었다가 옵션만 닫아도 인벤토리가 열려있는 한 게임 입력이 되살아나지 않는다.
	OpenUIPanelCount = FMath::Max(0, OpenUIPanelCount + (bOpen ? 1 : -1));
	const bool bAnyPanelOpen = OpenUIPanelCount > 0;

	SetShowMouseCursor(bAnyPanelOpen);

	// DefaultMappingContext는 UI가 열려도 더 이상 통째로 빼지 않는다 — WASD 이동
	// (AActionCharacter::OnMoveAction)은 UI가 열려있는 동안에도 계속 받아야 하기 때문이다.
	// 시야 회전/점프/스프린트/크라우치/공격/보조 액션/건축/벨트단축키는 대신 각 핸들러가
	// IsAnyUIPanelOpen()을 직접 확인해서 걸러낸다(ActionCharacter::IsUIBlockingGameplayInput 참고).

	if (bAnyPanelOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void AActionPlayerController::SetRespawnPoint(AActor* InRespawnPoint)
{
	if (!InRespawnPoint) return;

	if (!InRespawnPoint->GetClass()->ImplementsInterface(URespawnPointInterface::StaticClass())) return;

	UE_LOG(LogTemp, Warning, TEXT("%s 의 리스폰 지점이 %s 로 지정되었습니다."), * GetName(), *InRespawnPoint->GetName());
	RespawnPoint = InRespawnPoint;
}


void AActionPlayerController::RequestSpawn_Implementation()
{
	if (!HasAuthority()) return;

	ATestGameMode* GM = GetWorld()->GetAuthGameMode<ATestGameMode>();

	if (!GM) return;

	GM->RespawnPlayer(this);
}

AActor* AActionPlayerController::GetRespawnPoint()
{
	return RespawnPoint;
}

void AActionPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	UE_LOG(LogTemp, Warning,
		TEXT("=== ON REP PAWN === PC=%s Pawn=%s IsLocal=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetPawn()),
		IsLocalController() ? TEXT("TRUE") : TEXT("FALSE"));

	// 차량으로 Possess된 경우
    if (AWheeledVehicleBase* Vehicle =
        Cast<AWheeledVehicleBase>(GetPawn()))
    {
        Vehicle->AddVehicleInputMapping();
        return;
    }

    // 기존 Character 처리
    AActionCharacter* NewCharacter =
        Cast<AActionCharacter>(GetPawn());

    if (!NewCharacter)
        return;

    OnPossessedCharChange.Broadcast();
}





// Debug-----------------------------------------------------------------------------------------------------------------------

void AActionPlayerController::TestDamage(int32 PlayerIndex)
{
	UE_LOG(LogTemp, Warning,
		TEXT("=== TEST DAMAGE CMD === TargetPlayerIndex=%d"),
		PlayerIndex);

	for (TActorIterator<AActionCharacter> It(GetWorld()); It; ++It)
	{
		AActionCharacter* TargetCharacter = *It;

		if (!TargetCharacter) continue;

		APlayerState* TargetPlayerState = TargetCharacter->GetPlayerState();

		if (!TargetPlayerState) continue;

		UE_LOG(LogTemp, Warning,
			TEXT("Character=%s PlayerId=%d Target=%d"),
			*GetNameSafe(TargetCharacter),
			TargetPlayerState->GetPlayerId(),
			PlayerIndex);

		if (TargetPlayerState->GetPlayerId() != PlayerIndex) continue;

		UStatComponent* StatComp = TargetCharacter->GetStatComponent();

		if (!StatComp) return;
		UE_LOG(LogTemp, Warning,
			TEXT("=== TARGET STAT === Character=%s Ptr=%p StatComp=%s Ptr=%p Owner=%s OwnerPtr=%p"),
			*GetNameSafe(TargetCharacter),
			TargetCharacter,
			*GetNameSafe(StatComp),
			StatComp,
			*GetNameSafe(StatComp->GetOwner()),
			StatComp->GetOwner());

		UE_LOG(LogTemp, Warning,
			TEXT("=== TARGET FOUND === %s"),
			*GetNameSafe(TargetCharacter));
		UE_LOG(LogTemp, Warning,
			TEXT("TargetCharacter=%s Ptr=%p"),
			*GetNameSafe(TargetCharacter),
			TargetCharacter);
		StatComp->TestInflictDamage();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("=== TARGET NOT FOUND ==="));
}

void AActionPlayerController::TestHydrationDamage(int32 PlayerIndex)
{
	if (!HasAuthority()) return;

	for (TActorIterator<AActionCharacter> It(GetWorld()); It; ++It)
	{
		AActionCharacter* TargetCharacter = *It;

		if (!TargetCharacter)
			continue;

		APlayerState* TargetPlayerState = TargetCharacter->GetPlayerState();
		if (!TargetPlayerState) continue;

		if (PlayerState->GetPlayerId() != PlayerIndex) continue;

		UStatComponent* StatComp = TargetCharacter->GetStatComponent();

		if (!StatComp) return;

		StatComp->TestDecreaseHydration();
		return;
	}
}

void AActionPlayerController::OnGameMenuTogglePressed()
{
	AMainHUD* HUD = GetHUD<AMainHUD>();
	UMultiplayerMenuWidget* GameMenuWidget = HUD ? HUD->GetGameMenuWidget() : nullptr;
	if (!GameMenuWidget)
		return;

	const bool bOpen = GameMenuWidget->GetVisibility() == ESlateVisibility::Collapsed;

	if (bOpen)
	{
		GameMenuWidget->RefreshMenuState();
		GameMenuWidget->RefreshSessions();
	}

	GameMenuWidget->SetVisibility(
		bOpen
		? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed
	);

	SetGameMenuInputState(bOpen);	// Temp
	// ApplyUIInpuState(bOpen);
}

void AActionPlayerController::OnInventoryTogglePressed()
{
	CloseCrafting();
	// 캐릭터가 죽은 동안(사망 직후 UnPossess ~ 부활 전, 또는 살아있어도 bAlive=false인 짧은
	// 순간)엔 인벤토리 토글을 무시한다 — 죽은 화면에서 인벤토리 패널을 열어봐야 HUDPanel 자체가
	// Collapsed라 보이지도 않으면서 OpenUIPanelCount/커서 상태만 어긋나게 된다.
	AActionCharacter* PossessedCharacter = Cast<AActionCharacter>(GetPawn());
	const IHealthInterface* HealthInterface = PossessedCharacter ? Cast<IHealthInterface>(PossessedCharacter->GetStatComponent()) : nullptr;
	if (!PossessedCharacter || !HealthInterface || !HealthInterface->IsAlive())
	{
		return;
	}

	AMainHUD* HUD = GetHUD<AMainHUD>();
	UMainHUDWidget* MainHudWidget = HUD ? HUD->GetMainHudWidget() : nullptr;
	if (!MainHudWidget)
	{
		return;
	}

	const bool bIsOpen = MainHudWidget->ToggleInventoryPanel();
	SetInventoryInputState(bIsOpen);
}

void AActionPlayerController::ServerTestInflictDamage_Implementation()
{
	AActionCharacter* ActionChar = Cast<AActionCharacter>(GetPawn());

	if (!ActionChar)
		return;

	UStatComponent* StatComp = ActionChar->GetStatComponent();

	if (!StatComp)
		return;

	StatComp->Execute_InflictDamage(StatComp, 50.0f);
}
//------------------------------------------------------------------------------------------------------------------------------------------

// Q 키 입력
// 개인 제작 화면을 토글하며, 작업대 없는 제작은 nullptr로 구분
void AActionPlayerController::OnCraftingTogglePressed()
{
	AMainHUD* HUD = GetHUD<AMainHUD>();
	UMainHUDWidget* MainWidget = HUD ? HUD->GetMainHudWidget() : nullptr;
	if (MainWidget && MainWidget->IsCraftingPanelOpen())
	{
		CloseCrafting();
		return;
	}

	// Q 입력으로 UI 열기는 로컬에서만 작동
	// 실제 제작 요청은 제작 컴포넌트의 서버 RPC를 사용
	Client_OpenCrafting_Implementation(nullptr);
}

// 닫기 요청: HUD에 창 정리 위임
void AActionPlayerController::CloseCrafting()
{
	AMainHUD* HUD = GetHUD<AMainHUD>();
	if (UMainHUDWidget* MainWidget = HUD ? HUD->GetMainHudWidget() : nullptr)
		MainWidget->CloseCraftingPanel();
}

// 제작 가능 상태 확인, 개인 제작 컴포넌트와 선택 작업대를 HUD에 전달
void AActionPlayerController::Client_OpenCrafting_Implementation(AWorkbench* Bench)
{
	AActionCharacter* PossessedCharacter = Cast<AActionCharacter>(GetPawn());

	// 죽은 상태에서는 제작창 열기 방지
	const IHealthInterface* Health =
		PossessedCharacter
		? Cast<IHealthInterface>(PossessedCharacter->GetStatComponent())
		: nullptr;
	if (!Health || !Health->IsAlive())
	{
		return;
	}

	// 소유 클라이언트의 로컬 컨트롤러에서만 제작 UI 열기
	if (!IsLocalController())
		return;

	// 제작 데이터와 작업대를 전달하고 창 관리는 HUD에 위임
	AMainHUD* HUD = GetHUD<AMainHUD>();
	if (UMainHUDWidget* MainWidget = HUD ? HUD->GetMainHudWidget() : nullptr)
	{
		MainWidget->OpenCraftingPanel(CraftingComponent, Bench);
	}
}
