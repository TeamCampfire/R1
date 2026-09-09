#include "Vehicle/WheeledVehicleBase.h"
#include "ChaosWheeledVehicleMovementComponent.h"
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
	//bUseControllerRotationYaw = true;

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
}

void AWheeledVehicleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bCanFreeLook)
	{
		FRotator CurrentRotation = DriverCamera->GetRelativeRotation();

		FRotator TargetRotation = FRotator::ZeroRotator;

		CurrentRotation.Yaw = FMath::FInterpTo(
			CurrentRotation.Yaw,
			TargetRotation.Yaw,
			DeltaTime,
			5.0f);

		CurrentRotation.Pitch = FMath::FInterpTo(
			CurrentRotation.Pitch,
			TargetRotation.Pitch,
			DeltaTime,
			5.0f);

		DriverCamera->SetRelativeRotation(CurrentRotation);
	}
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

	// 탑승 차량 저장
	VehicleChar->SetCurrentVehicle(this);

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
	UE_LOG(LogTemp, Warning,
		TEXT("[EXIT] Vehicle=%s Character=%s Authority=%d Local=%d"),
		*GetNameSafe(this),
		*GetNameSafe(VehicleCharacter),
		HasAuthority(),
		IsLocallyControlled());

	if (!VehicleCharacter)return;

	int32 SeatIndex = SeatOccupants.IndexOfByKey(VehicleCharacter);

	if (SeatIndex == INDEX_NONE) return;

	AActionCharacter* Character =
		Cast<AActionCharacter>(VehicleCharacter);

	if (!Character) return;

	const bool bWasDriver = (SeatIndex == 0);

	SeatOccupants[SeatIndex] = nullptr;

	// 차량에서 분리
	FVector ExitLocation =
		SeatPoints[SeatIndex]->GetComponentLocation()
		+ SeatPoints[SeatIndex]->GetComponentTransform().TransformVector(
			FVector(0.0f, SeatIndex%2 ==0? -180.0f:180.0f, 0.0f));

	Character->SetActorLocation(ExitLocation);
	Character->DetachFromActor(
		FDetachmentTransformRules::KeepWorldTransform);

	FRotator NewRotation = Character->GetActorRotation();
	NewRotation.Pitch = 0.0f;
	NewRotation.Roll = 0.0f;

	Character->SetActorRotation(NewRotation);

	// 캐릭터 상태 복구
	Character->SetIsInVehicle(false, bWasDriver);
	Character->SetCurrentVehicle(nullptr);

	// 운전자였을 경우에만 차량 Possess 해제
	if (bWasDriver)
	{
		DriverCharacter = nullptr;

		// 현재 Vehicle을 Possess하고 있는 Controller를 가져온다.
		AActionPlayerController* PC =
			Cast<AActionPlayerController>(GetController());

		if (PC)
		{
			ClientRemoveVehicleInputMapping();

			PC->Possess(Character);

			UE_LOG(LogTemp, Warning,
				TEXT("[EXIT] Possess Character = %s | CurrentPawn = %s"),
				*GetNameSafe(Character),
				*GetNameSafe(PC->GetPawn()));

			if (PC->IsLocalController())
			{
				if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
				{
					if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
						LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
					{
						Subsystem->RemoveMappingContext(IMC_Vehicle);
					}
				}
			}
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

void AWheeledVehicleBase::RemoveVehicleInputMapping()
{
	AActionPlayerController* PC =
		Cast<AActionPlayerController>(GetController());

	if (!PC || !PC->IsLocalController())
		return;

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
		return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!Subsystem || !IMC_Vehicle)
		return;

	Subsystem->RemoveMappingContext(IMC_Vehicle);

	UE_LOG(LogTemp, Warning,
		TEXT("[VEHICLE IMC] Removed | Vehicle=%s"),
		*GetNameSafe(this));
}

void AWheeledVehicleBase::ClientRemoveVehicleInputMapping_Implementation()
{
	RemoveVehicleInputMapping();
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

void AWheeledVehicleBase::ServerExitVehicle_Implementation(AActionCharacter* InCharacter)
{
	if (!InCharacter) return;

	IVehicleInterface::Execute_ExitVehicle(
		this,
		InCharacter);
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
		IA_VehicleThrottle,
		ETriggerEvent::Completed,
		this,
		&AWheeledVehicleBase::StopMoveForward);

	EnhancedInputComponent->BindAction(
		IA_VehicleBrake,
		ETriggerEvent::Triggered,
		this,
		&AWheeledVehicleBase::Brake);

	EnhancedInputComponent->BindAction(
		IA_VehicleBrake,
		ETriggerEvent::Completed,
		this,
		&AWheeledVehicleBase::StopBrake);

	EnhancedInputComponent->BindAction(
		IA_VehicleSteering,
		ETriggerEvent::Triggered,
		this,
		&AWheeledVehicleBase::MoveRight);

	EnhancedInputComponent->BindAction(
		IA_VehicleSteering,
		ETriggerEvent::Completed,
		this,
		&AWheeledVehicleBase::StopSteering);

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

	EnhancedInputComponent->BindAction(
		IA_FreeLook,
		ETriggerEvent::Started,
		this,
		&AWheeledVehicleBase::OnFreeLookPressed);

	//EnhancedInputComponent->BindAction(
	//	IA_FreeLook,
	//	ETriggerEvent::Completed,
	//	this,
	//	&AWheeledVehicleBase::OnFreeLookReleased);

}

void AWheeledVehicleBase::MoveForward(const FInputActionValue& Value)
{
	const float Throttle = Value.Get<float>();

	if (!ChaosVehicleMovement)
		return;

	ChaosVehicleMovement->SetThrottleInput(Throttle);
	ChaosVehicleMovement->SetBrakeInput(0.0f);

}

void AWheeledVehicleBase::StopMoveForward(const FInputActionValue& Value)
{
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void AWheeledVehicleBase::Brake(const FInputActionValue& Value)
{
	const float Brake = Value.Get<float>();

	if (!ChaosVehicleMovement)
		return;

	ChaosVehicleMovement->SetBrakeInput(Brake);
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void AWheeledVehicleBase::StopBrake(const FInputActionValue& Value)
{
	ChaosVehicleMovement->SetBrakeInput(0.0f);
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void AWheeledVehicleBase::MoveRight(const FInputActionValue& Value)
{
	const float Steering = Value.Get<float>();

	if (!ChaosVehicleMovement)
		return;

	ChaosVehicleMovement->SetSteeringInput(Steering);
}

void AWheeledVehicleBase::StopSteering(const FInputActionValue& Value)
{
	ChaosVehicleMovement->SetSteeringInput(0.0f);
}

void AWheeledVehicleBase::PressToExitVehicle()
{
	AActionCharacter* Character = DriverCharacter;

	if (!Character)
		return;

	//IVehicleInterface::Execute_ExitVehicle(this, Character);
	ServerExitVehicle(Character);
}

void AWheeledVehicleBase::PassengerPressToExitVehicle(AActionCharacter* InCharacter)
{
	if (!InCharacter)
		return;

	ServerExitVehicle(InCharacter);
}

void AWheeledVehicleBase::OnLook(const FInputActionValue& InValue)
{
	if (!bCanFreeLook)
		return;

	const FVector2D LookValue = InValue.Get<FVector2D>();

	FRotator Rotation = DriverCamera->GetRelativeRotation();

	Rotation.Yaw += LookValue.X *1.4;
	Rotation.Pitch += LookValue.Y *1.4;

	Rotation.Pitch = FMath::Clamp(Rotation.Pitch, -60.0f, 60.0f);

	DriverCamera->SetRelativeRotation(Rotation);
}

void AWheeledVehicleBase::OnFreeLookPressed(const FInputActionValue& InValue)
{
	bCanFreeLook = bCanFreeLook? false : true;
	//bUseControllerRotationYaw = false;
	//UE_LOG(LogTemp, Warning, TEXT("[FREELOOK] ON"));
}

void AWheeledVehicleBase::OnFreeLookReleased(const FInputActionValue& InValue)
{
	bCanFreeLook = false;
	//bUseControllerRotationYaw = true;
	UE_LOG(LogTemp, Warning, TEXT("[FREELOOK] OFF"));
}
