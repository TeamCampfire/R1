/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/HeldItem/FishingBobber.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "DrawDebugHelpers.h"
#include "Item/HeldItem/FishingRod.h"

AFishingBobber::AFishingBobber()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 충돌체 설정 (Hit 및 Overlap 모두 감지)
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;
	CollisionComp->InitSphereRadius(12.0f);
	CollisionComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->OnComponentHit.AddDynamic(this, &AFishingBobber::OnBobberHit);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AFishingBobber::OnBobberOverlap);

	// 2. 외형 메시 (기본 구형 찌 메시)
	BobberMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BobberMesh"));
	BobberMesh->SetupAttachment(RootComponent);
	BobberMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		BobberMesh->SetStaticMesh(SphereMeshFinder.Object);
		BobberMesh->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.25f)); // 찌 모양 타원구
	}

	// 3. 투사체 이동 컴포넌트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
	ProjectileMovement->bAutoActivate = false;

	// 4. 멀티플레이어 리플리케이션
	bReplicates = true;
	SetReplicateMovement(true);
}

void AFishingBobber::BeginPlay()
{
	Super::BeginPlay();
}

void AFishingBobber::LaunchBobber(const FVector& LaunchVelocity)
{
	bIsInWater = false;
	bIsBiting = false;
	RunningTime = 0.0f;
	CurrentSubmergeOffset = 0.0f;

	if (ProjectileMovement)
	{
		ProjectileMovement->ProjectileGravityScale = 1.0f;
		ProjectileMovement->Velocity = LaunchVelocity;
		ProjectileMovement->Activate(true);
	}
}

void AFishingBobber::OnBobberHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsInWater) return;

	OnEnterWater(Cast<AWaterBody>(OtherActor), Hit.ImpactPoint);
}

void AFishingBobber::OnBobberOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsInWater) return;

	const FVector ContactLoc = bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation();
	OnEnterWater(Cast<AWaterBody>(OtherActor), ContactLoc);
}

void AFishingBobber::OnEnterWater(AWaterBody* WaterBody, const FVector& SurfaceLocation)
{
	if (bIsInWater) return;
	bIsInWater = true;
	CachedWaterBody = WaterBody;
	BaseWaterZ = SurfaceLocation.Z;

	if (CachedWaterBody && CachedWaterBody->GetWaterBodyComponent())
	{
		BaseWaterZ = CachedWaterBody->GetWaterBodyComponent()->GetConstantSurfaceZ();
	}

	// 1. 비행 정지, 중력 제거 및 물리 끄기 (절대 가라앉지 않음)
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Velocity = FVector::ZeroVector;
		ProjectileMovement->ProjectileGravityScale = 0.0f;
		ProjectileMovement->Deactivate();
	}

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. 착수 이펙트 및 사운드
	if (WaterSplashFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WaterSplashFX, SurfaceLocation);
	}
	// 수면 착수 파문 시각화
	DrawDebugCircle(GetWorld(), SurfaceLocation + FVector(0.f, 0.f, 2.f), 25.0f, 24, FColor::Cyan, false, 1.2f, 0, 3.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	DrawDebugCircle(GetWorld(), SurfaceLocation + FVector(0.f, 0.f, 2.f), 45.0f, 32, FColor::Emerald, false, 1.5f, 0, 2.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);

	if (WaterSplashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), WaterSplashSound, SurfaceLocation);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("[낚시] 찌가 수면에 안착했습니다. 물고기를 기다립니다..."));
	}
	UE_LOG(LogTemp, Display, TEXT("[낚시] 찌가 수면(Z=%.1f)에 안착했습니다."), BaseWaterZ);

	// 3. 낚싯대에 착수 완료 통보
	if (OwnerRod)
	{
		OwnerRod->OnBobberLandedInWater();
	}
}

void AFishingBobber::SetBiting(bool bBiting)
{
	bIsBiting = bBiting;

	// 입질 시작 시 첨벙 이펙트/사운드 및 수면 첨벙 파문
	if (bIsBiting)
	{
		const FVector BiteLoc = GetActorLocation();
		if (WaterSplashFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WaterSplashFX, BiteLoc);
		}
		// 첨벙 순간 퍼져나가는 푸른 수면 파문 링
		DrawDebugCircle(GetWorld(), BiteLoc + FVector(0.f, 0.f, 2.f), 35.0f, 24, FColor(0, 220, 255), false, 1.0f, 0, 4.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		DrawDebugCircle(GetWorld(), BiteLoc + FVector(0.f, 0.f, 2.f), 60.0f, 32, FColor(50, 150, 255), false, 1.2f, 0, 2.5f, FVector(1, 0, 0), FVector(0, 1, 0), false);

		if (WaterSplashSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), WaterSplashSound, BiteLoc);
		}
	}
}

void AFishingBobber::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 수면에 떠 있는 동안 부유(Bobbing) 및 입질 꿀렁임 물리
	if (bIsInWater)
	{
		RunningTime += DeltaTime;

		// 1. 실시간 WaterBody 표면 높이 추적
		if (CachedWaterBody && CachedWaterBody->GetWaterBodyComponent())
		{
			BaseWaterZ = CachedWaterBody->GetWaterBodyComponent()->GetConstantSurfaceZ();
		}

		// 2. 찰랑거리는 잔물결 오르내림 파동 연산
		const float SineWave = FMath::Sin(RunningTime * BobbingSpeed) * BobbingAmplitude;

		// 3. 입질 시 아래로 쑥 들어가는 깊이 보간
		const float TargetSubmerge = bIsBiting ? BiteSubmergeDepth : 0.0f;
		CurrentSubmergeOffset = FMath::FInterpTo(CurrentSubmergeOffset, TargetSubmerge, DeltaTime, 12.0f);

		// 4. 최종 Z 높이 적용 (XY 수평 좌표는 유지)
		FVector CurrentLoc = GetActorLocation();
		CurrentLoc.Z = BaseWaterZ + SineWave - CurrentSubmergeOffset;
		SetActorLocation(CurrentLoc);

		// 5. 찰랑거리는 찌의 자연스러운 각도 흔들림
		const float PitchTilt = FMath::Cos(RunningTime * BobbingSpeed) * 4.0f;
		const float RollTilt = FMath::Sin(RunningTime * (BobbingSpeed * 0.8f)) * 4.0f;
		SetActorRotation(FRotator(PitchTilt, 0.0f, RollTilt));
	}
}
