// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ActionCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

#include "Component/StatComponent.h"	
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"

// Sets default values
AActionCharacter::AActionCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

	// 눈높이(카메라) 포지션
	DefaultEyeHeight = BaseEyeHeight;
	CurrentWorldEyeHeight = GetActorLocation().Z + DefaultEyeHeight;

	// 스탯 컴포넌트 세팅
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
}

// Called when the game starts or when spawned
void AActionCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->ViewPitchMax = ViewPicthMax;
			PC->PlayerCameraManager->ViewPitchMin = ViewPicthMin;
		}
	}
	if (StatComponent)
	{
		StatComponent->InitializeStat();
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

	const float LocalOffset = CurrentWorldEyeHeight - GetActorLocation().Z; // 현재 캡슐 위치 기준으로 역산
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, LocalOffset));
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
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ACharacter::Jump);		// 부모클래스의 함수로 바인딩
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	
		// 스프린트 (Started/Completed는 모드와 무관하게 항상 둘 다 바인딩)
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AActionCharacter::OnSprintPressed);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AActionCharacter::OnSprintReleased);

		// 크라우치 (Started/Completed 둘 다 바인딩)
		EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &AActionCharacter::OnCrouchPressed);
		EIC->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &AActionCharacter::OnCrouchReleased);
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

UStatComponent* AActionCharacter::GetStatComponent() const
{
	return StatComponent;
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

		UE_LOG(LogTemp, Log, TEXT("OnSprintPressed  Toggle: %d"), SprintInputMode);
	}
	else  // Hold
	{
		bIsSprinting = true;
		UE_LOG(LogTemp, Log, TEXT("OnSprintPressed Started"));
	}
	ApplyMovementSettings();
}

void AActionCharacter::OnSprintReleased()
{
	if (SprintInputMode == ESprintInputMode::Hold)
	{
		bIsSprinting = false;
		ApplyMovementSettings();
		UE_LOG(LogTemp, Log, TEXT("OnSprintPressed Released"));
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
			UE_LOG(LogTemp, Log, TEXT("UnCrouch"));
		}
		else
		{
			bIsSprinting = false; // 상호배타 규칙 유지
			Crouch();
			UE_LOG(LogTemp, Log, TEXT("Crouch"));
		}
	}
	else // Hold
	{
		bIsSprinting = false;
		Crouch();
		UE_LOG(LogTemp, Log, TEXT("Crouch Hold"));
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

