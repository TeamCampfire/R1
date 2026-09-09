/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/HeldItem/FishingBobber.h"
#include "R1/R1.h"
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
	CollisionComp->InitSphereRadius(8.0f);
	CollisionComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComp->SetCollisionResponseToChannel(ECC_Water, ECR_Overlap);
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->OnComponentHit.AddDynamic(this, &AFishingBobber::OnBobberHit);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AFishingBobber::OnBobberOverlap);

	// 2. 외형 메시 (기본 구형 찌 메시)
	if (BobberMesh)
	{
		BobberMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BobberMesh"));
		BobberMesh->SetupAttachment(RootComponent);
		BobberMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BobberMesh->SetRelativeScale3D(FVector(5, 5, 5)); // 찌 모양 타원구
	}


	// 3. 투사체 이동 컴포넌트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 6000.f;
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

void AFishingBobber::LaunchBobber(const FVector& LaunchVelocity, float InExpectedWaterZ)
{
	bIsInWater = false;
	bIsBiting = false;
	RunningTime = 0.0f;
	CurrentSubmergeOffset = 0.0f;
	PrevTickLocation = GetActorLocation();

	if (InExpectedWaterZ > -900000.0f)
	{
		ExpectedWaterZ = InExpectedWaterZ;
		bHasExpectedWaterZ = true;
	}

	if (GetOwner())
	{
		CollisionComp->IgnoreActorWhenMoving(GetOwner(), true);
	}
	if (GetInstigator())
	{
		CollisionComp->IgnoreActorWhenMoving(GetInstigator(), true);
	}
	if (OwnerRod)
	{
		CollisionComp->IgnoreActorWhenMoving(OwnerRod, true);
	}

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

	// 하강 중이고 수면 높이 근처에 도달했을 때만 착수 인정 (공중 오발동 방지)
	if (ProjectileMovement && ProjectileMovement->Velocity.Z > 0.0f) return;
	if (bHasExpectedWaterZ && GetActorLocation().Z > ExpectedWaterZ + 30.0f) return;

	if (OtherActor && (OtherActor->IsA(AWaterBody::StaticClass()) || OtherActor->GetName().Contains(TEXT("Water"))))
	{
		OnEnterWater(Cast<AWaterBody>(OtherActor), Hit.ImpactPoint);
	}
}

void AFishingBobber::OnBobberOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsInWater) return;

	// 하강 중이고 수면 높이 근처에 도달했을 때만 착수 인정 (공중 수체 볼륨 조기 충돌 방지)
	if (ProjectileMovement && ProjectileMovement->Velocity.Z > 0.0f) return;
	if (bHasExpectedWaterZ && GetActorLocation().Z > ExpectedWaterZ + 30.0f) return;

	if (OtherActor && (OtherActor->IsA(AWaterBody::StaticClass()) || OtherActor->GetName().Contains(TEXT("Water"))))
	{
		const FVector ContactLoc = bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation();
		OnEnterWater(Cast<AWaterBody>(OtherActor), ContactLoc);
	}
}

void AFishingBobber::OnEnterWater(AWaterBody* WaterBody, const FVector& SurfaceLocation)
{
	if (bIsInWater) return;
	bIsInWater = true;
	CachedWaterBody = WaterBody;
	BaseWaterZ = SurfaceLocation.Z;

	if (CachedWaterBody && CachedWaterBody->GetWaterBodyComponent())
	{
		const float ConstantZ = CachedWaterBody->GetWaterBodyComponent()->GetConstantSurfaceZ();
		if (FMath::Abs(ConstantZ - SurfaceLocation.Z) < 50.0f)
		{
			BaseWaterZ = ConstantZ;
		}
	}
	else if (bHasExpectedWaterZ && FMath::Abs(ExpectedWaterZ - SurfaceLocation.Z) < 60.0f)
	{
		BaseWaterZ = ExpectedWaterZ;
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

	// 찌 위치를 정확한 착수 위치로 고정 (X, Y 위치 보존)
	FVector LandedLoc = SurfaceLocation.IsZero() ? GetActorLocation() : SurfaceLocation;
	LandedLoc.Z = BaseWaterZ;
	SetActorLocation(LandedLoc);

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

void AFishingBobber::SetFishPullFeedback(float InDirection, float InTensionPercent, bool bInCorrectResist)
{
	CurrentFishDirection = InDirection;
	CurrentTensionPercent = InTensionPercent;
	bCorrectResistFeedback = bInCorrectResist;
}

void AFishingBobber::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. 비행 중: 수면 착수 실시간 감지
	if (!bIsInWater)
	{
		const FVector CurrentLoc = GetActorLocation();

		// 하강 중일 때만 착수 감지 (상승 중 오감지 차단)
		if (ProjectileMovement && ProjectileMovement->Velocity.Z <= 0.0f)
		{
			// (1) 이전 틱 위치 ~ 현재 위치 사이의 ECC_Water 라인 트레이스 (초고속 관통 터널링 방지)
			FHitResult WaterHit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			if (OwnerRod) QueryParams.AddIgnoredActor(OwnerRod);
			if (GetOwner()) QueryParams.AddIgnoredActor(GetOwner());
			if (GetInstigator()) QueryParams.AddIgnoredActor(GetInstigator());

			const FVector TraceStart = PrevTickLocation.IsZero() ? CurrentLoc : PrevTickLocation;
			const bool bHitWater = GetWorld()->LineTraceSingleByChannel(
				WaterHit,
				TraceStart,
				CurrentLoc,
				ECC_Water,
				QueryParams
			);

			if (bHitWater)
			{
				OnEnterWater(Cast<AWaterBody>(WaterHit.GetActor()), WaterHit.ImpactPoint);
				PrevTickLocation = CurrentLoc;
				return;
			}
			// (2) 하강 비행 중 예상 수면 높이(ExpectedWaterZ) 통과 시 정밀 서브프레임 보간 안착
			else if (bHasExpectedWaterZ && CurrentLoc.Z <= ExpectedWaterZ)
			{
				FVector LandingLoc = CurrentLoc;
				if (!PrevTickLocation.IsZero() && FMath::Abs(CurrentLoc.Z - PrevTickLocation.Z) > KINDA_SMALL_NUMBER)
				{
					const float Alpha = FMath::Clamp((ExpectedWaterZ - PrevTickLocation.Z) / (CurrentLoc.Z - PrevTickLocation.Z), 0.0f, 1.0f);
					LandingLoc = FMath::Lerp(PrevTickLocation, CurrentLoc, Alpha);
				}
				LandingLoc.Z = ExpectedWaterZ;
				OnEnterWater(nullptr, LandingLoc);
				PrevTickLocation = CurrentLoc;
				return;
			}
		}

		PrevTickLocation = CurrentLoc;
		return;
	}

	// 2. 수면 안착 후: 부유(Bobbing) 및 입질 꿀렁임 물리
	RunningTime += DeltaTime;

	// (1) 실시간 WaterBody 표면 높이 추적 (유효 범위 내 보정)
	if (CachedWaterBody && CachedWaterBody->GetWaterBodyComponent())
	{
		const float ConstantZ = CachedWaterBody->GetWaterBodyComponent()->GetConstantSurfaceZ();
		if (FMath::Abs(ConstantZ - BaseWaterZ) < 50.0f)
		{
			BaseWaterZ = ConstantZ;
		}
	}

	// (2) 찰랑거리는 잔물결 오르내림 파동 연산
	const float SineWave = FMath::Sin(RunningTime * BobbingSpeed) * BobbingAmplitude;

	// (3) 입질 시 아래로 쑥 들어가는 깊이 보간
	const float TargetSubmerge = bIsBiting ? BiteSubmergeDepth : 0.0f;
	CurrentSubmergeOffset = FMath::FInterpTo(CurrentSubmergeOffset, TargetSubmerge, DeltaTime, 12.0f);

	// (4) 최종 Z 높이 적용 (수면에 완벽 고정되어 가라앉지 않음)
	FVector CurrentLoc = GetActorLocation();
	CurrentLoc.Z = BaseWaterZ + SineWave - CurrentSubmergeOffset;
	SetActorLocation(CurrentLoc);

	// (5) 미니게임 중 물고기 도망 방향으로 찌가 기울어지는 시각 피드백 (화면 글자 없이도 찌만 보고 판단 가능)
	const float TargetRoll = CurrentFishDirection * 24.0f;
	CurrentRollTilt = FMath::FInterpTo(CurrentRollTilt, TargetRoll, DeltaTime, 6.0f);

	const float PitchTilt = FMath::Cos(RunningTime * BobbingSpeed) * 4.0f;
	SetActorRotation(FRotator(PitchTilt, 0.0f, CurrentRollTilt));

	// (6) 미니게임 진행 중 찌 주변 수면 물살 파문 시각 피드백
	if (FMath::Abs(CurrentFishDirection) > 0.05f)
	{
		const FVector SurfacePos = FVector(CurrentLoc.X, CurrentLoc.Y, BaseWaterZ + 1.5f);

		// 올바른 저항 중: 안정적인 청록/에메랄드빛 물살
		// 잘못된 저항/과장력: 주황/붉은색 물보라 파문
		const FColor RippleColor = (CurrentTensionPercent > 0.8f) ? FColor::Red : (bCorrectResistFeedback ? FColor::Cyan : FColor(255, 130, 0));
		const float RippleRadius = FMath::Lerp(20.0f, 36.0f, CurrentTensionPercent);

		DrawDebugCircle(GetWorld(), SurfacePos, RippleRadius, 16, RippleColor, false, 0.0f, 0, 2.2f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}
}

bool AFishingBobber::CheckHasEscapedWater() const
{
	if (!bIsInWater || !GetWorld()) return false;

	const FVector BobberLoc = GetActorLocation();

	// 찌 위치 기준 상하 수직 라인 트레이스 (수면 위 150cm ~ 수면 아래 300cm)
	const FVector TraceStart = FVector(BobberLoc.X, BobberLoc.Y, BaseWaterZ + 150.0f);
	const FVector TraceEnd = FVector(BobberLoc.X, BobberLoc.Y, BaseWaterZ - 300.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (OwnerRod) QueryParams.AddIgnoredActor(OwnerRod);
	if (GetOwner()) QueryParams.AddIgnoredActor(GetOwner());
	if (GetInstigator()) QueryParams.AddIgnoredActor(GetInstigator());

	// 1. 수면(ECC_Water) 트레이스
	FHitResult WaterHit;
	const bool bHitWater = GetWorld()->LineTraceSingleByChannel(
		WaterHit,
		TraceStart,
		TraceEnd,
		ECC_Water,
		QueryParams
	);

	// 찌 아래에 더 이상 Water(수면)가 감지되지 않으면 물 밖 육지/부두로 완전히 탈출한 것임
	if (!bHitWater)
	{
		return true;
	}

	// 2. 지형/바닥(ECC_Visibility) 트레이스
	FHitResult GroundHit;
	const bool bHitGround = GetWorld()->LineTraceSingleByChannel(
		GroundHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	if (bHitGround)
	{
		// 지형 바닥 높이가 수면 높이보다 높거나 같으면 (육지/해안선 둔덕 위로 올라옴) 물 탈출
		if (GroundHit.ImpactPoint.Z >= WaterHit.ImpactPoint.Z - 2.0f)
		{
			return true;
		}
	}

	return false;
}
