#pragma once

#include "CoreMinimal.h"
#include "Item/PlaceableItemBase.h"
#include "Interface/InteractableInterface.h"
#include "Campfire.generated.h"


class UCampfireComponent;
class UParticleSystemComponent;
class UAudioComponent;
class UTexture2D;

UCLASS()
class R1_API ACampfire : public APlaceableItemBase, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ACampfire();

	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual TSoftObjectPtr<UTexture2D> GetInteractionIcon_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "Campfire")
	UCampfireComponent* GetCampfireComponent() const { return CampfireComponent; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleCampfireStateChanged();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|FX")
	TObjectPtr<UAudioComponent> FireAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UCampfireComponent> CampfireComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire")
	FText InteractionName = NSLOCTEXT("Campfire", "InteractionName", "모닥불");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "0"))
	float InteractionDistance = 350.f;

	// 조준 시 표시할 아이콘 — AWarehouseBase::InteractionIcon과 동일한 패턴(기능별 전용 아이콘).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire")
	TSoftObjectPtr<UTexture2D> InteractionIcon;
};
