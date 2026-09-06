

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VehicleInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UVehicleInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class R1_API IVehicleInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Vehicle")
	  void EnterVehicle(APawn* VehicleCharacter, int32 SeatIndex);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Vehicle")
	  void ExitVehicle(APawn* VehicleCharacter);

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Vehicle")
	void RequestMountVehicle(ACharacter* VehicleCharacter);
};
