/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/HeldItem/FishingRod.h"
#include "Components/StaticMeshComponent.h"
#include "CableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Character/ActionCharacter.h"
#include "Component/InventoryComponent.h"
#include "Item/HeldItem/FishingBobber.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Data/Item/ItemDataBase.h"

AFishingRod::AFishingRod()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 낚싯대 메시 (기본 낚싯대 형태 실린더)
	RodMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RodMesh"));
	RootComponent = RodMesh;
	RodMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		RodMesh->SetStaticMesh(CylinderMeshFinder.Object);
		RodMesh->SetRelativeScale3D(FVector(0.02f, 0.02f, 1.2f)); // 1.2m 얇은 낚싯대
		RodMesh->SetRelativeRotation(FRotator(45.f, 0.f, 0.f));
	}

	// 2. 낚싯줄 케이블
	FishingLineCable = CreateDefaultSubobject<UCableComponent>(TEXT("FishingLineCable"));
	FishingLineCable->SetupAttachment(RootComponent, RodTipSocketName);
	FishingLineCable->CableWidth = 1.2f;
	FishingLineCable->NumSegments = 8;
	FishingLineCable->SolverIterations = 4;
	FishingLineCable->bEnableStiffness = false;
	FishingLineCable->SetVisibility(false);

	CurrentState = EFishingState::Idle;
	bReplicates = true;

	BobberClass = AFishingBobber::StaticClass();
}

void AFishingRod::BeginPlay()
{
	Super::BeginPlay();

	if (OwnerCharacter)
	{
		TryInitializeInputs();
	}
}

void AFishingRod::OnEquipped(AActionCharacter* InCharacter)
{
	Super::OnEquipped(InCharacter);

	if (OwnerCharacter)
	{
		TryInitializeInputs();
	}
}

void AFishingRod::OnUnequipped()
{
	ResetFishing();
	PopFishingInputContext();
	Super::OnUnequipped();
}

void AFishingRod::SetupInputComponent(UEnhancedInputComponent* PlayerEIC)
{
	if (PlayerEIC)
	{
		BindRodInputs(PlayerEIC);
		bInputInitialized = true;
		UE_LOG(LogTemp, Display, TEXT("[낚싯대] SetupInputComponent: Enhanced Input 바인딩이 성공적으로 완료되었습니다."));
	}
}

void AFishingRod::TryInitializeInputs()
{
	if (bInputInitialized || !OwnerCharacter) return;

	// 1. 캐릭터의 InputComponent 시도
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(OwnerCharacter->InputComponent))
	{
		SetupInputComponent(EIC);
		return;
	}

	// 2. 컨트롤러의 InputComponent 시도
	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent))
		{
			SetupInputComponent(EIC);
		}
	}
}

void AFishingRod::OnPrimaryActionStarted()
{
	if (CurrentState == EFishingState::Minigame)
	{
		bIsReelingByLMB = true;
		Input_SetReeling(true);
	}
	else
	{
		Input_CastOrHook();
	}
}

void AFishingRod::OnPrimaryActionCompleted()
{
	bIsReelingByLMB = false;
	if (CurrentState == EFishingState::Minigame)
	{
		Input_SetReeling(false);
	}
}

void AFishingRod::OnMoveInput(const FVector2D& MoveValue)
{
	if (CurrentState == EFishingState::Minigame)
	{
		// A(-1.0) / D(+1.0) 좌우 저항
		Input_SetPull(MoveValue.X);

		// S키(MoveValue.Y < -0.1f) 후진 입력 시 릴 감기
		if (MoveValue.Y < -0.1f)
		{
			Input_SetReeling(true);
		}
		else if (!bIsReelingByLMB)
		{
			// 좌클릭으로 릴을 감고 있는 게 아니라면 S키를 뗐을 때 릴 감기 해제
			Input_SetReeling(false);
		}
	}
}

void AFishingRod::BindRodInputs(UEnhancedInputComponent* EIC)
{
	if (!EIC) return;

	if (IA_Fishing_Aim)
	{
		EIC->BindAction(IA_Fishing_Aim, ETriggerEvent::Started, this, &AFishingRod::Input_StartAim);
		EIC->BindAction(IA_Fishing_Aim, ETriggerEvent::Completed, this, &AFishingRod::Input_StopAim);
	}

	if (IA_Fishing_Cast)
	{
		EIC->BindAction(IA_Fishing_Cast, ETriggerEvent::Started, this, &AFishingRod::Input_OnCastStarted);
		EIC->BindAction(IA_Fishing_Cast, ETriggerEvent::Completed, this, &AFishingRod::Input_OnCastCompleted);
	}

	if (IA_Fishing_Reel)
	{
		EIC->BindAction(IA_Fishing_Reel, ETriggerEvent::Started, this, &AFishingRod::Input_OnReelTriggered);
		EIC->BindAction(IA_Fishing_Reel, ETriggerEvent::Triggered, this, &AFishingRod::Input_OnReelTriggered);
		EIC->BindAction(IA_Fishing_Reel, ETriggerEvent::Completed, this, &AFishingRod::Input_OnReelCompleted);
		EIC->BindAction(IA_Fishing_Reel, ETriggerEvent::Canceled, this, &AFishingRod::Input_OnReelCompleted);
	}

	if (IA_Fishing_Pull)
	{
		EIC->BindAction(IA_Fishing_Pull, ETriggerEvent::Triggered, this, &AFishingRod::Input_OnPullTriggered);
		EIC->BindAction(IA_Fishing_Pull, ETriggerEvent::Completed, this, &AFishingRod::Input_OnPullCompleted);
		EIC->BindAction(IA_Fishing_Pull, ETriggerEvent::Canceled, this, &AFishingRod::Input_OnPullCompleted);
	}

	if (IA_Fishing_Cancel)
	{
		EIC->BindAction(IA_Fishing_Cancel, ETriggerEvent::Started, this, &AFishingRod::Input_Cancel);
	}
}

void AFishingRod::Input_OnCastStarted()
{
	OnPrimaryActionStarted();
}

void AFishingRod::Input_OnCastCompleted()
{
	OnPrimaryActionCompleted();
}

void AFishingRod::Input_OnReelTriggered()
{
	Input_SetReeling(true);
}

void AFishingRod::Input_OnReelCompleted()
{
	if (!bIsReelingByLMB)
	{
		Input_SetReeling(false);
	}
}

void AFishingRod::Input_OnPullTriggered(const FInputActionValue& Value)
{
	const float PullAxis = Value.Get<float>();
	Input_SetPull(PullAxis);
}

void AFishingRod::Input_OnPullCompleted()
{
	Input_SetPull(0.0f);
}

void AFishingRod::PushFishingInputContext()
{
	if (!OwnerCharacter) return;
	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (IMC_Fishing && !Subsystem->HasMappingContext(IMC_Fishing))
			{
				Subsystem->AddMappingContext(IMC_Fishing, 10); // 높은 우선순위로 이동 입력 오버라이드
			}
		}
	}
}

void AFishingRod::PopFishingInputContext()
{
	if (!OwnerCharacter) return;
	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (IMC_Fishing)
			{
				Subsystem->RemoveMappingContext(IMC_Fishing);
			}
		}
	}
}

FVector AFishingRod::GetLocalRodTipOffset() const
{
	if (!CustomRodTipOffset.IsZero())
	{
		return CustomRodTipOffset;
	}

	if (RodMesh && RodMesh->DoesSocketExist(RodTipSocketName))
	{
		return RodMesh->GetSocketTransform(RodTipSocketName, RTS_Actor).GetLocation();
	}

	// 기본 실린더 메시 상단 (Z = +60cm, 앞쪽으로 30cm 오프셋)
	return FVector(30.f, 0.f, 60.f);
}

FVector AFishingRod::GetRodTipLocation() const
{
	if (!CustomRodTipOffset.IsZero())
	{
		return GetActorTransform().TransformPosition(CustomRodTipOffset);
	}

	if (RodMesh && RodMesh->DoesSocketExist(RodTipSocketName))
	{
		return RodMesh->GetSocketLocation(RodTipSocketName);
	}

	return GetActorTransform().TransformPosition(FVector(30.f, 0.f, 60.f));
}

void AFishingRod::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 지연 입력 초기화 안전장치
	if (!bInputInitialized && OwnerCharacter)
	{
		TryInitializeInputs();
	}

	// 1. 조준 중일 때 캐스팅 궤적 실시간 프리뷰 렌더링
	if (CurrentState == EFishingState::Aiming)
	{
		UpdateCastingTrajectory(DeltaTime);
	}
	// 2. 미니게임 진행 중일 때 장력 및 물고기 저항 틱 연산
	else if (CurrentState == EFishingState::Minigame)
	{
		UpdateMinigame(DeltaTime);
	}

	// 3. 낚싯줄 케이블 끝점 실시간 동기화
	if (FishingLineCable && FishingLineCable->IsVisible())
	{
		FishingLineCable->SetWorldLocation(GetRodTipLocation());
	}
}

// -------------------------------------------------------------
// 입력 핸들러
// -------------------------------------------------------------

void AFishingRod::Input_StartAim()
{
	// 낚시가 이미 진행 중이거나 쿨다운 중이면 조준 불가
	if (bIsFishingActive || bIsFinishCooldown) return;

	if (CurrentState == EFishingState::Idle)
	{
		CurrentState = EFishingState::Aiming;
		PushFishingInputContext();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Cyan, TEXT("[낚시] 우클릭 조준 시작: 착수 지점을 확인하세요. (좌클릭으로 투척)"));
		}
	}
}

void AFishingRod::Input_StopAim()
{
	// 캐스팅 전 순수 조준 상태에서만 우클릭을 뗐을 때 조준 취소 및 IMC 해제
	if (CurrentState == EFishingState::Aiming && !bIsFishingActive)
	{
		CurrentState = EFishingState::Idle;
		PopFishingInputContext();
	}
}

void AFishingRod::Input_CastOrHook()
{
	// 쿨다운 중이면 입력 차단
	if (bIsFinishCooldown) return;

	// 1. 조준 중 좌클릭 -> 찌 투척(Casting)
	if (CurrentState == EFishingState::Aiming)
	{
		if (!bValidWaterHit)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("[낚시] 물(수면)이 아닌 곳에는 캐스팅할 수 없습니다!"));
			}
			return;
		}

		const FVector TipLoc = GetRodTipLocation();
		const FVector TargetLoc = PredictedLandingLocation;

		// 물리적 포물선 초기 발사 속도 벡터 계산 (중력 -980 적용)
		FVector LaunchVelocity = FVector::ZeroVector;
		const float GravityZ = FMath::Abs(GetWorld()->GetGravityZ());
		const bool bHaveAim = UGameplayStatics::SuggestProjectileVelocity_CustomArc(
			this,
			LaunchVelocity,
			TipLoc,
			TargetLoc,
			GravityZ,
			0.5f // 50% 포물선 아크
		);

		if (!bHaveAim)
		{
			LaunchVelocity = (TargetLoc - TipLoc).GetSafeNormal() * 1500.0f + FVector(0.f, 0.f, 400.f);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = OwnerCharacter;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedBobber = GetWorld()->SpawnActor<AFishingBobber>(BobberClass, TipLoc, FRotator::ZeroRotator, SpawnParams);
		if (SpawnedBobber)
		{
			SpawnedBobber->SetOwnerRod(this);
			SpawnedBobber->LaunchBobber(LaunchVelocity);

			// 낚싯줄 연결 및 끝점 위치 동기화
			FishingLineCable->SetRelativeLocation(GetLocalRodTipOffset());
			FishingLineCable->SetVisibility(true);
			FishingLineCable->SetAttachEndTo(SpawnedBobber, NAME_None, NAME_None);

			CurrentState = EFishingState::Casting;
			bIsFishingActive = true; // ★ 낚시 세션 락 활성화
			PushFishingInputContext(); // ★ 캐스팅 중에도 IMC_Fishing 유지

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, TEXT("[낚시] 찌 투척 완료! 호수 수면으로 날아갑니다."));
			}
			UE_LOG(LogTemp, Display, TEXT("[낚시] 찌를 성공적으로 던졌습니다!"));
		}
	}
	// 2. 대기 중(입질 전) 좌클릭 -> 헛챔질! (너무 일찍 챔질하여 실패 종료)
	else if (CurrentState == EFishingState::WaitingBite)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("[낚시 실패] 헛챔질! 너무 일찍 챔질하여 물고기가 도망갔습니다."));
		}
		UE_LOG(LogTemp, Warning, TEXT("[낚시] 헛챔질! 너무 일찍 챔질하여 물고기가 도망갔습니다."));
		FinishFishing(false);
	}
	// 3. 입질(Biting) 중 좌클릭 -> 챔질(Hooking) 성공! ➔ 미니게임 돌입
	else if (CurrentState == EFishingState::Biting)
	{
		GetWorld()->GetTimerManager().ClearTimer(ReactionTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(BiteTimerHandle);

		if (OwnerCharacter && SpawnedBobber)
		{
			CurrentDistance = FVector::Dist2D(OwnerCharacter->GetActorLocation(), SpawnedBobber->GetActorLocation()) / 100.0f; // m 단위
		}
		else
		{
			CurrentDistance = 15.0f;
		}

		CurrentTension = 15.0f;
		FishEscapeDirection = (FMath::RandBool()) ? 1.0f : -1.0f;
		FishTurnTimer = FMath::RandRange(1.5f, 3.0f);
		PlayerPullInput = 0.0f;
		bIsReelingInput = false;
		bIsReelingByLMB = false;

		CurrentState = EFishingState::Minigame;
		PushFishingInputContext(); // ★ 미니게임 시작 시 IMC_Fishing 확실하게 유지

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Emerald, TEXT("[낚시] ★ 챔질 대성공! 미니게임 시작!\n[조작법] A/D: 물고기 반대 방향으로 저항 | S 또는 좌클릭 길게 누르기: 릴 감기 | Space: 취소"));
		}
		UE_LOG(LogTemp, Display, TEXT("[낚시] 챔질 대성공! 미니게임 시작! (거리: %.1fm)"), CurrentDistance);
	}
	// 4. 미니게임 중 좌클릭 입력 -> 릴 감기
	else if (CurrentState == EFishingState::Minigame)
	{
		bIsReelingByLMB = true;
		Input_SetReeling(true);
	}
}

void AFishingRod::Input_SetReeling(bool bReeling)
{
	bIsReelingInput = bReeling;
}

void AFishingRod::Input_SetPull(float PullAxis)
{
	PlayerPullInput = FMath::Clamp(PullAxis, -1.0f, 1.0f);
}

void AFishingRod::Input_Cancel()
{
	if (CurrentState != EFishingState::Idle)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("[낚시] 낚시를 취소했습니다."));
		}
		FinishFishing(false);
	}
}

void AFishingRod::OnBobberLandedInWater()
{
	if (CurrentState != EFishingState::Casting) return;

	CurrentState = EFishingState::WaitingBite;

	// 랜덤 입질 대기 타이머 시작 (3 ~ 7초)
	const float RandomWaitTime = FMath::RandRange(MinBiteWaitTime, MaxBiteWaitTime);
	GetWorld()->GetTimerManager().SetTimer(BiteTimerHandle, this, &AFishingRod::TriggerBite, RandomWaitTime, false);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("[낚시] 찌가 수면에 안착했습니다. 물고기 입질을 기다립니다..."));
	}
	UE_LOG(LogTemp, Display, TEXT("[낚시] 찌 착수 완료. %.1f초 후 입질 예정."), RandomWaitTime);
}

void AFishingRod::TriggerBite()
{
	if (CurrentState != EFishingState::WaitingBite) return;

	CurrentState = EFishingState::Biting;

	// 찌 첨벙 애니메이션 및 사운드 발동
	if (SpawnedBobber)
	{
		SpawnedBobber->SetBiting(true);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("★ [입질 발생!] 물고기가 물었습니다! 지금 좌클릭으로 챔질하세요!"));
	}
	UE_LOG(LogTemp, Display, TEXT("[낚시] 입질 발생! 챔질 유효 시간: %.1f초"), BiteReactionWindow);

	// 반응 제한 시간 초과 타이머 (2초 내에 안 누르면 놓침)
	GetWorld()->GetTimerManager().SetTimer(ReactionTimerHandle, this, &AFishingRod::OnBiteMissed, BiteReactionWindow, false);
}

void AFishingRod::OnBiteMissed()
{
	if (CurrentState == EFishingState::Biting)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("[낚시 실패] 챔질 타이밍을 놓쳤습니다! 물고기가 미끼를 물고 도망갔습니다."));
		}
		UE_LOG(LogTemp, Warning, TEXT("[낚시] 챔질 타이밍을 놓쳐 실패했습니다."));
		FinishFishing(false);
	}
}

// -------------------------------------------------------------
// 물리/렌더링/미니게임 핵심 연산
// -------------------------------------------------------------

void AFishingRod::UpdateCastingTrajectory(float DeltaTime)
{
	if (!OwnerCharacter || !GetWorld()) return;

	const FVector TipLoc = GetRodTipLocation();
	FRotator ControlRot = OwnerCharacter->GetControlRotation();

	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			ControlRot = PC->PlayerCameraManager->GetCameraRotation();
		}
	}

	// 1. 고정 프리뷰 위치 (플레이어 전방 FixedCastDistance)
	const FVector ForwardDir = ControlRot.Vector().GetSafeNormal2D();
	const FVector TargetHorizontal = OwnerCharacter->GetActorLocation() + ForwardDir * FixedCastDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.bTraceComplex = false;

	bValidWaterHit = false;
	FVector LandingPos = TargetHorizontal;

	// 2. WaterBodyCollision의 Overlap(겹침) 검출 (WaterBody는 Overlap이므로 OverlapMultiByProfile 사용)
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(40.0f, 250.0f);

	GetWorld()->OverlapMultiByProfile(
		OverlapResults,
		TargetHorizontal,
		FQuat::Identity,
		FName(TEXT("WaterBodyCollision")),
		CapsuleShape,
		QueryParams
	);

	for (const FOverlapResult& Overlap : OverlapResults)
	{
		if (Overlap.GetActor() && (Overlap.GetActor()->IsA(AWaterBody::StaticClass()) || Overlap.GetActor()->GetName().Contains(TEXT("Water"))))
		{
			bValidWaterHit = true;
			if (AWaterBody* WB = Cast<AWaterBody>(Overlap.GetActor()))
			{
				if (WB->GetWaterBodyComponent())
				{
					LandingPos.Z = WB->GetWaterBodyComponent()->GetConstantSurfaceZ();
				}
			}
			break;
		}
	}

	// OverlapMultiByChannel(ECC_WorldDynamic) 백업 검출
	if (!bValidWaterHit)
	{
		TArray<FOverlapResult> DynamicOverlaps;
		GetWorld()->OverlapMultiByChannel(
			DynamicOverlaps,
			TargetHorizontal,
			FQuat::Identity,
			ECC_WorldDynamic,
			CapsuleShape,
			QueryParams
		);

		for (const FOverlapResult& Overlap : DynamicOverlaps)
		{
			if (Overlap.GetActor() && Overlap.GetActor()->IsA(AWaterBody::StaticClass()))
			{
				bValidWaterHit = true;
				if (AWaterBody* WB = Cast<AWaterBody>(Overlap.GetActor()))
				{
					if (WB->GetWaterBodyComponent())
					{
						LandingPos.Z = WB->GetWaterBodyComponent()->GetConstantSurfaceZ();
					}
				}
				break;
			}
		}
	}

	PredictedLandingLocation = LandingPos;

	// 3. 부드러운 2차 베지어 포물선 아크 렌더링 (LifeTime = 0.0f로 잔상 100% 제거)
	const FColor ArcColor = bValidWaterHit ? FColor::Emerald : FColor(255, 50, 50);
	const FVector MidPoint = (TipLoc + LandingPos) * 0.5f + FVector(0.f, 0.f, 250.f);

	const int32 NumSegments = 24;
	FVector PrevPoint = TipLoc;

	for (int32 i = 1; i <= NumSegments; ++i)
	{
		const float T = static_cast<float>(i) / NumSegments;
		const FVector CurrPoint = FMath::Square(1.0f - T) * TipLoc + 2.0f * (1.0f - T) * T * MidPoint + FMath::Square(T) * LandingPos;

		DrawDebugLine(GetWorld(), PrevPoint, CurrPoint, ArcColor, false, 0.0f, 0, 4.5f);
		PrevPoint = CurrPoint;
	}

	// 4. 착수 지점 마커 렌더링 (LifeTime = 0.0f)
	if (bValidWaterHit)
	{
		DrawDebugCircle(GetWorld(), LandingPos + FVector(0.f, 0.f, 5.f), 40.0f, 24, FColor::Emerald, false, 0.0f, 0, 4.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		DrawDebugCircle(GetWorld(), LandingPos + FVector(0.f, 0.f, 5.f), 18.0f, 16, FColor::Cyan, false, 0.0f, 0, 3.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		DrawDebugCylinder(GetWorld(), LandingPos, LandingPos + FVector(0.f, 0.f, 30.f), 40.0f, 16, FColor::Emerald, false, 0.0f, 0, 2.5f);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(200, 0.05f, FColor::Green, TEXT("[조준 완료] 수면 포착! 좌클릭으로 캐스팅하세요!"));
		}
	}
	else
	{
		DrawDebugCircle(GetWorld(), LandingPos + FVector(0.f, 0.f, 5.f), 30.0f, 16, FColor(255, 50, 50), false, 0.0f, 0, 3.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(200, 0.05f, FColor::Red, TEXT("[조준 불가] 물(수면)을 향해 조준하세요!"));
		}
	}
}

void AFishingRod::UpdateMinigame(float DeltaTime)
{
	if (!OwnerCharacter || !SpawnedBobber) return;

	// 1. 물고기 도망 방향 주기적 전환 (1.5 ~ 3.0초 주기)
	FishTurnTimer -= DeltaTime;
	if (FishTurnTimer <= 0.0f)
	{
		FishEscapeDirection *= -1.0f; // 반대 방향으로 급선회
		FishTurnTimer = FMath::RandRange(2.0f, 4.0f);
	}

	// 2. 플레이어의 저항 방향이 물고기 반대 방향인지 판별
	// (물고기가 우측(+1)으로 도망갈 때 플레이어가 좌측 A(-1)를 당기면 곱이 음수 -> 올바른 저항)
	const bool bResistingCorrectly = (FishEscapeDirection * PlayerPullInput < -0.1f);

	// 3. 릴 감기(S / LMB) 및 장력 계산
	if (bIsReelingInput)
	{
		// 거리 좁힘
		CurrentDistance -= ReelSpeed * DeltaTime;

		// 릴링 시 장력 증가 (올바른 방향이면 12%/s, 잘못된 방향이면 35%/s 급상승)
		const float TensionGain = bResistingCorrectly ? TensionGainCorrect : TensionGainWrong;
		CurrentTension += TensionGain * DeltaTime;
	}
	else
	{
		// 릴을 안 감으면 장력 자연 감소
		CurrentTension -= TensionDecayRate * DeltaTime;

		// 대신 물고기가 낚싯줄을 끌고 조금씩 멀어짐
		CurrentDistance += FishPullSpeed * DeltaTime;
	}

	CurrentTension = FMath::Clamp(CurrentTension, 0.0f, 100.0f);

	// 실시간 미니게임 HUD 디버그 출력
	if (GEngine)
	{
		const FString FishDirStr = (FishEscapeDirection > 0.0f) ? TEXT("우측 -> [A키 당기기!]") : TEXT("좌측 <- [D키 당기기!]");
		FString MyInputStr = TEXT("중립");
		if (PlayerPullInput < -0.1f) MyInputStr = TEXT("A키 당김(좌)");
		else if (PlayerPullInput > 0.1f) MyInputStr = TEXT("D키 당김(우)");

		const FString ReelStr = bIsReelingInput ? TEXT("릴 감는 중(S) O") : TEXT("릴 정지 X");
		const FString ResistEvalStr = bResistingCorrectly ? TEXT("★ 올바른 저항! (텐션 완만)") : TEXT("▲ 잘못된 저항! (텐션 급상승)");

		GEngine->AddOnScreenDebugMessage(100, 0.1f, bResistingCorrectly ? FColor::Green : FColor::Orange,
			FString::Printf(TEXT("[미니게임] 거리: %.1fm | 텐션: %.0f%% | 물고기: %s | 내입력: [%s / %s] | %s"),
				CurrentDistance, CurrentTension, *FishDirStr, *ReelStr, *MyInputStr, *ResistEvalStr));
	}

	// 4. 찌의 수평 위치 업데이트 및 최소 접근 거리 한계 제어:
	const float ActualDist2D = FVector::Dist2D(OwnerCharacter->GetActorLocation(), SpawnedBobber->GetActorLocation());
	const FVector ToPlayer = (OwnerCharacter->GetActorLocation() - SpawnedBobber->GetActorLocation()).GetSafeNormal2D();
	const FVector RightVec = FVector::CrossProduct(ToPlayer, FVector::UpVector);

	// (1) 물고기 좌우 흔들기
	SpawnedBobber->AddActorWorldOffset(RightVec * FishEscapeDirection * 150.0f * DeltaTime);

	// (2) ★ S키(릴 감기) 시 플레이어 앞 MinCatchDistance(2.0m)까지만 안전하게 견인 (몸속 파고들기 차단)
	if (bIsReelingInput && ActualDist2D > MinCatchDistance)
	{
		SpawnedBobber->AddActorWorldOffset(ToPlayer * ReelSpeed * 100.0f * DeltaTime);
	}

	// 5. 성공/실패 판정
	if (CurrentTension >= 100.0f)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("[낚시 실패] 낚싯줄이 끊어졌습니다!"));
		UE_LOG(LogTemp, Warning, TEXT("[낚시] 장력이 100%%를 초과하여 낚싯줄이 끊어졌습니다! (실패)"));
		FinishFishing(false);
	}
	else if (ActualDist2D <= MinCatchDistance || CurrentDistance <= (MinCatchDistance / 100.0f))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, TEXT("[낚시 대성공!] 물고기를 낚아 올렸습니다! (인벤토리에 생고기 획득)"));
		UE_LOG(LogTemp, Display, TEXT("[낚시] 찌가 발앞 %.1fm에 도달하여 물고기를 건져 올렸습니다! (성공!)"), MinCatchDistance / 100.0f);
		FinishFishing(true);
	}
}

void AFishingRod::FinishFishing(bool bSuccess)
{
	GetWorld()->GetTimerManager().ClearTimer(BiteTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ReactionTimerHandle);

	if (bSuccess)
	{
		CurrentState = EFishingState::ReelingSuccess;

		// 서버 권한으로 보상 아이템 지급 요청
		Server_FinishFishing(true);
	}
	else
	{
		CurrentState = EFishingState::Failed;
	}

	ResetFishing();

	// 낚시 종료 직후 0.6초간 이동 차단 완충 쿨다운 적용
	bIsFinishCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(
		FinishCooldownTimerHandle,
		this,
		&AFishingRod::OnFinishCooldownEnded,
		0.6f,
		false
	);
}

void AFishingRod::OnFinishCooldownEnded()
{
	bIsFinishCooldown = false;
	UE_LOG(LogTemp, Display, TEXT("[낚시] 쿨다운 종료. 이제 이동이 가능합니다."));
}

bool AFishingRod::Server_FinishFishing_Validate(bool bSuccess)
{
	// 낚시 세션(bIsFishingActive)이 실제로 활성화되어 있었는지 서버에서 유효성 검증
	return bIsFishingActive;
}

void AFishingRod::Server_FinishFishing_Implementation(bool bSuccess)
{
	if (!bSuccess) return;

	// 서버 권한으로 캐릭터 인벤토리에 생고기(DA_Item_Consumable_RawMeat) 지급
	if (OwnerCharacter && FishRewardItemData)
	{
		if (UInventoryComponent* InvComp = OwnerCharacter->FindComponentByClass<UInventoryComponent>())
		{
			int32 RemainCount = 0;
			const bool bAdded = InvComp->AddItem(FishRewardItemData, 1, RemainCount);
			if (bAdded)
			{
				UE_LOG(LogTemp, Display, TEXT("[낚시 서버] 인벤토리에 보상 [%s] 지급 완료!"), *FishRewardItemData->DisplayName.ToString());
			}
		}
	}
}

void AFishingRod::ResetFishing()
{
	bIsFishingActive = false; // ★ 낚시 세션 락 해제
	CurrentState = EFishingState::Idle;
	CurrentTension = 0.0f;
	CurrentDistance = 0.0f;
	PlayerPullInput = 0.0f;
	bIsReelingInput = false;
	bIsReelingByLMB = false;

	if (FishingLineCable)
	{
		FishingLineCable->SetVisibility(false);
		FishingLineCable->SetAttachEndTo(nullptr, NAME_None, NAME_None);
	}

	if (SpawnedBobber)
	{
		SpawnedBobber->Destroy();
		SpawnedBobber = nullptr;
	}

	PopFishingInputContext();
}
