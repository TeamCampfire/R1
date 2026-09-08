

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Interface/Vehicle/VehicleEntryInterface.h"
#include "Interface/InteractableInterface.h"
#include "VehicleEntryBase.generated.h"

class AVehicleBase;
class AWheeledVehicleBase;

UCLASS()
class R1_API AVehicleEntryBase : public AActor, public IVehicleEntryInterface, public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVehicleEntryBase();


public:

	//virtual FText GetInteractionDisplayName_Implementation() const override { return FText::FromString(TEXT("Vehicle Entry")); }

	virtual bool CanInteract_Implementation(APawn* Interactor) const override { return true; }

	virtual void Interact_Implementation(APawn* Interactor) override;
protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<AWheeledVehicleBase> Vehicle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	int32 SeatIndex = 0;









protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
