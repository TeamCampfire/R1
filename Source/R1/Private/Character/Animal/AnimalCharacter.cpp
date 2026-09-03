/// 최초작성 : 2026.09.03
/// 작성자 : 주형진
#include "Character/Animal/AnimalCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Component/HarvestableComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Character/Animal/AnimalAIController.h"
#include "UObject/ConstructorHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"

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
	HarvestableComponent->SetIsReplicated(true);
	HarvestableComponent->Activate(false);

	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -75.0f), FRotator(0.0f, -90.0f, 0.0f));
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

float AAnimalCharacter::GetCurrentHealth_Implementation() const
{
	return CurrentHp;
}

float AAnimalCharacter::GetMaxHealth_Implementation() const
{
	return MaxHp;
}

void AAnimalCharacter::InflictDamage_Implementation(float InAmount)
{
	CurrentHp = FMath::Clamp(CurrentHp - InAmount, 0, MaxHp);
	if (InAmount > 0)
	{
		Multicast_PlayHitMontage();
	}
	if (CurrentHp <= 0) Die();
}

void AAnimalCharacter::Heal_Implementation(float InAmount)
{
	CurrentHp = FMath::Clamp(CurrentHp + InAmount, 0, MaxHp);
}

bool AAnimalCharacter::IsAlive() const
{
	return !bIsDead;
}

void AAnimalCharacter::Die()
{
	if (!HasAuthority() || bIsDead) return;

	bIsDead = true;
	HarvestableComponent->Activate(true);

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

void AAnimalCharacter::Multicast_PlayHitMontage_Implementation()
{
	PlayAnimMontage(AM_Hitted);
}

void AAnimalCharacter::Multicast_Die_Implementation()
{
	bIsDead = true;

	// 1. 이동 및 중력 즉시 정지 (클라이언트 로컬 낙하/땅 꺼짐 방지)
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		MoveComp->GravityScale = 0.0f;
	}

	// 2. 캡슐 충돌: 플레이어가 시체에 부딪혀 막히지 않도록 Pawn 충돌 무시
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);

	// 3. 스켈레탈 메시: 시체 채집(Visibility) 충돌 허용
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // 채집 공격(Visibility) 맞을 수 있게 Block
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);     // 카메라는 시체에 걸리지 않게 무시
		MeshComp->SetSimulatePhysics(false);
	}
	PlayAnimMontage(AM_Death);

}
