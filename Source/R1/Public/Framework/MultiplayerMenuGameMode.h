#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MultiplayerMenuGameMode.generated.h"

class UMultiplayerMenuWidget;

/** Lv_MainMenu에서 멀티플레이 메뉴를 생성하고 UI 입력을 활성화한다. */
UCLASS()
class R1_API AMultiplayerMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMultiplayerMenuGameMode();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UMultiplayerMenuWidget> MultiplayerMenuWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerMenuWidget> MultiplayerMenuWidget;
};
