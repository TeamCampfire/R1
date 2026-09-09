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

	static ConstructorHelpers::FObjectFinder<UItemDataBase> RawMeatFinder(TEXT("/Game/Data/Item/ConsumableItem/DA_Item_Consumable_RawMeat.DA_Item_Consumable_RawMeat"));
	if (RawMeatFinder.Succeeded())
	{
		FishRewardItemData = RawMeatFinder.Object;
	}
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
	                    NewState == EFishingState::Minigame ||
	                    NewState == EFishingState::ReelingSuccess);

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
	                    NewState == EFishingState::Minigame ||
	                    NewState == EFishingState::ReelingSuccess);

	if (NewState == EFishingState::Biting)
	{
		if (SpawnedBobber && IsValid(SpawnedBobber))
		{
			SpawnedBobber->SetBiting(true);
		}
	}
	else if (NewState == EFishingState::Idle)
	{
		if (OwnerCharacter && !OwnerCharacter->IsLocallyControlled())
		{
			ResetFishing();
		}
	}
}

void AFishingRod::Server_SetPull_Implementation(float PullAxis)
{
	const float ClampedAxis = FMath::Clamp(PullAxis, -1.0f, 1.0f);
	PlayerPullInput = (FMath::Abs(ClampedAxis) < 0.05f) ? 0.0f : ClampedAxis;
	if (PlayerPullInput == 0.0f)
	{
		AnimPullInput = 0.0f;
	}
}

void AFishingRod::Server_SetReeling_Implementation(bool bReeling)
{
	bIsReelingInput = bReeling;
}

void AFishingRod::OnRep_CurrentState()
{
	bIsFishingActive = (CurrentState == EFishingState::Casting ||
	                    CurrentState == EFishingState::WaitingBite ||
	                    CurrentState == EFishingState::Biting ||
	                    CurrentState == EFishingState::Minigame ||
	                    CurrentState == EFishingState::ReelingSuccess);

	if (CurrentState == EFishingState::Biting)
	{
		if (SpawnedBobber && IsValid(SpawnedBobber))
		{
			SpawnedBobber->SetBiting(true);
		}
	}
	else if (CurrentState == EFishingState::ReelingSuccess || CurrentState == EFishingState::Failed)
	{
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
	}
	else if (CurrentState == EFishingState::Idle)
	{
		if (OwnerCharacter && !OwnerCharacter->IsLocallyControlled())
		{
			ResetFishing();
		}
	}
}

FVector AFishingRod::GetRodTipLocation() const
{
	if (!CustomRodTipOffset.IsZero())
	{
		return GetActorTransform().TransformPosition(CustomRodTipOffset);
	}

	// 로컬 1인칭 화면에서는 ItemMesh1P의 소켓 우선 사용
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		if (ItemMesh1P && ItemMesh1P->DoesSocketExist(RodTipSocketName))
		{
			return ItemMesh1P->GetSocketLocation(RodTipSocketName);
		}
	}
	else
	{
		// 3인칭(리슨 서버 / 관전자) 화면에서는 ItemMesh3P의 소켓 사용!
		if (ItemMesh3P && ItemMesh3P->DoesSocketExist(RodTipSocketName))
		{
			return ItemMesh3P->GetSocketLocation(RodTipSocketName);
		}
	}

	// 3인칭 우선 폴백 (3P 관전자 환경 대비)
	if (ItemMesh3P && ItemMesh3P->DoesSocketExist(RodTipSocketName))
	{
		return ItemMesh3P->GetSocketLocation(RodTipSocketName);
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

	// 1. 릴링 여부에 따른 애니메이션용 좌우 기울기(Pull) 감쇄 및 부드러운 보간 (로컬 입력자 및 서버에서만 연산, 관전자 클라이언트는 Replicated 수신)
	if ((OwnerCharacter && OwnerCharacter->IsLocallyControlled()) || HasAuthority())
	{
		if (FMath::IsNearlyZero(PlayerPullInput, 0.05f))
		{
			AnimPullInput = 0.0f;
		}
		else
		{
			const float TargetAnimPull = IsReelingInput() ? (PlayerPullInput * ReelingTiltDamping) : PlayerPullInput;
			AnimPullInput = FMath::FInterpTo(AnimPullInput, TargetAnimPull, DeltaTime, TiltInterpSpeed);
		}
	}

	// 2. 낚싯줄 케이블 끝점 실시간 동기화 (로컬 플레이어, 호스트, 관전자 모든 머신에서 실행!)
	if (FishingLineCable && FishingLineCable->IsVisible())
	{
		// 로컬 1인칭 시점이면 ItemMesh1P, 3인칭(관전자/리슨 서버) 시점이면 ItemMesh3P에 확실하게 부착
		USceneComponent* TargetMesh = nullptr;
		if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
		{
			TargetMesh = (ItemMesh1P && ItemMesh1P->DoesSocketExist(RodTipSocketName)) ? ItemMesh1P : ItemMesh3P;
		}
		else
		{
			TargetMesh = (ItemMesh3P && ItemMesh3P->DoesSocketExist(RodTipSocketName)) ? ItemMesh3P : ItemMesh1P;
		}

		if (TargetMesh && (FishingLineCable->GetAttachParent() != TargetMesh || FishingLineCable->GetAttachSocketName() != RodTipSocketName))
		{
			FishingLineCable->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			FishingLineCable->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, RodTipSocketName);
			FishingLineCable->SetRelativeLocation(FVector::ZeroVector);
		}

		const FVector RodTipLoc = GetRodTipLocation();
		FishingLineCable->SetWorldLocation(RodTipLoc);

		if (SpawnedBobber && IsValid(SpawnedBobber))
		{
			const FVector BobberLoc = SpawnedBobber->GetActorLocation();
			const float LineDist = FVector::Dist(RodTipLoc, BobberLoc);
			FishingLineCable->CableLength = LineDist;
			FishingLineCable->EndLocation = FVector(0.f, 0.f, 5.0f);
		}
	}

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

	// 3. 조준 중일 때 캐스팅 궤적 실시간 프리뷰 렌더링 (내 화면에만 렌더링)
	if (CurrentState == EFishingState::Aiming)
	{
		UpdateCastingTrajectory(DeltaTime);
	}
	// 4. 미니게임 진행 중일 때 장력 및 물고기 저항 틱 연산 (내 로컬 세션에서만 연산)
	else if (CurrentState == EFishingState::Minigame)
	{
		UpdateMinigame(DeltaTime);

		// 5. 미니게임 진행 중 찌 위치를 20 FPS(0.05초)로 서버 및 관전자에게 동기화 전송
		if (SpawnedBobber && IsValid(SpawnedBobber))
		{
			BobberSyncTimer += DeltaTime;
			if (BobberSyncTimer >= 0.05f)
			{
				BobberSyncTimer = 0.0f;
				if (!HasAuthority())
				{
					Server_UpdateBobberLocation(SpawnedBobber->GetActorLocation());
				}
				else
				{
					Multicast_UpdateBobberLocation(SpawnedBobber->GetActorLocation());
				}
			}
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

		/*
		if (GEngine && OwnerCharacter && OwnerCharacter->IsLocallyControlled())
		{
			GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Cyan, TEXT("[낚시] 우클릭 조준 시작: 착수 지점을 확인하세요. (좌클릭으로 투척)"));
		}
		*/
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
			/*
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("[낚시] 물(수면)이 아닌 곳에는 캐스팅할 수 없습니다!"));
			}
			*/
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

			// 낚싯줄 연결 및 끝점 위치 정밀 동기화 (1P는 ItemMesh1P, 3P는 ItemMesh3P)
			USceneComponent* TargetMesh = nullptr;
			if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
			{
				TargetMesh = (ItemMesh1P && ItemMesh1P->DoesSocketExist(RodTipSocketName)) ? ItemMesh1P : ItemMesh3P;
			}
			else
			{
				TargetMesh = (ItemMesh3P && ItemMesh3P->DoesSocketExist(RodTipSocketName)) ? ItemMesh3P : ItemMesh1P;
			}

			if (TargetMesh)
			{
				FishingLineCable->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				FishingLineCable->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, RodTipSocketName);
				FishingLineCable->SetRelativeLocation(FVector::ZeroVector);
			}
			FishingLineCable->SetWorldLocation(TipLoc);

			FishingLineCable->EndLocation = FVector(0.f, 0.f, 5.0f);
			FishingLineCable->CableLength = 20.0f;
			FishingLineCable->SetAttachEndToComponent(SpawnedBobber->GetRootComponent());
			FishingLineCable->SetVisibility(true);

			SetFishingState(EFishingState::Casting);
			PushFishingInputContext(); // ★ 캐스팅 중에도 IMC_Fishing 유지

			// 원격 머신(리슨 서버 / 관전자)에 찌 비행 및 케이블 생성 동기화 요청
			if (!HasAuthority())
			{
				Server_StartCast(TipLoc, TargetLoc, LaunchVelocity);
			}
			else
			{
				Multicast_StartCast(TipLoc, TargetLoc, LaunchVelocity);
			}

			/*
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, TEXT("[낚시] 찌 투척 완료! 호수 수면으로 날아갑니다."));
			}
			*/
			UE_LOG(LogTemp, Display, TEXT("[낚시] 찌를 성공적으로 던졌습니다!"));
		}
	}
	// 2. 대기 중(입질 전) 좌클릭 -> 헛챔질! (너무 일찍 챔질하여 실패 종료)
	else if (CurrentState == EFishingState::WaitingBite)
	{
		/*
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("[낚시 실패] 헛챔질! 너무 일찍 챔질하여 물고기가 도망갔습니다."));
		}
		*/
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
		CurrentLateralOffset = FishEscapeDirection * 160.0f; // 시작부터 좌 또는 우로 1.6m 급발진 이탈!
		FishTurnTimer = FMath::RandRange(2.0f, 3.5f);
		PlayerPullInput = 0.0f;
		AnimPullInput = 0.0f;
		bIsReelingInput = false;
		bIsReelingByLMB = false;

		// 찌를 초기 이탈 위치로 즉각 이동
		if (OwnerCharacter && SpawnedBobber)
		{
			const FVector ToPlayer = (OwnerCharacter->GetActorLocation() - SpawnedBobber->GetActorLocation()).GetSafeNormal2D();
			const FVector RightVec = FVector::CrossProduct(ToPlayer, FVector::UpVector);
			SpawnedBobber->AddActorWorldOffset(RightVec * CurrentLateralOffset);
		}

		SetFishingState(EFishingState::Minigame);
		PushFishingInputContext(); // ★ 미니게임 시작 시 IMC_Fishing 확실하게 유지

		/*
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Emerald, TEXT("[낚시] ★ 챔질 대성공! 미니게임 시작!\n[조작법] A/D: 물고기를 먼저 중앙으로 정렬 | S 또는 좌클릭: 중앙 정렬 시에만 릴 감기 (미정렬 시 줄 끊어짐) | Space: 취소"));
		}
		*/
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
	const float FinalAxis = (FMath::Abs(ClampedAxis) < 0.05f) ? 0.0f : ClampedAxis;

	if (!FMath::IsNearlyEqual(PlayerPullInput, FinalAxis, 0.01f))
	{
		PlayerPullInput = FinalAxis;
		if (PlayerPullInput == 0.0f)
		{
			AnimPullInput = 0.0f;
		}
		if (!HasAuthority())
		{
			Server_SetPull(PlayerPullInput);
		}
	}
}

void AFishingRod::Input_Cancel()
{
	if (CurrentState != EFishingState::Idle && CurrentState != EFishingState::ReelingSuccess)
	{
		/*
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("[낚시] 낚시를 취소했습니다."));
		}
		*/
		FinishFishing(false);
	}
}

void AFishingRod::OnBobberLandedInWater()
{
	if (FishingLineCable && SpawnedBobber)
	{
		const float LineDist = FVector::Dist(GetRodTipLocation(), SpawnedBobber->GetActorLocation());
		FishingLineCable->CableLength = LineDist * 1.005f;
		FishingLineCable->EndLocation = FVector(0.f, 0.f, 5.0f);
	}

	// 원격 머신(리슨 서버 / 관전자)은 타이머를 돌리지 않고 클라이언트 동기화를 따름
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled()) return;

	if (CurrentState != EFishingState::Casting) return;

	SetFishingState(EFishingState::WaitingBite);

	// 랜덤 입질 대기 타이머 시작 (3 ~ 7초)
	const float RandomWaitTime = FMath::RandRange(MinBiteWaitTime, MaxBiteWaitTime);
	GetWorld()->GetTimerManager().SetTimer(BiteTimerHandle, this, &AFishingRod::TriggerBite, RandomWaitTime, false);

	/*
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("[낚시] 찌가 수면에 안착했습니다. 물고기 입질을 기다립니다..."));
	}
	*/
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

	/*
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("★ [입질 발생!] 물고기가 물었습니다! 지금 좌클릭으로 챔질하세요!"));
	}
	*/
	UE_LOG(LogTemp, Display, TEXT("[낚시] 입질 발생! 챔질 유효 시간: %.1f초"), BiteReactionWindow);

	// 반응 제한 시간 초과 타이머 (2초 내에 안 누르면 놓침)
	GetWorld()->GetTimerManager().SetTimer(ReactionTimerHandle, this, &AFishingRod::OnBiteMissed, BiteReactionWindow, false);
}

void AFishingRod::OnBiteMissed()
{
	if (CurrentState == EFishingState::Biting)
	{
		/*
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("[낚시 실패] 챔질 타이밍을 놓쳤습니다! 물고기가 미끼를 물고 도망갔습니다."));
		}
		*/
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

	/*
	if (GEngine && !DebugStateMessage.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(200, 0.05f, DebugMessageColor, DebugStateMessage);
	}
	*/
}

void AFishingRod::UpdateMinigame(float DeltaTime)
{
	if (!OwnerCharacter || !SpawnedBobber) return;

	// 1. 플레이어와 찌 간의 기하학적 기준축 계산
	const FVector PlayerLoc = OwnerCharacter->GetActorLocation();
	const FVector BobberLoc = SpawnedBobber->GetActorLocation();
	const float ActualDist2D = FVector::Dist2D(PlayerLoc, BobberLoc);
	const FVector ToPlayer = (PlayerLoc - BobberLoc).GetSafeNormal2D();
	const FVector RightVec = FVector::CrossProduct(ToPlayer, FVector::UpVector); // 플레이어가 찌를 바라보는 기준 우측(+) 방향

	// 2. 물고기 도망 방향 주기적 전환 및 경계 반사
	FishTurnTimer -= DeltaTime;
	if (FishTurnTimer <= 0.0f)
	{
		FishEscapeDirection *= -1.0f; // 반대 방향으로 선회
		FishTurnTimer = FMath::RandRange(2.5f, 4.5f);
	}

	// 최대 편차 경계 도달 시 반대 방향으로 선회
	if (CurrentLateralOffset >= MaxLateralOffset && FishEscapeDirection > 0.0f)
	{
		FishEscapeDirection = -1.0f;
		FishTurnTimer = FMath::RandRange(2.0f, 3.5f);
	}
	else if (CurrentLateralOffset <= -MaxLateralOffset && FishEscapeDirection < 0.0f)
	{
		FishEscapeDirection = 1.0f;
		FishTurnTimer = FMath::RandRange(2.0f, 3.5f);
	}

	// 3. 실제 물리적 수평 이동 연산 (물고기 헤엄 + 플레이어 A/D 당기기)
	// FishEscapeSpeed: 물고기가 헤엄쳐 도망치는 속도 (우측 + / 좌측 -)
	// PlayerPullPower: 플레이어가 A(좌:-1.0) 또는 D(우:+1.0)로 힘겹게 끌어당기는 견인력
	const float FishLateralVelocity = FishEscapeDirection * FishEscapeSpeed;
	const float PlayerLateralVelocity = PlayerPullInput * PlayerPullPower;
	const float NetLateralVelocity = FishLateralVelocity + PlayerLateralVelocity;

	const float LateralDelta = NetLateralVelocity * DeltaTime;
	CurrentLateralOffset = FMath::Clamp(CurrentLateralOffset + LateralDelta, -MaxLateralOffset, MaxLateralOffset);

	// 찌를 월드 상에서 좌우로 실제 이동!
	SpawnedBobber->AddActorWorldOffset(RightVec * LateralDelta);

	// 4. 중심 정렬도(CenterFactor) 계산 (중심 = 1.0, 최대 이탈 = 0.0)
	const float LateralDeviation = FMath::Abs(CurrentLateralOffset);
	const float NormalizedOffset = FMath::Clamp(LateralDeviation / MaxLateralOffset, 0.0f, 1.0f);
	const float CenterFactor = 1.0f - NormalizedOffset;

	// 중앙 정렬 판정: 편차가 25% 이내 (약 ±85cm 이내)
	const bool bIsCentered = (NormalizedOffset <= 0.25f);

	// 5. 릴 감기(S / LMB) 및 장력/거리 연산
	if (bIsReelingInput)
	{
		if (bIsCentered)
		{
			// ★ 중앙 정렬 성공: 릴이 전속력으로 물고기를 힘차게 끌어당김!
			CurrentDistance -= ReelSpeed * DeltaTime;

			if (ActualDist2D > MinCatchDistance)
			{
				SpawnedBobber->AddActorWorldOffset(ToPlayer * ReelSpeed * 100.0f * DeltaTime);
			}

			// 안정적 릴링 장력 증가 (초당 8%)
			CurrentTension += TensionGainCorrect * DeltaTime;
		}
		else
		{
			// ★ 제압 실패(중앙 이탈 상태에서 억지로 릴링):
			// 릴이 전혀 전진하지 못하고(전진 속도 0), 오히려 물고기가 줄을 비틀며 바깥으로 끌고 나감!
			const float PullBack = FishPullSpeed * 0.6f * DeltaTime;
			CurrentDistance += PullBack;
			SpawnedBobber->AddActorWorldOffset(-ToPlayer * PullBack * 100.0f);

			// 장력 급상승 (초당 48% -> 약 1.8초 만에 100% 도달하여 줄 끊어짐!)
			CurrentTension += TensionGainWrong * DeltaTime;
		}
	}
	else
	{
		// 릴을 풀었을 때 장력 자연 감소 (초당 35%)
		CurrentTension -= TensionDecayRate * DeltaTime;

		// 릴을 쉬는 동안 물고기가 바깥쪽으로 슬금슬금 물러남
		const float PushBackSpeed = FishPullSpeed * 0.4f;
		CurrentDistance += PushBackSpeed * DeltaTime;
		SpawnedBobber->AddActorWorldOffset(-ToPlayer * PushBackSpeed * 50.0f * DeltaTime);
	}

	CurrentTension = FMath::Clamp(CurrentTension, 0.0f, 100.0f);

	// ★ 핵심: 장력 100% 도달 시 낚싯줄 끊어짐 (실패!)
	if (CurrentTension >= 100.0f)
	{
		/*
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("[낚시 실패] 낚싯줄이 끊어졌습니다! (장력 100% 초과)"));
		}
		*/
		UE_LOG(LogTemp, Warning, TEXT("[낚시] 장력 100%% 초과로 낚싯줄이 끊어져 실패했습니다."));
		FinishFishing(false);
		return;
	}

	// 6. 찌에 좌우 편차 및 장력 피드백 전달
	const float DirectionFeedback = CurrentLateralOffset / MaxLateralOffset; // -1.0 ~ +1.0
	SpawnedBobber->SetFishPullFeedback(DirectionFeedback, CurrentTension / 100.0f, bIsCentered);

	// 7. 실시간 HUD 메시지 (중앙 정렬 게이지 및 안내)
	/*
	if (GEngine)
	{
		FString AlignStatus;
		FColor StatusColor;

		if (bIsCentered)
		{
			AlignStatus = TEXT("★ [중앙 정렬!] S키(릴)를 눌러 물고기를 끌어당기세요!");
			StatusColor = (CurrentTension > 75.0f) ? FColor(255, 120, 0) : FColor::Green;
		}
		else if (CurrentLateralOffset > 0.0f)
		{
			AlignStatus = TEXT("▶ [우측 이탈!] A키(좌)를 눌러 물고기를 먼저 중앙으로 정렬하세요!");
			StatusColor = FColor(255, 140, 0);
		}
		else
		{
			AlignStatus = TEXT("◀ [좌측 이탈!] D키(우)를 눌러 물고기를 먼저 중앙으로 정렬하세요!");
			StatusColor = FColor(255, 140, 0);
		}

		FString ReelStatus = TEXT("릴 정지");
		if (bIsReelingInput)
		{
			if (bIsCentered)
			{
				ReelStatus = TEXT("★ 전속력 릴링 중 (전진 O)");
			}
			else
			{
				ReelStatus = TEXT("▲ 릴 헛돎! 줄 끌려나감! (위험: 릴 즉시 떼고 A/D 정렬!)");
				StatusColor = FColor::Red;
			}
		}

		// 게이지 시각화: [- - - | O | - - -]
		const int32 GaugeIndex = FMath::Clamp(FMath::RoundToInt((CurrentLateralOffset / MaxLateralOffset) * 5.0f), -5, 5);
		FString GaugeStr = TEXT("[");
		for (int32 i = -5; i <= 5; ++i)
		{
			if (i == GaugeIndex) GaugeStr += TEXT("●");
			else if (i == 0) GaugeStr += TEXT("|");
			else GaugeStr += TEXT("-");
		}
		GaugeStr += TEXT("]");

		GEngine->AddOnScreenDebugMessage(100, 0.1f, StatusColor,
			FString::Printf(TEXT("[미니게임] 거리: %.1fm | 장력: %.0f%% | 정렬: %s\n%s | %s"),
				CurrentDistance, CurrentTension, *GaugeStr, *AlignStatus, *ReelStatus));
	}
	*/

	// 7. 성공 판정 (물고기를 충분히 발앞/육지 근처까지 끌어왔을 때만 랜딩 인정)
	const bool bBobberEscapedWater = SpawnedBobber->CheckHasEscapedWater();
	const bool bReachedPlayer = (ActualDist2D <= MinCatchDistance || CurrentDistance <= (MinCatchDistance / 100.0f));
	const bool bReeledCloseEnough = (CurrentDistance <= 3.5f || ActualDist2D <= MinCatchDistance + 150.0f);

	if (bReeledCloseEnough && (bBobberEscapedWater || bReachedPlayer))
	{
		const FString SuccessReason = bBobberEscapedWater ? TEXT("찌가 물을 탈출하여 육지로 랜딩 성공!") : TEXT("물고기를 발앞까지 안전하게 끌어당겼습니다!");
		const FString RewardItemName = FishRewardItemData ? FishRewardItemData->DisplayName.ToString() : TEXT("물고기");

		/*
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green,
				FString::Printf(TEXT("[낚시 대성공!] %s (인벤토리에 [%s] %d개 획득)"), *SuccessReason, *RewardItemName, FishRewardCount));
		}
		*/
		UE_LOG(LogTemp, Display, TEXT("[낚시] %s (보상: [%s] x%d 획득 성공!)"), *SuccessReason, *RewardItemName, FishRewardCount);
		FinishFishing(true);
		return;
	}
}

void AFishingRod::FinishFishing(bool bSuccess, UItemDataBase* OptionalRewardItem, int32 OptionalRewardCount)
{
	if (CurrentState == EFishingState::Idle) return;

	if (OptionalRewardItem)
	{
		FishRewardItemData = OptionalRewardItem;
		FishRewardCount = FMath::Max(1, OptionalRewardCount);
	}

	GetWorld()->GetTimerManager().ClearTimer(BiteTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ReactionTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(FinishCooldownTimerHandle);

	// 낚싯줄 및 찌 즉시 정리 (세레머니 도중 케이블이 허공에 늘어지는 현상 방지)
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

	// 미니게임 입력 및 잔여 편차 즉시 리셋
	CurrentLateralOffset = 0.0f;
	PlayerPullInput = 0.0f;
	AnimPullInput = 0.0f;
	bIsReelingInput = false;
	bIsReelingByLMB = false;

	const EFishingState EndState = bSuccess ? EFishingState::ReelingSuccess : EFishingState::Failed;
	SetFishingState(EndState);

	// 미니게임 키매핑 해제 (캐릭터 이동 락은 bIsFishingActive 및 bIsFinishCooldown으로 유지)
	PopFishingInputContext();

	bIsFinishCooldown = true;

	// 서버 권한으로 보상 아이템 지급 및 세션 동기화 요청
	Server_FinishFishing(bSuccess, FishRewardItemData, FishRewardCount);

	// 세레머니 또는 실패 쿨다운 타이머 시작 (만료 시 OnFinishCeremonyEnded에서 ResetFishing 호출)
	const float Duration = bSuccess ? SuccessCeremonyDuration : FailureDuration;
	GetWorld()->GetTimerManager().SetTimer(
		FinishCooldownTimerHandle,
		this,
		&AFishingRod::OnFinishCeremonyEnded,
		Duration,
		false
	);
}

void AFishingRod::OnFinishCeremonyEnded()
{
	bIsFinishCooldown = false;
	ResetFishing();
	UE_LOG(LogTemp, Display, TEXT("[낚시] 세레머니 및 쿨다운 종료. 이제 이동이 가능합니다."));
}

bool AFishingRod::Server_FinishFishing_Validate(bool bSuccess, UItemDataBase* InRewardItem, int32 InCount)
{
	// 클라이언트 강제 종료(RPC validation failure) 방지: 항상 true 반환
	return true;
}

void AFishingRod::Server_FinishFishing_Implementation(bool bSuccess, UItemDataBase* InRewardItem, int32 InCount)
{
	if (bSuccess)
	{
		UItemDataBase* TargetRewardItem = InRewardItem ? InRewardItem : FishRewardItemData.Get();
		const int32 TargetCount = (InCount > 0) ? InCount : FishRewardCount;

		// 서버 권한으로 캐릭터 인벤토리에 보상 아이템 지급
		if (OwnerCharacter && TargetRewardItem)
		{
			if (UInventoryComponent* InvComp = OwnerCharacter->FindComponentByClass<UInventoryComponent>())
			{
				int32 RemainCount = 0;
				const bool bAdded = InvComp->AddItem(TargetRewardItem, TargetCount, RemainCount);
				if (bAdded)
				{
					UE_LOG(LogTemp, Display, TEXT("[낚시 서버] 인벤토리에 보상 [%s] x%d 지급 완료!"), *TargetRewardItem->DisplayName.ToString(), TargetCount);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[낚시 서버] 인벤토리 공간 부족으로 보상 [%s] x%d 지급 실패!"), *TargetRewardItem->DisplayName.ToString(), TargetCount);
				}
			}
		}
	}

	// 서버 측에서도 줄 및 찌 정리
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

	const EFishingState EndState = bSuccess ? EFishingState::ReelingSuccess : EFishingState::Failed;
	CurrentState = EndState;
	bIsFishingActive = bSuccess;
	CurrentLateralOffset = 0.0f;
	PlayerPullInput = 0.0f;
	AnimPullInput = 0.0f;
	bIsReelingInput = false;

	// 관전자들에게 세레머니/실패 연출 동기화 전파
	Multicast_PlayEndState(bSuccess);

	// 서버 측 세레머니 만료 타이머 등록
	const float Duration = bSuccess ? SuccessCeremonyDuration : FailureDuration;
	GetWorld()->GetTimerManager().SetTimer(
		FinishCooldownTimerHandle,
		this,
		&AFishingRod::OnFinishCeremonyEnded,
		Duration,
		false
	);
}

void AFishingRod::StartCast_Visuals(const FVector& TipLoc, const FVector& TargetLoc, const FVector& LaunchVelocity)
{
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled()) return;
	if (!BobberClass || !GetWorld()) return;

	if (SpawnedBobber && IsValid(SpawnedBobber))
	{
		SpawnedBobber->Destroy();
		SpawnedBobber = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector RodTip = GetRodTipLocation();
	SpawnedBobber = GetWorld()->SpawnActor<AFishingBobber>(BobberClass, RodTip, FRotator::ZeroRotator, SpawnParams);
	if (SpawnedBobber)
	{
		SpawnedBobber->SetOwnerRod(this);
		SpawnedBobber->SetExpectedWaterZ(TargetLoc.Z);
		SpawnedBobber->LaunchBobber(LaunchVelocity, TargetLoc.Z);

		if (FishingLineCable)
		{
			USceneComponent* TargetMesh = nullptr;
			if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
			{
				TargetMesh = (ItemMesh1P && ItemMesh1P->DoesSocketExist(RodTipSocketName)) ? ItemMesh1P : ItemMesh3P;
			}
			else
			{
				TargetMesh = (ItemMesh3P && ItemMesh3P->DoesSocketExist(RodTipSocketName)) ? ItemMesh3P : ItemMesh1P;
			}

			if (TargetMesh)
			{
				FishingLineCable->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				FishingLineCable->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, RodTipSocketName);
				FishingLineCable->SetRelativeLocation(FVector::ZeroVector);
			}
			FishingLineCable->SetWorldLocation(RodTip);

			FishingLineCable->EndLocation = FVector(0.f, 0.f, 5.0f);
			FishingLineCable->CableLength = 20.0f;
			FishingLineCable->SetAttachEndToComponent(SpawnedBobber->GetRootComponent());
			FishingLineCable->SetVisibility(true);
		}
	}
}

void AFishingRod::Server_StartCast_Implementation(const FVector& TipLoc, const FVector& TargetLoc, const FVector& LaunchVelocity)
{
	Multicast_StartCast(TipLoc, TargetLoc, LaunchVelocity);
}

void AFishingRod::Multicast_StartCast_Implementation(const FVector& TipLoc, const FVector& TargetLoc, const FVector& LaunchVelocity)
{
	StartCast_Visuals(TipLoc, TargetLoc, LaunchVelocity);
}

void AFishingRod::Server_UpdateBobberLocation_Implementation(const FVector_NetQuantize& InLocation)
{
	if (SpawnedBobber && IsValid(SpawnedBobber) && (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled()))
	{
		SpawnedBobber->SetActorLocation(InLocation);
	}
	Multicast_UpdateBobberLocation(InLocation);
}

void AFishingRod::Multicast_UpdateBobberLocation_Implementation(const FVector_NetQuantize& InLocation)
{
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled()) return;
	if (HasAuthority()) return;

	if (SpawnedBobber && IsValid(SpawnedBobber))
	{
		SpawnedBobber->SetActorLocation(InLocation);
	}
}

void AFishingRod::Multicast_PlayEndState_Implementation(bool bSuccess)
{
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled()) return;
	if (HasAuthority()) return;

	const EFishingState EndState = bSuccess ? EFishingState::ReelingSuccess : EFishingState::Failed;
	CurrentState = EndState;
	bIsFishingActive = bSuccess;

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

	CurrentLateralOffset = 0.0f;
	PlayerPullInput = 0.0f;
	AnimPullInput = 0.0f;
	bIsReelingInput = false;

	const float Duration = bSuccess ? SuccessCeremonyDuration : FailureDuration;
	GetWorld()->GetTimerManager().SetTimer(
		FinishCooldownTimerHandle,
		this,
		&AFishingRod::OnFinishCeremonyEnded,
		Duration,
		false
	);
}

void AFishingRod::Multicast_EndFishing_Implementation()
{
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled()) return;
	if (HasAuthority()) return;
	ResetFishing();
}

void AFishingRod::ResetFishing()
{
	bIsFishingActive = false; // ★ 낚시 세션 락 해제
	bIsFinishCooldown = false;
	CurrentState = EFishingState::Idle;
	CurrentTension = 0.0f;
	CurrentDistance = 0.0f;
	CurrentLateralOffset = 0.0f;
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

	if (HasAuthority())
	{
		Multicast_EndFishing();
	}
}
