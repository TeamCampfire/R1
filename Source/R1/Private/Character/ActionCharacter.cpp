// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ActionCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/HarvestableComponent.h"

#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"

// Sets default values
AActionCharacter::AActionCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Head메시 생성
	HeadMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetMesh());
	HeadMesh->SetLeaderPoseComponent(GetMesh());

	// Leg메시 생성
	LegMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegMesh"));
	LegMesh->SetupAttachment(GetMesh());
	LegMesh->SetLeaderPoseComponent(GetMesh());
	

	/// 카메라 생성 및 세팅
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight)); // 캡슐 기준 눈높이
	FirstPersonCamera->bUsePawnControlRotation = true;

	bUseControllerRotationYaw = true;	// 캐릭터 몸체(액터) 자체가 좌우로 회전하도록
	GetCharacterMovement()->bOrientRotationToMovement = false; // 이동 방향으로 자동 회전하지 않게 (1인칭은 항상 카메라 보는 방향이 정면)

	// 크라우치 가능 모드로 세팅
	// 빈 프로젝트 시작시 기본 비활성화 / Third Person 탬플릿으로 시작하면 활성화 되어 있음
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);		// Standing: Radius, HalfHeight
	GetCharacterMovement()->SetCrouchedHalfHeight(60.f);		// Crouch 시 목표 HalfHeight

	// 눈높이(카메라) 포지션
	DefaultEyeHeight = BaseEyeHeight;
	CurrentWorldEyeHeight = GetActorLocation().Z + DefaultEyeHeight;
}

// Called when the game starts or when spawned
void AActionCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// PC의 카메라 상하각도 세팅
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->ViewPitchMax = ViewPicthMax;
			PC->PlayerCameraManager->ViewPitchMin = ViewPicthMin;
		}
	}

	// 이동관련 파라미터 세팅
	ApplyMovementSettings();
}

// Called every frame
void AActionCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/// 크라우치모드(true/false)에 따라 카메라 높이 보간
	const float TargetLocalEyeHeight = bIsCrouched ? CrouchedEyeHeight : DefaultEyeHeight;
	const float TargetWorldEyeHeight = GetActorLocation().Z + TargetLocalEyeHeight; // 캡슐이 이미 이동한 뒤 기준

	CurrentWorldEyeHeight = FMath::FInterpTo(CurrentWorldEyeHeight, TargetWorldEyeHeight, DeltaTime, CrouchInterpSpeed);

	//const float LocalOffset = CurrentWorldEyeHeight - GetActorLocation().Z; // 현재 캡슐 위치 기준으로 역산
	//FirstPersonCamera->SetRelativeLocation(FVector(
	//	FirstPersonCamera->GetRelativeLocation().X, 
	//	FirstPersonCamera->GetRelativeLocation().Y,
	//	LocalOffset
	//));
	
}

// Called to bind functionality to input
void AActionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// SetupPlayerInputComponent
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 이동, 회전
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AActionCharacter::OnMoveAction);
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AActionCharacter::OnLookInput);

		// Jump는 눌렀을 때(Started) 시작, 뗐을 때(Completed) 멈춤
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AActionCharacter::OnJumpPressed);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	
		// 스프린트 (Started/Completed는 모드와 무관하게 항상 둘 다 바인딩)
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AActionCharacter::OnSprintPressed);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AActionCharacter::OnSprintReleased);

		// 크라우치 (Started/Completed 둘 다 바인딩)
		EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &AActionCharacter::OnCrouchPressed);
		EIC->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &AActionCharacter::OnCrouchReleased);

		EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &AActionCharacter::OnAttackPressed);
	}
}

void AActionCharacter::SetSprintInputMode(ESprintInputMode NewNode)
{
	SprintInputMode = NewNode;
	bIsSprinting = false;		// 모드 전환 순간 스프린트 상태 꼬이는 것 방지 (Hold 누르고 있던 중 전환 등)
	ApplyMovementSettings();
}

void AActionCharacter::SetCrouchInputMode(ECrouchInputMode NewMode)
{
	CrouchInputMode = NewMode;
	UnCrouch(); // 모드 전환 시 안전하게 초기화 (Hold 누르고 있던 중 전환 등)
	ApplyMovementSettings();
}

void AActionCharacter::ProcessAttack()
{
	//TODO 무기 타입에 따라서 세분화
	if (AActor* Target = DetectdObjectInAttackRange())
	{
		// 자원을 얻을 수 있는 대상인지 확인
		if (UHarvestableComponent* HarvestComt = Target->FindComponentByClass<UHarvestableComponent>())
		{
			// 자원 획득 진행
			FHarvestRes HarvRes =  IHarvestable::Execute_OnHitted(HarvestComt, this);
			if (HarvRes.HarvesResult)
			{
				UE_LOG(LogTemp, Display, TEXT("자원 [%s]를 %d개 획득!"), *(HarvRes.ItemData), HarvRes.Count);
			}
		}
		//TOOD 자원이 얻는 대상이 아니라 공격을 받는 대상
		else if (true)
		{
			UE_LOG(LogTemp, Display, TEXT("TODO 공격을 받는 인터페이스 구현"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("아무도 것도 맞지 않았습니다."));
	}
}


bool AActionCharacter::CanJumpInternal_Implementation() const
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	// 엔진 기본 구현에서 "!bIsCrouched" 체크만 제외하고 재구현.

	// CanAttemptJump()를 그대로 쓰면 크라우치 중엔 항상 막힘.
	// IsJumpAllowed()는 유지하고 크라우치 조건만 제외해서 직접 조합한다.
	bool bCanJump = MoveComp && MoveComp->IsJumpAllowed()
		&& (MoveComp->IsMovingOnGround() || MoveComp->IsFalling());

	if (bCanJump)
	{
		if (JumpCurrentCount == 0 && MoveComp->IsFalling())
		{
			bCanJump = JumpCurrentCount + 1 < JumpMaxCount;
		}
		else
		{
			bCanJump = JumpCurrentCount < JumpMaxCount;
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("CanJumpInternal called, bIsCrouched=%d, bCanJump=%d"), bIsCrouched, bCanJump);

	return bCanJump;
}

void AActionCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	//UE_LOG(LogTemp, Warning, TEXT("OnJumped_Implementation called! UnCrouching now."));
	if (bIsCrouched)
	{
		UnCrouch(); // 점프가 실제로 발동된 뒤에 크라우치를 풀어준다
	}
}

void AActionCharacter::OnMoveAction(const FInputActionValue& InValue)
{
	const FVector2D MoveValue = InValue.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), MoveValue.Y);
	AddMovementInput(GetActorRightVector(), MoveValue.X);

	//UE_LOG(LogTemp, Log, TEXT("OnMoveAction"));
}

void AActionCharacter::OnLookInput(const FInputActionValue& InValue)
{
	const FVector2D LookValue = InValue.Get<FVector2D>();
	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);

	//UE_LOG(LogTemp, Log, TEXT("OnLookInput"));
}

void AActionCharacter::OnSprintPressed()
{
	// 크라우치 모드에는 스프린트 안함
	if (bIsCrouched) return;

	if (SprintInputMode == ESprintInputMode::Toggle)
	{
		bIsSprinting = !bIsSprinting;	// 누를 때만 반전

		//UE_LOG(LogTemp, Log, TEXT("OnSprintPressed  Toggle: %d"), SprintInputMode);
	}
	else  // Hold
	{
		bIsSprinting = true;
		//UE_LOG(LogTemp, Log, TEXT("OnSprintPressed Started"));
	}
	ApplyMovementSettings();
}

void AActionCharacter::OnSprintReleased()
{
	if (SprintInputMode == ESprintInputMode::Hold)
	{
		bIsSprinting = false;
		ApplyMovementSettings();
		//UE_LOG(LogTemp, Log, TEXT("OnSprintPressed Released"));
	}
	// Toggle 모드에서는 뗄 때 아무것도 안 함
}

void AActionCharacter::OnCrouchPressed()
{
	if (CrouchInputMode == ECrouchInputMode::Toggle)
	{
		if (bIsCrouched)
		{
			UnCrouch();
			//UE_LOG(LogTemp, Log, TEXT("UnCrouch"));
		}
		else
		{
			bIsSprinting = false; // 상호배타 규칙 유지
			Crouch();
			//UE_LOG(LogTemp, Log, TEXT("Crouch"));
		}
	}
	else // Hold
	{
		bIsSprinting = false;
		Crouch();
		//UE_LOG(LogTemp, Log, TEXT("Crouch Hold"));
	}
	ApplyMovementSettings();
}

void AActionCharacter::OnCrouchReleased()
{
	if (CrouchInputMode == ECrouchInputMode::Hold)
	{
		UnCrouch();
		ApplyMovementSettings();
	}
	// Toggle 모드에서는 뗄 때 아무것도 안 함
}

void AActionCharacter::OnJumpPressed()
{
	Jump();
}

void AActionCharacter::OnAttackPressed()
{
	UE_LOG(LogTemp, Display, TEXT("AttackPressed"));
	if (!AM_Attack)
	{
		UE_LOG(LogTemp, Display, TEXT("AM_Attack was nullptr"));
		return;
	}

	if (UAnimInstance* Instance = GetMesh()->GetAnimInstance())
	{
		if (!Instance->IsAnyMontagePlaying())
			PlayAnimMontage(AM_Attack);
	}
}


void AActionCharacter::ApplyMovementSettings()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// 이동속도 세팅
		MoveComp->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
		MoveComp->MaxWalkSpeedCrouched = CrouchSpeed;

		// 점프파워 세팅
		MoveComp->JumpZVelocity = JumpPower;
	}
}

AActor* AActionCharacter::DetectdObjectInAttackRange()
{
	AActor* Res = nullptr;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (APlayerCameraManager* CameraManger = PC->PlayerCameraManager)
		{
			// 카메라의 위치에서 사정거리만큼 line trace
			FHitResult HitRes;
			FVector StartPos = CameraManger->GetCameraLocation();
			FVector EndPos = StartPos + CameraManger->GetActorForwardVector() * AttackRange;
			if (GetWorld()->LineTraceSingleByChannel(HitRes, StartPos, EndPos, ECC_Visibility))
			{
				// Hit된 Actor을 Res에 세팅
				Res = HitRes.GetActor();
				UE_LOG(LogTemp, Display, TEXT("%s"), *Res->GetFName().ToString());
			}
		}
	}
	return Res;
}

