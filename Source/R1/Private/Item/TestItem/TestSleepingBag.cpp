


#include "Item/TestItem/TestSleepingBag.h"

// Sets default values
ATestSleepingBag::ATestSleepingBag()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

FTransform ATestSleepingBag::GetRespawnTransform_Implementation() const
{
	return GetActorTransform();
}

// Called when the game starts or when spawned
void ATestSleepingBag::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATestSleepingBag::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

