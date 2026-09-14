#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HarvestPoolable.generated.h"

/**
 * Optional lifecycle hooks for actors managed by AHarvestSpawner.
 *
 * Most static harvestables only need UHarvestableComponent's automatic reset.
 * Stateful actors (animals, destructible Blueprint actors, and so on) can
 * implement these events to restore their own state when reused.
 */
UINTERFACE(BlueprintType)
class R1_API UHarvestPoolable : public UInterface
{
	GENERATED_BODY()
};

class R1_API IHarvestPoolable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Harvest Pool")
	void OnTakenFromHarvestPool();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Harvest Pool")
	void OnReturnedToHarvestPool();
};
