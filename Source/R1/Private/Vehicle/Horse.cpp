


#include "Vehicle/Horse.h"

// Sets default values
AHorse::AHorse()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Replicate 설정
	bReplicates = true;
	SetReplicateMovement(true);
}

void AHorse::EnterVehicle_Implementation(APawn* VehicleCharacter, int32 SeatIndex)
{
}

void AHorse::ExitVehicle_Implementation(APawn* VehicleCharacter)
{
}

AActionCharacter* AHorse::GetDriverCharacter() const
{
	return nullptr;
}

void AHorse::RequestMountVehicle_Implementation(ACharacter* VehicleCharacter)
{
}

// Called when the game starts or when spawned
void AHorse::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHorse::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AHorse::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

