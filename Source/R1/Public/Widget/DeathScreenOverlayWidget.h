

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeathScreenOverlayWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpawnButtonClicked);

/**
 * 
 */
UCLASS()
class R1_API UDeathScreenOverlayWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnRespawnButtonClicked();
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> RespawnButton;
public:
	UPROPERTY(BlueprintAssignable)
	FOnSpawnButtonClicked OnRespawnClicked;
};
