/// 최초작성 : 2026.09.03
#include "Character/AnimalCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Component/HarvestableComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Character/AnimalAIController.h"
#include "UObject/ConstructorHelpers.h"

AAnimalCharacter::AAnimalCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	// AI 컨트롤러 지정 및 자동 빙의 설정
	AIControllerClass = AAnimalAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 캡슐 콜리전 설정
	GetCapsuleComponent()->InitCapsuleSize(45.0f, 75.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	// 캐릭터 무브먼트 설정 (동물 4족 보행 회전 부드럽게)
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 시체 채집 컴포넌트 기본 부착
	HarvestableComponent = CreateDefaultSubobject<UHarvestableComponent>(TEXT("HarvestableComponent"));
	HarvestableComponent->SetIsReplicatedByDefault(true);

	// 사슴 기본 스켈레탈 메시 설정
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DeerMeshFinder(TEXT("/Game/AnimalVarietyPack/DeerStagAndDoe/Meshes/SK_DeerStag.SK_DeerStag"));
	if (DeerMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(DeerMeshFinder.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -75.0f), FRotator(0.0f, -90.0f, 0.0f));
	}
}

void AAnimalCharacter::BeginPlay()
{
	Super::BeginPlay();
	CurrentHp = MaxHp;
}

void AAnimalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAnimalCharacter, CurrentHp);
	DOREPLIFETIME(AAnimalCharacter, bIsDead);
}

FHarvestRes AAnimalCharacter::OnHitted_Implementation(AActionCharacter* InCharacter, const FVector& HitLocation)
{
	FHarvestRes Res;
	if (!HasAuthority()) return Res;

	// 1. 이미 죽어있는 상태면 시체 채집 컴포넌트로 전달 (가죽/천/고기 수확)
	if (bIsDead)
	{
		if (HarvestableComponent)
		{
			return IHarvestable::Execute_OnHitted(HarvestableComponent, InCharacter, HitLocation);
		}
		return Res;
	}

	// 2. 살아있는 상태에서의 피격 (사냥 대미지 처리)
	CurrentHp -= 50.0f; // 기본 타격 대미지 (추후 무기 공격력 연동)

	if (CurrentHp <= 0.0f)
	{
		CurrentHp = 0.0f;
		Die();
	}

	Res.HarvesResult = true;
	return Res;
}

void AAnimalCharacter::Die()
{
	if (!HasAuthority() || bIsDead) return;

	bIsDead = true;

	// AI 두뇌 정지
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (AIC->BrainComponent)
		{
			AIC->BrainComponent->StopLogic("Animal Died");
		}
		AIC->UnPossess();
	}

	// 모든 클라이언트에 랙돌 및 사망 상태 브로드캐스트
	Multicast_Die();
}

void AAnimalCharacter::Multicast_Die_Implementation()
{
	bIsDead = true;

	// 캡슐 충돌 끄기 (플레이어가 시체에 부딪혀 막히지 않게)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 스켈레탈 메시 랙돌 물리 켜기
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetSimulatePhysics(true);
	}
}

void AAnimalCharacter::OnHarvestEnd_Implementation()
{
	if (HasAuthority())
	{
		Destroy();
	}
}

void AAnimalCharacter::SpawnImpactDecal_Implementation(const FVector SpawnPoint, const FRotator SpawnRotator)
{
	if (HarvestableComponent)
	{
		IHarvestable::Execute_SpawnImpactDecal(HarvestableComponent, SpawnPoint, SpawnRotator);
	}
}
