#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/PlaceableItem/Campfire/CampfireTypes.h"
#include "CampfireWidget.generated.h"

class ACampfire;
class UCampfireComponent;
class UCampfireSlotWidget;
class UProgressBar;
class UButton;
class UTextBlock;

UCLASS()
class R1_API UCampfireWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void BindCampfire(ACampfire* InCampfire);

	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void UnbindCampfire();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCampfireSlotWidget> FuelSlot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCampfireSlotWidget> InputSlot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCampfireSlotWidget> FuelOutputSlot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCampfireSlotWidget> CookingOutputSlot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> CookingProgressBar;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> FuelProgressBar;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ToggleFireButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ToggleFireText;

private:

	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void HandleToggleFire();

	UFUNCTION()
	void HandleSlotRightClicked(ACampfire* Campfire, FCampfireSlotRef CampfireSlotRef);

	UFUNCTION()
	void HandleInventoryDropped(ACampfire* Campfire, FInventorySlotRef From,FCampfireSlotRef To, int32 Count, bool bHalfSplit);

	UFUNCTION()
	void HandleCampfireDropped(ACampfire* Campfire, FCampfireSlotRef From, FCampfireSlotRef To, int32 Count, bool bHalfSplit);

	void InitializeSlots();

	TWeakObjectPtr<ACampfire> BoundCampfire;
};
