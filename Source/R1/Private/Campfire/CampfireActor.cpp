#include "Campfire/CampfireActor.h"

#include "Campfire/CampfireComponent.h"
#include "Character/ActionPlayerController.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"

ACampfireActor::ACampfireActor()
{
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireMesh"));
	Mesh->SetupAttachment(GetRootComponent());

	FireAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudio"));
	FireAudio->SetupAttachment(GetRootComponent());
	FireAudio->bAutoActivate = false;

	CampfireComponent = CreateDefaultSubobject<UCampfireComponent>(TEXT("CampfireComponent"));
}

void ACampfireActor::BeginPlay()
{
	Super::BeginPlay();
	if (CampfireComponent)
	{
		CampfireComponent->OnCampfireStateChanged.AddDynamic(this, &ACampfireActor::HandleCampfireStateChanged);
		HandleCampfireStateChanged();
	}
}

FText ACampfireActor::GetInteractionDisplayName_Implementation() const { return InteractionName; }

bool ACampfireActor::CanInteract_Implementation(APawn* Interactor) const
{
	return IsValid(Interactor) && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation())
		<= FMath::Square(InteractionDistance);
}

void ACampfireActor::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || !CanInteract_Implementation(Interactor)) return;
	if (AActionPlayerController* PC = Cast<AActionPlayerController>(Interactor->GetController()))
	{
		PC->Client_OpenCampfire(this);
	}
}

void ACampfireActor::HandleCampfireStateChanged()
{
	const bool bLit = CampfireComponent && CampfireComponent->bIsLit;
	if (FireAudio) FireAudio->SetActive(bLit);
}
