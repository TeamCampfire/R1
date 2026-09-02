// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ActionPlayerController.h"
#include "Character/ActionCharacter.h"
#include "Component/StatComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"

void AActionPlayerController::BeginPlay()
{
	Super::BeginPlay();

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
	}

}

void AActionPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AActionCharacter* NewCharacter = Cast<AActionCharacter>(InPawn);

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

void AActionPlayerController::SetInventoryInputState(bool bOpen)
{
	bShowMouseCursor = bOpen;

	if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			if (bOpen)
			{
				// 인벤토리가 열려있는 동안엔 이동/시점/공격 등 게임플레이 입력을 완전히 끊는다.
				SubSystem->RemoveMappingContext(DefaultMappingContext);
			}
			else
			{
				SubSystem->AddMappingContext(DefaultMappingContext, GameInputPriority);
			}
		}
	}

	if (bOpen)
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
