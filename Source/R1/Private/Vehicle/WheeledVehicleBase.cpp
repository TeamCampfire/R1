#include "ChaosWheeledVehicleMovementComponent.h"
#include "Vehicle/WheeledVehicleBase.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Character/ActionCharacter.h"
#include "Character/ActionPlayerController.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"


#include "Vehicle/vehicletestWheelFront.h"
#include "Vehicle/vehicletestWheelRear.h"

AWheeledVehicleBase::AWheeledVehicleBase()
{
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(TEXT("Vehicle"));

	ChaosVehicleMovement =
		CastChecked<UChaosWheeledVehicleMovementComponent>(
			GetVehicleMovement());

	// Camera
	DriverCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DriverCamera"));
	DriverCamera->SetupAttachment(RootComponent);

	// Chassis
	Chassis = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("Chassis"));
	Chassis->SetupAttachment(GetMesh());

	// Front Left
	TireFrontLeft = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("Tire Front Left"));
	TireFrontLeft->SetupAttachment(
		GetMesh(), TEXT("VisWheel_FL"));
	TireFrontLeft->SetCollisionProfileName(TEXT("NoCollision"));

	// Front Right
	TireFrontRight = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("Tire Front Right"));
	TireFrontRight->SetupAttachment(
		GetMesh(), TEXT("VisWheel_FR"));
	TireFrontRight->SetCollisionProfileName(TEXT("NoCollision"));
	TireFrontRight->SetRelativeRotation(
		FRotator(0.0f, 180.0f, 0.0f));

	// Rear Left
	TireRearLeft = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("Tire Rear Left"));
	TireRearLeft->SetupAttachment(
		GetMesh(), TEXT("VisWheel_BL"));
	TireRearLeft->SetCollisionProfileName(TEXT("NoCollision"));

	// Rear Right
	TireRearRight = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("Tire Rear Right"));
	TireRearRight->SetupAttachment(
		GetMesh(), TEXT("VisWheel_BR"));
	TireRearRight->SetCollisionProfileName(TEXT("NoCollision"));
	TireRearRight->SetRelativeRotation(
		FRotator(0.0f, 180.0f, 0.0f));

	ChaosVehicleMovement->TransmissionSetup.bUseAutomaticGears = true;

	ChaosVehicleMovement->WheelSetups.SetNum(4);

	ChaosVehicleMovement->WheelSetups[0].WheelClass =
		UvehicletestWheelFront::StaticClass();
	ChaosVehicleMovement->WheelSetups[0].BoneName =
		TEXT("PhysWheel_FL");

	ChaosVehicleMovement->WheelSetups[1].WheelClass =
		UvehicletestWheelFront::StaticClass();
	ChaosVehicleMovement->WheelSetups[1].BoneName =
		TEXT("PhysWheel_FR");

	ChaosVehicleMovement->WheelSetups[2].WheelClass =
		UvehicletestWheelRear::StaticClass();
	ChaosVehicleMovement->WheelSetups[2].BoneName =
		TEXT("PhysWheel_BL");

	ChaosVehicleMovement->WheelSetups[3].WheelClass =
		UvehicletestWheelRear::StaticClass();
	ChaosVehicleMovement->WheelSetups[3].BoneName =
		TEXT("PhysWheel_BR");

	ChaosVehicleMovement->EngineSetup.MaxTorque = 600.0f;
	ChaosVehicleMovement->EngineSetup.MaxRPM = 5000.0f;
	ChaosVehicleMovement->EngineSetup.EngineIdleRPM = 1200.0f;
	ChaosVehicleMovement->EngineSetup.EngineBrakeEffect = 0.05f;
	ChaosVehicleMovement->EngineSetup.EngineRevUpMOI = 5.0f;
	ChaosVehicleMovement->EngineSetup.EngineRevDownRate = 600.0f;

	ChaosVehicleMovement->DifferentialSetup.DifferentialType =
		EVehicleDifferential::AllWheelDrive;

	ChaosVehicleMovement->DifferentialSetup.FrontRearSplit = 0.5f;

	ChaosVehicleMovement->SteeringSetup.SteeringType =
		ESteeringType::AngleRatio;

	ChaosVehicleMovement->SteeringSetup.AngleRatio = 0.7f;

	ChaosVehicleMovement->ChassisHeight = 160.0f;
	ChaosVehicleMovement->DragCoefficient = 0.1f;
	ChaosVehicleMovement->DownforceCoefficient = 0.4f;
	ChaosVehicleMovement->CenterOfMassOverride = FVector(0.0f, 0.0f, 75.0f);
	ChaosVehicleMovement->bEnableCenterOfMassOverride = true;

	ChaosVehicleMovement->bLegacyWheelFrictionPosition = false;
}

void AWheeledVehicleBase::BeginPlay()
{
	Super::BeginPlay();	
	InitializeSeatPoints();
	SeatOccupants.SetNum(SeatPoints.Num());
	for (int32 i = 0; i < ChaosVehicleMovement->WheelSetups.Num(); ++i)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WHEEL %d] Bone=%s Class=%s"),
			i,
			*ChaosVehicleMovement->WheelSetups[i].BoneName.ToString(),
			*GetNameSafe(ChaosVehicleMovement->WheelSetups[i].WheelClass));
	}
}

void AWheeledVehicleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWheeledVehicleBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(AWheeledVehicleBase, DriverCharacter);
}

void AWheeledVehicleBase::RequestMountVehicle_Implementation(ACharacter* VehicleCharacter)
{
}

void AWheeledVehicleBase::EnterVehicle_Implementation(APawn* VehicleCharacter, int32 InSeatIndex)
{
	AActionCharacter* VehicleChar = Cast<AActionCharacter>(VehicleCharacter);
	AActionPlayerController* PC = Cast<AActionPlayerController>(VehicleChar->GetController());
	bool bIsDriver = (InSeatIndex == 0);

	if (IsSeatOccupied(InSeatIndex))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[VEHICLE] Seat %d is already occupied"),
			InSeatIndex);

		return;
	}

	SeatOccupants[InSeatIndex] = VehicleChar;

	// 운전석
	if (InSeatIndex == 0)
	{
		DriverCharacter = VehicleChar;
	}

	if (!SeatPoints.IsValidIndex(InSeatIndex)) return;

	VehicleChar->SetIsInVehicle(true, bIsDriver);
	USceneComponent* SeatScene = SeatPoints[InSeatIndex];

	if (!SeatScene) return;

	VehicleChar->AttachToComponent(
		SeatScene,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale
	);
	UE_LOG(LogTemp, Warning,
		TEXT("[ENTER VEHICLE] Vehicle=%s Char=%s Authority=%d Local=%d PC=%s"),
		*GetNameSafe(this),
		*GetNameSafe(VehicleChar),
		HasAuthority(),
		VehicleChar->IsLocallyControlled(),
		*GetNameSafe(PC)
	);

	if (InSeatIndex == 0)
	{
		PC->Possess(this);
		if (PC->IsLocalController())
		{
			if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
					LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[VEHICLE IMC] Add IMC_Vehicle | Authority=%d Local=%d IMC=%s"),
						HasAuthority(),
						PC->IsLocalController(),
						IMC_Vehicle ? TEXT("VALID") : TEXT("NULL"));
					Subsystem->AddMappingContext(IMC_Vehicle, 10);
				}
			}
		}
	}

}

void AWheeledVehicleBase::ExitVehicle_Implementation(APawn* VehicleCharacter)
{
	if (!VehicleCharacter)return;

	int32 SeatIndex = SeatOccupants.IndexOfByKey(VehicleCharacter);

	if (SeatIndex == INDEX_NONE) return;

	AActionCharacter* Character =
		Cast<AActionCharacter>(VehicleCharacter);

	if (!Character) return;

	const bool bWasDriver = (SeatIndex == 0);

	SeatOccupants[SeatIndex] = nullptr;

	// 차량에서 분리
	Character->DetachFromActor(
		FDetachmentTransformRules::KeepWorldTransform);

	// 캐릭터 상태 복구
	Character->SetIsInVehicle(false, bWasDriver);

	// 운전자였을 경우에만 차량 Possess 해제
	if (bWasDriver)
	{
		DriverCharacter = nullptr;

		AActionPlayerController* PC =
			Cast<AActionPlayerController>(
				Character->GetController());

		if (PC)
		{
			if (PC->IsLocalController())
			{
				if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
				{
					if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
						LocalPlayer->GetSubsystem<
						UEnhancedInputLocalPlayerSubsystem>())
					{
						Subsystem->RemoveMappingContext(IMC_Vehicle);
					}
				}
			}

			PC->Possess(Character);
		}
	}
}

AActionCharacter* AWheeledVehicleBase::GetDriverCharacter() const
{
	return DriverCharacter;
}

bool AWheeledVehicleBase::IsSeatOccupied(int32 InSeatIndex) const
{
	if (!SeatOccupants.IsValidIndex(InSeatIndex))
		return false;

	return IsValid(SeatOccupants[InSeatIndex]);
}

void AWheeledVehicleBase::InitializeSeatPoints()
{
	SeatPoints.Empty();

	TArray<USceneComponent*> Components;
	GetComponents<USceneComponent>(Components);

	for (USceneComponent* Component : Components)
	{
		if (!Component) continue;

		// 좌석 추가
		if (Component->GetName().StartsWith(TEXT("Seat_")))
		{
			SeatPoints.Add(Component);
		}
	}

	// 좌석 정렬
	SeatPoints.Sort([](const USceneComponent& A, const USceneComponent& B)
		{
			return A.GetName() < B.GetName();
		});
}

void AWheeledVehicleBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent)
		return;

	EnhancedInputComponent->BindAction(
		IA_VehicleThrottle,
		ETriggerEvent::Triggered,
		this,
		&AWheeledVehicleBase::MoveForward);

	EnhancedInputComponent->BindAction(
		IA_VehicleBrake,
		ETriggerEvent::Triggered,
		this,
		&AWheeledVehicleBase::Brake);

	EnhancedInputComponent->BindAction(
		IA_VehicleSteering,
		ETriggerEvent::Triggered,
		this,
		&AWheeledVehicleBase::MoveRight);

	EnhancedInputComponent->BindAction(
		IA_VehicleExit,
		ETriggerEvent::Started,
		this,
		&AWheeledVehicleBase::PressToExitVehicle);
	EnhancedInputComponent->BindAction(
		IA_Look,
		ETriggerEvent::Triggered,
		this,
		&AWheeledVehicleBase::OnLook);
	UE_LOG(LogTemp, Warning,
		TEXT("[VEHICLE INPUT SETUP] Vehicle=%s Authority=%d Local=%d"),
		*GetNameSafe(this),
		HasAuthority(),
		IsLocallyControlled());
}

void AWheeledVehicleBase::MoveForward(const FInputActionValue& Value)
{
	const float Throttle = Value.Get<float>();

	if (!ChaosVehicleMovement)
		return;

	ChaosVehicleMovement->SetThrottleInput(Throttle);
	ChaosVehicleMovement->SetBrakeInput(0.0f);

}

void AWheeledVehicleBase::Brake(const FInputActionValue& Value)
{
	const float Brake = Value.Get<float>();

	if (!ChaosVehicleMovement)
		return;

	ChaosVehicleMovement->SetBrakeInput(Brake);
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void AWheeledVehicleBase::MoveRight(const FInputActionValue& Value)
{
	const float Steering = Value.Get<float>();

	if (!ChaosVehicleMovement)
		return;

	ChaosVehicleMovement->SetSteeringInput(Steering);
}

void AWheeledVehicleBase::PressToExitVehicle()
{
	AActionCharacter* Character = DriverCharacter;

	if (!Character)
		return;

	Execute_ExitVehicle(this, Character);
}

void AWheeledVehicleBase::OnLook(const FInputActionValue& InValue)
{
	const FVector2D LookValue = InValue.Get<FVector2D>();

	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);
}

void AWheeledVehicleBase::AddVehicleInputMapping()
{
	if (!IsLocallyControlled())
		return;

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (!PC)
		return;

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();

	if (!LocalPlayer)
		return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!Subsystem)
		return;

	if (!IMC_Vehicle)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VEHICLE IMC] IMC_Vehicle is NULL"));
		return;
	}

	Subsystem->AddMappingContext(IMC_Vehicle, 10);

	UE_LOG(LogTemp, Warning,
		TEXT("[VEHICLE IMC] Added | Vehicle=%s | Local=%d"),
		*GetNameSafe(this),
		IsLocallyControlled());
}
