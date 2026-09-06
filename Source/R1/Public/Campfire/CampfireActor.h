#pragma once

#include "CoreMinimal.h"
#include "BuildingSystem/BuildingActor.h"
#include "Interface/InteractableInterface.h"
#include "CampfireActor.generated.h"


class UCampfireComponent;
class UParticleSystemComponent;
class UAudioComponent;
class UStaticMeshComponent;

UCLASS()
class R1_API ACampfireActor : public ABuildingActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ACampfireActor();

	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "Campfire")
	UCampfireComponent* GetCampfireComponent() const { return CampfireComponent; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleCampfireStateChanged();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Mesh")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|FX")
	TObjectPtr<UAudioComponent> FireAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UCampfireComponent> CampfireComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire")
	FText InteractionName = NSLOCTEXT("Campfire", "InteractionName", "모닥불");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "0"))
	float InteractionDistance = 350.f;
};
