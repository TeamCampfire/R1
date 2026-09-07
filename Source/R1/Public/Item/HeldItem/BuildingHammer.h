

#pragma once

#include "CoreMinimal.h"
#include "Item/HeldItemBase.h"
#include "BuildingHammer.generated.h"

/**
 * 
 */
UCLASS()
class R1_API ABuildingHammer : public AHeldItemBase
{
	GENERATED_BODY()

public:
	virtual void OnSecondaryActionStarted() override;

protected:
	UFUNCTION(Server, Reliable)
	void Server_ApplyBuildingDamage(ABuildingActor* TargetBuilding, float Damage);
};
