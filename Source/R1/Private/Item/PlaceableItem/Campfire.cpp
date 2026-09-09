#include "Item/PlaceableItem/Campfire.h"

#include "Data/Item/PlaceableItemData.h"
#include "Component/CampfireComponent.h"
#include "Character/ActionPlayerController.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

ACampfire::ACampfire()
{
	bReplicates = true;

	// 모닥불 소리 날 곳
	FireAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudio"));
	FireAudio->SetupAttachment(GetRootComponent());
	FireAudio->bAutoActivate = false;

	// 모닥불 기능
	CampfireComponent = CreateDefaultSubobject<UCampfireComponent>(TEXT("CampfireComponent"));
}

void ACampfire::BeginPlay()
{
	Super::BeginPlay();

	// 리슨 서버 연결이라 불필요한 검사이긴 한데, 테스트나 서버 빌드 등에서 안전성 확보 위해 전용 서버인지 검사 후 사운드 설정
	if (GetNetMode() != NM_DedicatedServer)
	{
		// 사운드 유효 거리, 방향 설정은 BP에서 지정
		FireAudio->AttenuationSettings = WorldSoundAttenuation;
		FireAudio->bOverrideAttenuation = false;
		if (BurningSound)
		{
			FireAudio->SetSound(BurningSound);
		}
	}

	// 모닥불 상태 변경 관련 델리게이트 구독
	if (CampfireComponent)
	{
		CampfireComponent->OnCampfireStateChanged.AddDynamic(this, &ACampfire::HandleCampfireStateChanged);
		HandleCampfireStateChanged();	// 현재 점화 상태를 오디오 상태에 반영
	}
}

FText ACampfire::GetInteractionDisplayName_Implementation() const {
	return PlaceableItemData ? PlaceableItemData->DisplayName : FText::GetEmpty();
}

bool ACampfire::CanInteract_Implementation(APawn* Interactor) const
{
	// 거리확인
	return IsValid(Interactor) &&
		FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionDistance);
}

// 서버가 상호작용 가능 여부 확인 후, 해당 플레이어에게 모닥불 UI 열기 RPC 전송
void ACampfire::Interact_Implementation(APawn* Interactor)
{
	// 서버의 호출인지, 상호작용 가능한 거리인지 확인
	if (!HasAuthority() || !CanInteract_Implementation(Interactor))
		return;

	// 상호작용 플레이어의 클라이언트에서 모닥불 UI를 열도록 Client RPC 호출
	if (AActionPlayerController* PC = Cast<AActionPlayerController>(Interactor->GetController()))
	{
		PC->Client_OpenCampfire(this);
	}
}

// 점화 상태 전환에 맞춰 연소음 재생/정지하고, 소화음 재생
void ACampfire::HandleCampfireStateChanged()
{
	// 테스트, 서버 빌드용 안전장치
	if (GetNetMode() == NM_DedicatedServer)
		return;

	// 점화 상태 확인
	const bool bLit = CampfireComponent && CampfireComponent->bIsLit;

	// 진행도 갱신때 소리를 다시 시작하는 것 방지
	if (bWasLitForAudio == bLit)
		return;

	// 소리 재생/정지
	if (FireAudio)
	{
		if (bLit)
			FireAudio->Play();
		else
			FireAudio->Stop();
	}

	// 소화음 재생
	if (bWasLitForAudio && !bLit && ExtinguishSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExtinguishSound, GetActorLocation(), 1.f, 1.f, 0.f, WorldSoundAttenuation);
	}

	bWasLitForAudio = bLit;			// 점화 오디오 상태 업데이트: 이번에 처리한 점화 상태를 다음 상태와 비교하기 위해 저장
}

// 설치 성공 알림을 한 번만 멀티캐스트하여 관련 클라이언트에 설치음 재생
void ACampfire::NotifyPlacementSucceeded()
{
	if (!HasAuthority() || bPlacementSoundSent) return;
	bPlacementSoundSent = true;

	MulticastPlayPlacementSound();	// 설치음 재생용 함수
}

// 설치음 재생용 NetMulticast RPC
void ACampfire::MulticastPlayPlacementSound_Implementation()
{
	// 테스트 및 서버 빌드 안전장치 && 설치음 에셋 유효성 검사
	if (GetNetMode() != NM_DedicatedServer && PlacementSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PlacementSound, GetActorLocation(), 1.f, 1.f, 0.f, WorldSoundAttenuation);
	}
}
