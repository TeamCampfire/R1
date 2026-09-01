#include "Framework/MultiplayerMenuGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Widget/Multiplayer/MultiplayerMenuWidget.h"

AMultiplayerMenuGameMode::AMultiplayerMenuGameMode()
{
	DefaultPawnClass = nullptr;
}

void AMultiplayerMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (IsRunningDedicatedServer() || !MultiplayerMenuWidgetClass)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	MultiplayerMenuWidget = CreateWidget<UMultiplayerMenuWidget>(PlayerController, MultiplayerMenuWidgetClass);
	if (!MultiplayerMenuWidget)
	{
		return;
	}

	MultiplayerMenuWidget->AddToViewport();
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MultiplayerMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}
