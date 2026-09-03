

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

	// 뷰포트에 추가된 위젯이 컨트롤러 수명 동안 유지되도록 참조
	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerMenuWidget> MultiplayerMenuWidget;

};
