#include "Item/PlaceableItem/Campfire/Campfire.h"

#include "Item/PlaceableItem/Campfire/CampfireComponent.h"
#include "Character/ActionPlayerController.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/Pawn.h"

ACampfire::ACampfire()
{
	bReplicates = true;

	FireAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudio"));
	FireAudio->SetupAttachment(GetRootComponent());
	FireAudio->bAutoActivate = false;

	CampfireComponent = CreateDefaultSubobject<UCampfireComponent>(TEXT("CampfireComponent"));
}

void ACampfire::BeginPlay()
{
	Super::BeginPlay();
	if (CampfireComponent)
	{
		CampfireComponent->OnCampfireStateChanged.AddDynamic(this, &ACampfire::HandleCampfireStateChanged);
		HandleCampfireStateChanged();
	}
}

FText ACampfire::GetInteractionDisplayName_Implementation() const { return InteractionName; }

bool ACampfire::CanInteract_Implementation(APawn* Interactor) const
{
	return IsValid(Interactor) && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation())
		<= FMath::Square(InteractionDistance);
}

void ACampfire::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || !CanInteract_Implementation(Interactor)) return;
	if (AActionPlayerController* PC = Cast<AActionPlayerController>(Interactor->GetController()))
	{
		PC->Client_OpenCampfire(this);
	}
}

TSoftObjectPtr<UTexture2D> ACampfire::GetInteractionIcon_Implementation() const
{
	return InteractionIcon;
}

void ACampfire::HandleCampfireStateChanged()
{
	const bool bLit = CampfireComponent && CampfireComponent->bIsLit;
	if (FireAudio) FireAudio->SetActive(bLit);
}
