#include "Vehicle/CarBase.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Components/SceneComponent.h"
#include "Character/ActionCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"



ACarBase::ACarBase()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	DriverCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DriverCamera"));
	DriverCamera->SetupAttachment(RootComponent);

	bReplicates = true;
	SetReplicateMovement(true);

	FloatingMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));

	FloatingMovement->UpdatedComponent = GetRootComponent();

	FloatingMovement->MaxSpeed = 600.0f;
	FloatingMovement->Acceleration = 2048.0f;
	FloatingMovement->Deceleration = 2048.0f;
}

void ACarBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EIC =
		Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ACarBase::OnMoveInput);
		if (IA_Look)
		{
			UE_LOG(LogTemp, Warning, TEXT("IA_Look = %s"), *IA_Look->GetName());

			EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ACarBase::OnLookInput);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("IA_Look is NULL"));
		}
		if (IA_Move)
		{
			EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ACarBase::OnMoveInput);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("IA_Move is NULL"));
		}
	}
}

void ACarBase::OnLookInput(const FInputActionValue& InValue)
{
	const FVector2D LookValue = InValue.Get<FVector2D>();

	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);
}

void ACarBase::OnMoveInput(const FInputActionValue& InValue)
{
	const FVector2D MoveValue = InValue.Get<FVector2D>();

	AddMovementInput(GetActorForwardVector(), MoveValue.Y);
	AddMovementInput(GetActorRightVector(), MoveValue.X);

	if (!HasAuthority())
	{
		ServerMove(MoveValue);
	}
}

void ACarBase::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
}

void ACarBase::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning,
		TEXT("[CAR MOVEMENT] Car=%s Movement=%s UpdatedComponent=%s"),
		*GetNameSafe(this),
		*GetNameSafe(FloatingMovement),
		*GetNameSafe(FloatingMovement ? FloatingMovement->UpdatedComponent : nullptr));
}

void ACarBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACarBase::ServerMove_Implementation(const FVector2D& MoveValue)
{
	const FVector Direction =
		GetActorForwardVector() * MoveValue.Y +
		GetActorRightVector() * MoveValue.X;

	const FVector NewLocation =
		GetActorLocation() + Direction * 10.0f;

	SetActorLocation(NewLocation);
}
