#include "Item/SleepingBag.h"
#include "Character/ActionCharacter.h"
#include "Character/ActionPlayerController.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASleepingBag::ASleepingBag()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ASleepingBag::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASleepingBag, OccupantCharacter);
}

FTransform ASleepingBag::GetRespawnTransform_Implementation() const
{
	return GetActorTransform();
}

void ASleepingBag::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || !Interactor) return;
	if (OccupantCharacter != nullptr) return;

	AActionCharacter* Player = Cast<AActionCharacter>(Interactor);
	AActionPlayerController* PC = Cast<AActionPlayerController>(Interactor->GetController());
	if (Player && PC)
	{
		// 지금 침낭을 소유한(잠을 자는 캐릭터)
		OccupantCharacter = Player;
		PC->SetRespawnPoint(this);
		Player->StartSleeping(this);
	}

}

TSoftObjectPtr<UTexture2D> ASleepingBag::GetInteractionIcon_Implementation() const
{
	if (!InteractionIcon) return nullptr;
	return InteractionIcon ;
}

// 침낭 소유권 해제
void ASleepingBag::ClearOccupant(AActionCharacter* Character)
{
	if (!HasAuthority()) return;
	// 자고 있던 본인이 일어난 경우에만 침낭을 비움
	if (OccupantCharacter == Character)
	{
		OccupantCharacter = nullptr;
	}
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

