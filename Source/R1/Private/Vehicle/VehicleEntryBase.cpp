#include "Vehicle/VehicleEntryBase.h"
#include "Vehicle/VehicleBase.h"
#include "Vehicle/WheeledVehicleBase.h"
#include "Character/ActionPlayerController.h"
#include "Character/ActionCharacter.h"

// Sets default values
AVehicleEntryBase::AVehicleEntryBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}



void AVehicleEntryBase::Interact_Implementation(APawn* Interactor)
{
	if (!Interactor) return;
	//UE_LOG(LogTemp, Warning, TEXT("=== VehicleEntryBase::Interact_Implementation === Interactor=%s"), *GetNameSafe(Interactor));
	UE_LOG(LogTemp, Warning, TEXT("=== VehicleEntryBase::Interact_Implementation === Interactor=%s Vehicle=%s SeatIndex=%d"), *GetNameSafe(Interactor), *GetNameSafe(Vehicle), SeatIndex);
	if (!Vehicle) return;

	Vehicle->Execute_EnterVehicle(Vehicle, Interactor, SeatIndex);
}

// Called when the game starts or when spawned
void AVehicleEntryBase::BeginPlay()
{
	Super::BeginPlay();
	Vehicle = Cast<AWheeledVehicleBase>(GetAttachParentActor());
}

// Called every frame
void AVehicleEntryBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

