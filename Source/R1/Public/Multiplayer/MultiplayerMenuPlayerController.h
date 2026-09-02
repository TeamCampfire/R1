

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MultiplayerMenuPlayerController.generated.h"

class UMultiplayerMenuWidget;
/**
 * 
 */
UCLASS()
class R1_API AMultiplayerMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected :

	virtual void BeginPlay() override;

private :

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UMultiplayerMenuWidget> MultiplayerMenuWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerMenuWidget> MultiplayerMenuWidget;

};
