

#include "Vehicle/Horse.h"
#include "Vehicle/HorseMovementComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Character/ActionCharacter.h"
#include "Character/ActionPlayerController.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AHorse::AHorse(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer.SetDefaultSubobjectClass<UHorseMovementComponent>
	(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Replicate 설정
	bReplicates = true;
	SetReplicateMovement(true);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	// Mesh
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(ECC_Pawn);

	// Camera
	HorseCameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HorseCameraRoot"));

	HorseCameraRoot->SetupAttachment(GetCapsuleComponent());
	HorseCameraRoot->SetUsingAbsoluteRotation(true);
	HorseCameraRoot->SetRelativeLocation(FVector(-300.0f, 0.0f, 150.0f));
	HorseCameraRoot->SetRelativeRotation(FRotator(-10.0f, 0.0f, 0.0f));

	HorseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("HorseCamera"));

	HorseCamera->SetupAttachment(HorseCameraRoot);
	HorseCamera->SetRelativeLocation(FVector::ZeroVector);
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

}

void AHorse::EnterVehicle_Implementation(APawn* VehicleCharacter, int32 SeatIndex)
{
	AActionCharacter* Character = Cast<AActionCharacter>(VehicleCharacter);

	if (!Character)
		return;

	// 이미 운전자가 있으면 탑승 불가
	if (DriverCharacter)
		return;

	if (!SeatPoints.IsValidIndex(0) || !SeatPoints[0])
		return;

	AActionPlayerController* PC =
		Cast<AActionPlayerController>(Character->GetController());

	if (!PC)
		return;

	DriverCharacter = Character;

	// 차량 충돌 설정
	GetMesh()->SetCollisionResponseToChannel(
		ECC_GameTraceChannel4,
		ECR_Ignore
	);

	// 캐릭터 차량 상태
	Character->SetCurrentHorse(this);
	Character->SetIsInVehicle(true, true);

	// 운전석에 부착
	Character->AttachToComponent(
		SeatPoints[0],
		FAttachmentTransformRules::SnapToTargetNotIncludingScale
	);

	// 차량 Possess
	PC->Possess(this);

	if (PC->IsLocalController())
	{
		AddHorseInputMapping();
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[VEHICLE ENTER] Vehicle=%s Character=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Character)
	);
}

void AHorse::ExitVehicle_Implementation(APawn* VehicleCharacter)
{
}

AActionCharacter* AHorse::GetDriverCharacter() const
{
	return nullptr;
}

void AHorse::RequestMountVehicle_Implementation(ACharacter* VehicleCharacter)
{
}
void AHorse::AddHorseInputMapping()
{
	UE_LOG(LogTemp, Warning,
		TEXT("[HORSE IMC] AddHorseInputMapping ENTER | Horse=%s Local=%d PC=%s"),
		*GetNameSafe(this),
		IsLocallyControlled(),
		*GetNameSafe(GetController()));

	if (!IsLocallyControlled())
		return;

	APlayerController* PC =
		Cast<APlayerController>(GetController());

	if (!PC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[HORSE IMC] PC is NULL"));
		return;
	}

	ULocalPlayer* LocalPlayer =
		PC->GetLocalPlayer();

	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[HORSE IMC] LocalPlayer is NULL"));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<
		UEnhancedInputLocalPlayerSubsystem>();

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[HORSE IMC] Subsystem is NULL"));
		return;
	}

	if (!HorseMappingContext)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[HORSE IMC] HorseMappingContext is NULL"));
		return;
	}
	//Subsystem->RemoveMappingContext(CharacterMappingContext);
	Subsystem->AddMappingContext(
		HorseMappingContext,
		20
	);
	UE_LOG(LogTemp, Warning,
		TEXT("[HORSE IMC] Added SUCCESS | Horse=%s"),
		*GetNameSafe(this));
}

void AHorse::RemoveHorseInputMapping()
{
	AActionPlayerController* PC =
		Cast<AActionPlayerController>(GetController());

	if (!PC || !PC->IsLocalController())
		return;

	ULocalPlayer* LocalPlayer =
		PC->GetLocalPlayer();

	if (!LocalPlayer)
		return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!Subsystem || !HorseMappingContext)
		return;

	Subsystem->RemoveMappingContext(HorseMappingContext);
	//Subsystem->AddMappingContext(CharacterMappingContext, 0);
	UE_LOG(LogTemp, Warning,
		TEXT("[HORSE IMC] Removed | Horse=%s"),
		*GetNameSafe(this));
}

void AHorse::Move(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("[HORSE INPUT] MOVE CALLED"));
	const FVector2D Input = Value.Get<FVector2D>();

	UHorseMovementComponent* Movement =
		Cast<UHorseMovementComponent>(GetCharacterMovement());

	if (!Movement)
	{
		return;
	}
	UE_LOG(LogTemp, Warning,
		TEXT("[HORSE MOVE] Mode=%d Ground=%d"),
		(int32)Movement->MovementMode,
		Movement->IsMovingOnGround());	
	// A / D
	Movement->SetTurnInput(Input.X);

	// W / S
	if (!FMath::IsNearlyZero(Input.Y))
	{
		AddMovementInput(
			GetActorForwardVector(),
			Input.Y
		);
	}
}
void AHorse::StopMove(const FInputActionValue& Value)
{
	if (UHorseMovementComponent* Movement =
		Cast<UHorseMovementComponent>(
			GetCharacterMovement()
		))
	{
		Movement->SetTurnInput(0.0f);
	}
}
void AHorse::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();

	CameraYaw += Input.X;

	CameraPitch = FMath::Clamp(
		CameraPitch + Input.Y,
		-80.0f,
		80.0f
	);

	HorseCamera->SetWorldRotation(
		FRotator(
			CameraPitch,
			CameraYaw,
			0.0f
		)
	);
}

void AHorse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AHorse::ServerTurn_Implementation(float Input)
{
	AddActorLocalRotation(
		FRotator(
			0.0f,
			Input * 100.0f * GetWorld()->GetDeltaSeconds(),
			0.0f
		)
	);
}

// Called when the game starts or when spawned
void AHorse::BeginPlay()
{
	Super::BeginPlay();
	SeatPoints.Empty();

	TArray<USceneComponent*> Components;
	GetComponents<USceneComponent>(Components);

	for (USceneComponent* Component : Components)
	{
		if (!Component) continue;

		if (Component->GetName().StartsWith(TEXT("Seat_")))
		{
			SeatPoints.Add(Component);
		}
	}

	SeatPoints.Sort([](const USceneComponent& A, const USceneComponent& B)
		{
			return A.GetName() < B.GetName();
		});
	if (IsLocallyControlled())
	{
		AddHorseInputMapping();
	}
}

// Called every frame
void AHorse::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AHorse::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogTemp, Warning,
		TEXT("[HORSE INPUT] SetupPlayerInputComponent | Horse=%s"),
		*GetNameSafe(this));

	UEnhancedInputComponent* EnhancedInput =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[HORSE INPUT] EnhancedInputComponent CAST FAILED"));
		return;
	}

	EnhancedInput->BindAction(
		IA_Move,
		ETriggerEvent::Triggered,
		this,
		&AHorse::Move
	);

	EnhancedInput->BindAction(
		IA_Move,
		ETriggerEvent::Completed,
		this,
		&AHorse::StopMove
	);

	EnhancedInput->BindAction(
		IA_Look,
		ETriggerEvent::Triggered,
		this,
		&AHorse::Look
	);

	//GetCharacterMovement()->bOrientRotationToMovement = false;
}
