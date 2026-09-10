#include "Item/SleepingBag.h"
#include "Character/ActionCharacter.h"
#include "Character/ActionPlayerController.h"

// Sets default values
ASleepingBag::ASleepingBag()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

FTransform ASleepingBag::GetRespawnTransform_Implementation() const
{
	return GetActorTransform();
}

void ASleepingBag::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || !Interactor) return;

	AActionCharacter* Player = Cast<AActionCharacter>(Interactor);
	AActionPlayerController* PC = Cast<AActionPlayerController>(Interactor->GetController());
	if (Player && PC)
	{
		PC->SetRespawnPoint(this);
		Player->StartSleeping(GetTransform());
	}

}

TSoftObjectPtr<UTexture2D> ASleepingBag::GetInteractionIcon_Implementation() const
{
	if (!InteractionIcon) return nullptr;
	return InteractionIcon ;
}

// Called when the game starts or when spawned
void ASleepingBag::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASleepingBag::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

