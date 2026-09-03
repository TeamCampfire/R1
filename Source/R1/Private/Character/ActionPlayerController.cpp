// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ActionPlayerController.h"
#include "Character/ActionCharacter.h"
#include "Component/StatComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Framework/MainHUD.h"
#include "Widget/Multiplayer/MultiplayerMenuWidget.h"

#include "BuildingSystem/Component/BuildingPlacementComponent.h"
#include "Data/Building/BuildingPartDefinition.h"

AActionPlayerController::AActionPlayerController()
{
	// 빌딩 배치 컴포넌트 생성
	BuildingPlacementComponent = CreateDefaultSubobject<UBuildingPlacementComponent>(TEXT("BuildingPlacementComp"));
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
	ApplyUIInputState();

	FlushPressedKeys();	// 메뉴 여는 순간 눌려있던 키가 계속 적용되는 것 방지
}

void AActionPlayerController::ApplyUIInputState(bool bOpen)
{
	// 열려있는 UI 패널이 하나도 없다가 하나 생길 때(0→1)만 게임 입력을 끄고,
	// 마지막 하나가 닫힐 때(1→0)만 게임 입력을 복구한다 — 인벤토리를 연 채로 옵션을
	// 열었다가 옵션만 닫아도 인벤토리가 열려있는 한 게임 입력이 되살아나지 않는다.
	OpenUIPanelCount = FMath::Max(0, OpenUIPanelCount + (bOpen ? 1 : -1));
	const bool bAnyPanelOpen = OpenUIPanelCount > 0;

	SetShowMouseCursor(bAnyPanelOpen);

	if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			if (bAnyPanelOpen)
			{
				// UI 패널이 하나라도 열려있는 동안엔 이동/시점/공격 등 게임플레이 입력을 완전히 끊는다.
				SubSystem->RemoveMappingContext(DefaultMappingContext);
			}
			else
			{
				SubSystem->AddMappingContext(DefaultMappingContext, GameInputPriority);
			}
		}
	}

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

	AActionCharacter* NewCharacter = Cast<AActionCharacter>(GetPawn());

	// 차량 등 Character가 아닌 Pawn으로 Possess된 경우
	// Character 변경으로 취급하지 않는다.
	if (!NewCharacter) return;

	if (!IsLocalController()) return;

	if (!GetPawn()) return;
	
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
			TEXT("=== TARGET FOUND === %s"),
			*GetNameSafe(TargetCharacter));

		StatComp->TestInflictDamage();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("=== TARGET NOT FOUND ==="));
}

void AActionPlayerController::TestHydrationDamage(int32 PlayerIndex)
{
	//if (!HasAuthority()) return;

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
