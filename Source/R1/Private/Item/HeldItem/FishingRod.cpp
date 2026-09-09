/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/HeldItem/FishingRod.h"
#include "R1/R1.h"
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
#include "Net/UnrealNetwork.h"


AFishingRod::AFishingRod()
{
	PrimaryActorTick.bCanEverTick = true;

	// 2. 낚싯줄 케이블 (중간 꺾임/처짐이 없는 팽팽한 1세그먼트 직선 낚싯줄)
	FishingLineCable = CreateDefaultSubobject<UCableComponent>(TEXT("FishingLineCable"));
	FishingLineCable->SetupAttachment(RootComponent, RodTipSocketName);
	FishingLineCable->CableWidth = 0.6f;
	FishingLineCable->NumSegments = 1; // 1개 세그먼트: 로드 끝 ~ 찌 직결 팽팽한 직선
	FishingLineCable->SolverIterations = 1;
	FishingLineCable->bEnableStiffness = true;
	FishingLineCable->CableGravityScale = 0.0f; // 중력 처짐/출렁임 완전 제거
	FishingLineCable->EndLocation = FVector(0.f, 0.f, 5.0f); // 찌 상단에 정확히 연결
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
	if (bIsFishingActive || CurrentState != EFishingState::Idle)
	{
		Server_FinishFishing(false);
	}
	ResetFishing();
	PopFishingInputContext();
	Super::OnUnequipped();
}

void AFishingRod::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BiteTimerHandle);
		World->GetTimerManager().ClearTimer(ReactionTimerHandle);
		World->GetTimerManager().ClearTimer(FinishCooldownTimerHandle);
	}

	if (SpawnedBobber && IsValid(SpawnedBobber))
	{
		SpawnedBobber->Destroy();
		SpawnedBobber = nullptr;
	}

	PopFishingInputContext();

	Super::EndPlay(EndPlayReason);
}

void AFishingRod::SetupInputComponent(UEnhancedInputComponent* PlayerEIC)
{
	if (PlayerEIC)
	{
		BindRodInputs(PlayerEIC);
		bInputInitialized = true;
		//UE_LOG(LogTemp, Display, TEXT("[낚싯대] SetupInputComponent: Enhanced Input 바인딩이 성공적으로 완료되었습니다."));
	}
}

void AFishingRod::TryInitializeInputs()
{
	if (bInputInitialized || !OwnerCharacter || !OwnerCharacter->IsLocallyControlled()) return;

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

	if (ItemMesh1P && ItemMesh1P->DoesSocketExist(RodTipSocketName))
	{
		return ItemMesh1P->GetSocketTransform(RodTipSocketName, RTS_Actor).GetLocation();
	}

	// 기본 실린더 메시 상단 (Z = +60cm, 앞쪽으로 30cm 오프셋)
	return FVector(30.f, 0.f, 60.f);
}

void AFishingRod::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFishingRod, CurrentState);
	DOREPLIFETIME(AFishingRod, bIsFishingActive);
	DOREPLIFETIME_CONDITION(AFishingRod, AnimPullInput, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AFishingRod, bIsReelingInput, COND_SkipOwner);
}

void AFishingRod::SetFishingState(EFishingState NewState)
{
	CurrentState = NewState;
	bIsFishingActive = (NewState == EFishingState::Casting ||
	                    NewState == EFishingState::WaitingBite ||
	                    NewState == EFishingState::Biting ||
	                    NewState == EFishingState::Minigame);

	if (!HasAuthority())
	{
		Server_SetFishingState(NewState);
	}
}

void AFishingRod::Server_SetFishingState_Implementation(EFishingState NewState)
{
	CurrentState = NewState;
	bIsFishingActive = (NewState == EFishingState::Casting ||
	                    NewState == EFishingState::WaitingBite ||
	                    NewState == EFishingState::Biting ||
	                    NewState == EFishingState::Minigame);
}

void AFishingRod::Server_SetPull_Implementation(float PullAxis)
{
	PlayerPullInput = FMath::Clamp(PullAxis, -1.0f, 1.0f);
}

void AFishingRod::Server_SetReeling_Implementation(bool bReeling)
{
	bIsReelingInput = bReeling;
}

void AFishingRod::OnRep_CurrentState(EFishingState NewState)
{
	bIsFishingActive = (NewState == EFishingState::Casting ||
	                    NewState == EFishingState::WaitingBite ||
	                    NewState == EFishingState::Biting ||
	                    NewState == EFishingState::Minigame);
}

FVector AFishingRod::GetRodTipLocation() const
{
	if (!CustomRodTipOffset.IsZero())
	{
		return GetActorTransform().TransformPosition(CustomRodTipOffset);
	}

	if (ItemMesh1P && ItemMesh1P->DoesSocketExist(RodTipSocketName))
	{
		return ItemMesh1P->GetSocketLocation(RodTipSocketName);
	}

	return GetActorTransform().TransformPosition(FVector(30.f, 0.f, 60.f));
}

void AFishingRod::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 릴링 여부에 따른 애니메이션용 좌우 기울기(Pull) 감쇄 및 부드러운 보간
	// (릴을 감는 중에는 ReelingTiltDamping(기본 25%) 비율로 감쇄하여 낚싯대 흔들림 방지 및 자연스러운 자세 유지)
	const float TargetAnimPull = IsReelingInput() ? (PlayerPullInput * ReelingTiltDamping) : PlayerPullInput;
	AnimPullInput = FMath::FInterpTo(AnimPullInput, TargetAnimPull, DeltaTime, TiltInterpSpeed);

	// 로컬에서 직접 조종하는 내 캐릭터가 아니라면 로컬 렌더링/미니게임 연산 건너뛰기 (멀티플레이 격리)
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	// 지연 입력 초기화 안전장치
	if (!bInputInitialized)
	{
		TryInitializeInputs();
	}

	// 1. 조준 중일 때 캐스팅 궤적 실시간 프리뷰 렌더링 (내 화면에만 렌더링)
	if (CurrentState == EFishingState::Aiming)
	{
		UpdateCastingTrajectory(DeltaTime);
	}
	// 2. 미니게임 진행 중일 때 장력 및 물고기 저항 틱 연산 (내 로컬 세션에서만 연산)
	else if (CurrentState == EFishingState::Minigame)
	{
		UpdateMinigame(DeltaTime);
	}

	// 3. 낚싯줄 케이블 끝점 실시간 동기화
	if (FishingLineCable && FishingLineCable->IsVisible())
	{
		const FVector RodTipLoc = GetRodTipLocation();
		FishingLineCable->SetWorldLocation(RodTipLoc);

		if (SpawnedBobber)
		{
			const FVector BobberLoc = SpawnedBobber->GetActorLocation();
			const float LineDist = FVector::Dist(RodTipLoc, BobberLoc);
			FishingLineCable->CableLength = LineDist;
			FishingLineCable->EndLocation = FVector(0.f, 0.f, 5.0f);
		}
	}
}

// -------------------------------------------------------------
// 입력 핸들러
// -------------------------------------------------------------

void AFishingRod::StartAim()
{
	// 낚시가 이미 진행 중이거나 쿨다운 중이면 조준 불가
	if (bIsFishingActive || bIsFinishCooldown) return;

	if (CurrentState == EFishingState::Idle)
	{
		SetFishingState(EFishingState::Aiming);
		PushFishingInputContext();

		if (GEngine && OwnerCharacter && OwnerCharacter->IsLocallyControlled())
		{
			GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Cyan, TEXT("[낚시] 우클릭 조준 시작: 착수 지점을 확인하세요. (좌클릭으로 투척)"));
		}
	}
}

void AFishingRod::StopAim()
{
	// 캐스팅 전 순수 조준 상태에서만 우클릭을 뗐을 때 조준 취소 및 IMC 해제
	if (CurrentState == EFishingState::Aiming && !bIsFishingActive)
	{
		SetFishingState(EFishingState::Idle);
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

		// 물리적 포물선 초기 발사 속도 벡터 정밀 계산 (프리뷰 궤적과 100% 일치)
		FVector LaunchVelocity = FVector::ZeroVector;
		float FlightTime = 0.0f;
		CalculateCastVelocity(TipLoc, TargetLoc, LaunchVelocity, FlightTime);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = OwnerCharacter;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedBobber = GetWorld()->SpawnActor<AFishingBobber>(BobberClass, TipLoc, FRotator::ZeroRotator, SpawnParams);
		if (SpawnedBobber)
		{
			SpawnedBobber->SetOwnerRod(this);
			SpawnedBobber->SetExpectedWaterZ(TargetLoc.Z);
			SpawnedBobber->LaunchBobber(LaunchVelocity, TargetLoc.Z);

			// 낚싯줄 연결 및 끝점 위치 정밀 동기화
			FishingLineCable->EndLocation = FVector(0.f, 0.f, 5.0f);
			FishingLineCable->CableLength = 20.0f;
			FishingLineCable->SetWorldLocation(TipLoc);
			FishingLineCable->SetAttachEndToComponent(SpawnedBobber->GetRootComponent());
			FishingLineCable->SetVisibility(true);

			SetFishingState(EFishingState::Casting);
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

		SetFishingState(EFishingState::Minigame);
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
	if (bIsReelingInput != bReeling)
	{
		bIsReelingInput = bReeling;
		if (!HasAuthority())
		{
			Server_SetReeling(bIsReelingInput);
		}
	}
}

void AFishingRod::Input_SetPull(float PullAxis)
{
	const float ClampedAxis = FMath::Clamp(PullAxis, -1.0f, 1.0f);
	if (!FMath::IsNearlyEqual(PlayerPullInput, ClampedAxis, 0.01f))
	{
		PlayerPullInput = ClampedAxis;
		if (!HasAuthority())
		{
			Server_SetPull(PlayerPullInput);
		}
	}
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

	SetFishingState(EFishingState::WaitingBite);

	if (FishingLineCable && SpawnedBobber)
	{
		const float LineDist = FVector::Dist(GetRodTipLocation(), SpawnedBobber->GetActorLocation());
		FishingLineCable->CableLength = LineDist * 1.005f;
		FishingLineCable->EndLocation = FVector(0.f, 0.f, 5.0f);
	}

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

	SetFishingState(EFishingState::Biting);

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

bool AFishingRod::CalculateCastVelocity(const FVector& StartPos, const FVector& TargetPos, FVector& OutVelocity, float& OutFlightTime) const
{
	if (!GetWorld()) return false;

	const float WorldGravityZ = GetWorld()->GetGravityZ();
	const float AbsG = FMath::Max(FMath::Abs(WorldGravityZ), 100.0f);
	const float Dist2D = FVector::Dist2D(StartPos, TargetPos);

	// 거리 비례 자연스러운 포물선 정점 높이 설정 (1.2m ~ 4.0m)
	const float PeakHeight = FMath::Clamp(Dist2D * 0.22f, 120.0f, 400.0f);
	const float EffectivePeakZ = FMath::Max(StartPos.Z + PeakHeight, TargetPos.Z + 60.0f);

	// 1. 발사점에서 정점까지 도달하는 수직 상승 속도 및 시간
	const float RiseHeight = FMath::Max(EffectivePeakZ - StartPos.Z, 10.0f);
	const float Vz = FMath::Sqrt(2.0f * AbsG * RiseHeight);
	const float TimeUp = Vz / AbsG;

	// 2. 정점에서 타겟 수면 높이까지 낙하하는 시간
	const float DropHeight = FMath::Max(EffectivePeakZ - TargetPos.Z, 10.0f);
	const float TimeDown = FMath::Sqrt(2.0f * DropHeight / AbsG);

	OutFlightTime = FMath::Max(TimeUp + TimeDown, 0.1f);

	// 3. 비행 시간 동안 정확히 타겟 (X, Y)에 도달하는 수평 속도
	const float InvTime = 1.0f / OutFlightTime;
	OutVelocity.X = (TargetPos.X - StartPos.X) * InvTime;
	OutVelocity.Y = (TargetPos.Y - StartPos.Y) * InvTime;
	OutVelocity.Z = Vz;

	return true;
}

void AFishingRod::UpdateCastingTrajectory(float DeltaTime)
{
	if (!OwnerCharacter || !GetWorld()) return;

	const FVector TipLoc = GetRodTipLocation();
	FVector CamLoc = OwnerCharacter->GetActorLocation();
	FRotator CamRot = OwnerCharacter->GetControlRotation();

	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			CamLoc = PC->PlayerCameraManager->GetCameraLocation();
			CamRot = PC->PlayerCameraManager->GetCameraRotation();
		}
	}

	const FVector CamForward = CamRot.Vector();
	// 최대 캐스팅 거리보다 여유 있게 트레이스를 쏴서 먼 수면도 감지 후 "거리 초과"로 안내할 수 있게 함
	const float TraceDistance = FMath::Max(MaxCastDistance * 1.5f, 3500.0f);
	const FVector TraceStart = CamLoc;
	const FVector TraceEnd = CamLoc + (CamForward * TraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.bTraceComplex = false;

	// 1. 화면 중앙에서 Water 채널(ECC_Water / ECC_GameTraceChannel3)로 트레이스 발사
	FHitResult WaterHit;
	const bool bHitWater = GetWorld()->LineTraceSingleByChannel(
		WaterHit,
		TraceStart,
		TraceEnd,
		ECC_Water,
		QueryParams
	);

	// 2. 시야 차단 여부 검사용 가시성 트레이스
	FHitResult VisHit;
	const bool bHitVis = GetWorld()->LineTraceSingleByChannel(
		VisHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	FVector LandingPos = FVector::ZeroVector;
	FString DebugStateMessage;
	FColor DebugMessageColor = FColor::White;

	// 물(수면)에 맞았고, 전방에 지형이나 바위 등 가로막는 물체가 수면보다 앞에 있지 않은 경우
	if (bHitWater && (!bHitVis || WaterHit.Distance <= VisHit.Distance + 10.0f))
	{
		LandingPos = WaterHit.ImpactPoint;

		// AWaterBody인 경우 일정한 수면 높이(ConstantSurfaceZ) 보정 적용
		if (AWaterBody* WB = Cast<AWaterBody>(WaterHit.GetActor()))
		{
			if (WB->GetWaterBodyComponent())
			{
				const float SurfaceZ = WB->GetWaterBodyComponent()->GetConstantSurfaceZ();
				if (FMath::Abs(SurfaceZ - WaterHit.ImpactPoint.Z) < 50.0f)
				{
					LandingPos.Z = SurfaceZ;
				}
			}
		}

		// 수평 캐스팅 거리 검증 (캐릭터 위치 기준)
		const float CastDistance2D = FVector::Dist2D(OwnerCharacter->GetActorLocation(), LandingPos);

		if (CastDistance2D > MaxCastDistance)
		{
			bValidWaterHit = false;
			DebugStateMessage = FString::Printf(TEXT("[조준 불가] 최대 거리 초과! (현재: %.1fm / 최대: %.1fm)"), CastDistance2D / 100.0f, MaxCastDistance / 100.0f);
			DebugMessageColor = FColor(255, 100, 100);
		}
		else if (CastDistance2D < MinCastDistance)
		{
			bValidWaterHit = false;
			DebugStateMessage = FString::Printf(TEXT("[조준 불가] 너무 가깝습니다! (현재: %.1fm / 최소: %.1fm)"), CastDistance2D / 100.0f, MinCastDistance / 100.0f);
			DebugMessageColor = FColor(255, 100, 100);
		}
		else
		{
			bValidWaterHit = true;
			DebugStateMessage = FString::Printf(TEXT("[조준 완료] 수면 포착 (거리: %.1fm)! 좌클릭으로 캐스팅하세요."), CastDistance2D / 100.0f);
			DebugMessageColor = FColor::Green;
		}
	}
	else if (bHitVis)
	{
		// 지형이나 바위, 육지 등 물이 아닌 일반 오브젝트에 닿은 경우
		LandingPos = VisHit.ImpactPoint;
		bValidWaterHit = false;
		DebugStateMessage = TEXT("[조준 불가] 물(수면)이 아닙니다!");
		DebugMessageColor = FColor::Red;
	}
	else
	{
		// 허공이나 하늘을 조준 중인 경우
		LandingPos = CamLoc + CamForward * MaxCastDistance;
		bValidWaterHit = false;
		DebugStateMessage = TEXT("[조준 불가] 물(수면)을 향해 조준하세요!");
		DebugMessageColor = FColor::Red;
	}

	PredictedLandingLocation = LandingPos;

	// 3. 물리 발사체와 100% 동일한 포물선 궤적 연산 및 프리뷰 라인 렌더링
	const FColor ArcColor = bValidWaterHit ? FColor::Emerald : FColor(255, 50, 50);

	FVector PreviewVelocity = FVector::ZeroVector;
	float FlightTime = 0.0f;
	CalculateCastVelocity(TipLoc, LandingPos, PreviewVelocity, FlightTime);

	const int32 NumSegments = 28;
	const float TimeStep = FlightTime / NumSegments;
	const FVector GravityVec = FVector(0.f, 0.f, GetWorld()->GetGravityZ());

	FVector PrevPoint = TipLoc;
	for (int32 i = 1; i <= NumSegments; ++i)
	{
		const float SimTime = i * TimeStep;
		const FVector CurrPoint = TipLoc + PreviewVelocity * SimTime + 0.5f * GravityVec * FMath::Square(SimTime);

		DrawDebugLine(GetWorld(), PrevPoint, CurrPoint, ArcColor, false, 0.0f, 0, 4.0f);
		PrevPoint = CurrPoint;
	}

	// 4. 착수 지점 마커 렌더링 (LifeTime = 0.0f로 잔상 방지)
	if (bValidWaterHit)
	{
		DrawDebugCircle(GetWorld(), LandingPos + FVector(0.f, 0.f, 5.f), 40.0f, 24, FColor::Emerald, false, 0.0f, 0, 4.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		DrawDebugCircle(GetWorld(), LandingPos + FVector(0.f, 0.f, 5.f), 18.0f, 16, FColor::Cyan, false, 0.0f, 0, 3.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		DrawDebugCylinder(GetWorld(), LandingPos, LandingPos + FVector(0.f, 0.f, 30.f), 40.0f, 16, FColor::Emerald, false, 0.0f, 0, 2.5f);
	}
	else
	{
		DrawDebugCircle(GetWorld(), LandingPos + FVector(0.f, 0.f, 5.f), 30.0f, 16, FColor(255, 50, 50), false, 0.0f, 0, 3.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}

	if (GEngine && !DebugStateMessage.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(200, 0.05f, DebugMessageColor, DebugStateMessage);
	}
}

void AFishingRod::UpdateMinigame(float DeltaTime)
{
	if (!OwnerCharacter || !SpawnedBobber) return;

	// 1. 물고기 도망 방향 주기적 전환 (3.0 ~ 5.5초 주기: 여유로운 턴)
	FishTurnTimer -= DeltaTime;
	if (FishTurnTimer <= 0.0f)
	{
		FishEscapeDirection *= -1.0f; // 반대 방향으로 선회
		FishTurnTimer = FMath::RandRange(3.0f, 5.5f);
	}

	// 2. 플레이어의 저항 방향이 물고기 반대 방향인지 판별
	// (물고기가 우측(+1)으로 도망갈 때 플레이어가 좌측 A(-1)를 당기면 곱이 음수 -> 올바른 저항)
	const bool bResistingCorrectly = (FishEscapeDirection * PlayerPullInput < -0.1f);

	// ★ 핵심 메커니즘: 물고기 반대 방향(A/D)으로 제압 중일 때만 릴(S)이 전진하여 거리가 좁혀짐!
	// 반대 방향 저항 없이 S만 누르면 릴이 헛돌며 전진하지 못하고 물고기가 줄을 끌고 나가며 텐션만 급상승함.
	const bool bCanPullIn = bResistingCorrectly && (CurrentTension < 99.0f);

	// 3. 릴 감기(S / LMB) 및 장력/거리 연산
	if (bIsReelingInput)
	{
		if (bCanPullIn)
		{
			// 제압 성공: 릴이 물고기를 힘차게 끌어당김
			CurrentDistance -= ReelSpeed * DeltaTime;
			CurrentTension += TensionGainCorrect * DeltaTime;
		}
		else if (!bResistingCorrectly)
		{
			// 제압 실패(A/D 저항 없음/잘못됨): 물고기가 힘으로 버티며 줄을 끌고 나감, 텐션 급상승!
			CurrentDistance += FishPullSpeed * 0.5f * DeltaTime;
			CurrentTension += TensionGainWrong * DeltaTime;
		}
		else
		{
			// 올바른 방향이지만 텐션 99% 도달: 릴이 헛돌며 대기
			CurrentDistance += FishPullSpeed * 0.2f * DeltaTime;
			CurrentTension += TensionGainCorrect * DeltaTime;
		}
	}
	else
	{
		// 릴을 안 감으면 장력 자연 감소 (초당 35% 빠른 해제)
		CurrentTension -= TensionDecayRate * DeltaTime;

		// 릴을 쉬는 동안 물고기가 조금씩 줄을 끌고 도망침
		CurrentDistance += FishPullSpeed * DeltaTime;
	}

	CurrentTension = FMath::Clamp(CurrentTension, 0.0f, 100.0f);

	// 4. 찌에 물고기 도망 방향 및 텐션 상태 피드백 전달 (찌 기울어짐 & 물살 파문)
	SpawnedBobber->SetFishPullFeedback(FishEscapeDirection, CurrentTension / 100.0f, bResistingCorrectly);


	// 보조 디버그 HUD
	if (GEngine)
	{
		const FString FishDirStr = (FishEscapeDirection > 0.0f) ? TEXT("우측 -> [A키 당기기!]") : TEXT("좌측 <- [D키 당기기!]");
		FString MyInputStr = TEXT("중립(저항 없음)");
		if (PlayerPullInput < -0.1f) MyInputStr = TEXT("A키(좌)");
		else if (PlayerPullInput > 0.1f) MyInputStr = TEXT("D키(우)");

		FString ReelStr = TEXT("릴 정지");
		if (bIsReelingInput)
		{
			if (bCanPullIn) ReelStr = TEXT("★ 정상 릴링 중 (전진 O)");
			else if (!bResistingCorrectly) ReelStr = TEXT("▲ 릴 헛돎! (물고기 반대 방향 A/D 먼저 당기세요!)");
			else ReelStr = TEXT("▲ 릴 헛돎! (텐션 과다, 릴 잠시 풀기)");
		}

		GEngine->AddOnScreenDebugMessage(100, 0.1f, bCanPullIn ? FColor::Green : FColor::Orange,
			FString::Printf(TEXT("[미니게임] 거리: %.1fm | 텐션: %.0f%% | 물고기: %s | 내입력: %s | %s"),
				CurrentDistance, CurrentTension, *FishDirStr, *MyInputStr, *ReelStr));
	}

	// 6. 찌의 수평 위치 업데이트 및 최소 접근 거리 한계 제어:
	const float ActualDist2D = FVector::Dist2D(OwnerCharacter->GetActorLocation(), SpawnedBobber->GetActorLocation());
	const FVector ToPlayer = (OwnerCharacter->GetActorLocation() - SpawnedBobber->GetActorLocation()).GetSafeNormal2D();
	const FVector RightVec = FVector::CrossProduct(ToPlayer, FVector::UpVector);

	// (1) 물고기 좌우 흔들기
	SpawnedBobber->AddActorWorldOffset(RightVec * FishEscapeDirection * 150.0f * DeltaTime);

	// (2) 릴 감기 이동 (올바른 A/D 저항으로 제압 중일 때만 플레이어 앞으로 견인)
	if (bIsReelingInput)
	{
		if (bCanPullIn && ActualDist2D > MinCatchDistance)
		{
			// 제압 성공: 플레이어 쪽으로 힘차게 끌려옴
			SpawnedBobber->AddActorWorldOffset(ToPlayer * ReelSpeed * 100.0f * DeltaTime);
		}
		else if (!bResistingCorrectly)
		{
			// 제압 실패: 물고기가 플레이어 바깥쪽으로 줄을 끌고 나감
			SpawnedBobber->AddActorWorldOffset(-ToPlayer * FishPullSpeed * 60.0f * DeltaTime);
		}
	}
	else
	{
		// 릴을 풀었을 때 물고기가 조금씩 바깥쪽으로 끌고 나감
		SpawnedBobber->AddActorWorldOffset(-ToPlayer * FishPullSpeed * 60.0f * DeltaTime);
	}

	// 7. 성공 판정 (물 탈출 또는 플레이어 근접 시 대성공!)
	const bool bBobberEscapedWater = SpawnedBobber->CheckHasEscapedWater();
	const bool bReachedPlayer = (ActualDist2D <= MinCatchDistance || CurrentDistance <= (MinCatchDistance / 100.0f));

	if (bBobberEscapedWater || bReachedPlayer)
	{
		const FString SuccessReason = bBobberEscapedWater ? TEXT("찌가 물을 탈출하여 육지로 랜딩 성공!") : TEXT("물고기를 발앞까지 안전하게 끌어당겼습니다!");
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green,
				FString::Printf(TEXT("[낚시 대성공!] %s (인벤토리에 생고기 획득)"), *SuccessReason));
		}
		UE_LOG(LogTemp, Display, TEXT("[낚시] %s (성공!)"), *SuccessReason);
		FinishFishing(true);
	}
}

void AFishingRod::FinishFishing(bool bSuccess)
{
	if (CurrentState == EFishingState::Idle) return;

	GetWorld()->GetTimerManager().ClearTimer(BiteTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ReactionTimerHandle);

	CurrentState = bSuccess ? EFishingState::ReelingSuccess : EFishingState::Failed;

	// 서버 권한으로 보상 아이템 지급 및 서버 측 세션 종료 요청
	Server_FinishFishing(bSuccess);

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
	// 클라이언트 강제 종료(RPC validation failure) 방지: 항상 true 반환
	return true;
}

void AFishingRod::Server_FinishFishing_Implementation(bool bSuccess)
{
	if (bSuccess)
	{
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

	// 서버 측 낚시 세션 및 상태 정리
	bIsFishingActive = false;
	CurrentState = EFishingState::Idle;
	PlayerPullInput = 0.0f;
	AnimPullInput = 0.0f;
	bIsReelingInput = false;
}

void AFishingRod::ResetFishing()
{
	bIsFishingActive = false; // ★ 낚시 세션 락 해제
	CurrentState = EFishingState::Idle;
	CurrentTension = 0.0f;
	CurrentDistance = 0.0f;
	PlayerPullInput = 0.0f;
	AnimPullInput = 0.0f;
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
