

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interface/Vehicle/VehicleInterface.h"
#include "VehicleBase.generated.h"

class AActionCharacter;
class USceneComponent;
class UInputMappingContext;
class UCameraComponent;

UCLASS()
class R1_API AVehicleBase : public APawn, public IVehicleInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVehicleBase();


public:
	// Implement the RequestMountVehicle function from the IVehicleInterface
	virtual void RequestMountVehicle_Implementation(ACharacter* VehicleCharacter) override;

	virtual void EnterVehicle_Implementation(APawn* VehicleCharacter, int32 InSeatIndex) override;

	virtual void ExitVehicle_Implementation(APawn* VehicleCharacter) override;

protected:
	void InitializeSeatPoints();

	virtual void PossessedBy(AController* NewController) override;

protected:
	TWeakObjectPtr<ACharacter> CurrentPassenger;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	//TObjectPtr<USceneComponent> Root;

	// 탈것의 좌석들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Seats")
	TArray<TObjectPtr<USceneComponent>> SeatPoints;

	// 운전자
	UPROPERTY()
	TObjectPtr<AActionCharacter> DriverCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_Vehicle;

private:
	UPROPERTY()
	TArray<TObjectPtr<APawn>> SeatOccupants;




protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;



};
