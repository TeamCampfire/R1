// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ActionPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"

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

