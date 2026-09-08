#include "Item/PlaceableItem/Campfire/Campfire.h"

#include "Item/PlaceableItem/Campfire/CampfireComponent.h"
#include "Character/ActionPlayerController.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

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
	if (GetNetMode() != NM_DedicatedServer)
	{
		// 거리, 방향 설정은 BP에서 지정
		FireAudio->AttenuationSettings = WorldSoundAttenuation;
		FireAudio->bOverrideAttenuation = false;
		if (BurningSound) FireAudio->SetSound(BurningSound);
	}
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

void ACampfire::HandleCampfireStateChanged()
{
	if (GetNetMode() == NM_DedicatedServer) return;
	const bool bLit = CampfireComponent && CampfireComponent->bIsLit;
	// 진행도 갱신마다 소리를 다시 시작하지 않음
	if (bAudioStateInitialized && bWasLitForAudio == bLit)
		return;

	if (FireAudio)
	{
		if (bLit)
			FireAudio->Play();
		else
			FireAudio->Stop();
	}

	if (bAudioStateInitialized && bWasLitForAudio && !bLit && ExtinguishSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExtinguishSound, GetActorLocation(), 1.f, 1.f, 0.f, WorldSoundAttenuation);
	}
	bWasLitForAudio = bLit;
	bAudioStateInitialized = true;
}

void ACampfire::NotifyPlacementSucceeded()
{
	if (!HasAuthority() || bPlacementSoundSent) return;
	bPlacementSoundSent = true;
	MulticastPlayPlacementSound();
}

void ACampfire::MulticastPlayPlacementSound_Implementation()
{
	if (GetNetMode() != NM_DedicatedServer && PlacementSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PlacementSound, GetActorLocation(),
			1.f, 1.f, 0.f, WorldSoundAttenuation);
	}
}
