

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Interface/Vehicle/VehicleInterface.h"
#include "Interface/Vehicle/HorseInterface.h"
#include "Horse.generated.h"

UCLASS()
class R1_API AHorse : public ACharacter, public IVehicleInterface, public IHorseInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHorse();

public:
	virtual void EnterVehicle_Implementation(APawn* VehicleCharacter, int32 SeatIndex) override;

	virtual void ExitVehicle_Implementation(APawn* VehicleCharacter) override;

	virtual AActionCharacter* GetDriverCharacter() const override;

	virtual void RequestMountVehicle_Implementation(ACharacter* VehicleCharacter) override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
