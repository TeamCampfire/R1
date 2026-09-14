

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
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

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
	HorseCameraRoot->SetRelativeLocation(FVector(-300.0f, 0.0f, 150.0f));
	HorseCameraRoot->SetRelativeRotation(FRotator(-10.0f, 0.0f, 0.0f));

	HorseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("HorseCamera"));

	HorseCamera->SetupAttachment(HorseCameraRoot);
	HorseCamera->SetUsingAbsoluteRotation(true);
	HorseCamera->SetRelativeLocation(FVector::ZeroVector);
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// IMC_Default에 이 액션이 매핑되어 있어 Horse를 Possess한 동안에도
	// 기존 상호작 키로 하차할 수 있다. BP에서 별도 액션으로 덮어써도 된다.
	static ConstructorHelpers::FObjectFinder<UInputAction> ExitActionRef(
		TEXT("/Game/Data/Input/IA_Interact.IA_Interact"));
	if (ExitActionRef.Succeeded())
	{
		IA_Exit = ExitActionRef.Object;
	}

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
	//Character->SetActorEnableCollision(false);

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
	if (!HasAuthority() || !VehicleCharacter || VehicleCharacter != DriverCharacter)
	{
		return;
	}

	AActionCharacter* Character = Cast<AActionCharacter>(VehicleCharacter);
	AActionPlayerController* PC = Cast<AActionPlayerController>(GetController());
	if (!Character || !PC)
	{
		return;
	}

	// 말 옆으로 이동한 뒤 부착과 탑승 상태를 풀어준다.
	const USceneComponent* SeatPoint =
		SeatPoints.IsValidIndex(0) ? SeatPoints[0].Get() : nullptr;
	FVector ExitLocation = SeatPoint
		? SeatPoint->GetComponentLocation() - GetActorRightVector() * 150.0f
		: GetActorLocation() - GetActorRightVector() * 150.0f;

	Character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Character->SetCurrentHorse(nullptr);
	Character->SetIsInVehicle(false, true);

	FRotator ExitRotation = GetActorRotation();
	ExitRotation.Pitch = 0.0f;
	ExitRotation.Roll = 0.0f;
	GetWorld()->FindTeleportSpot(Character, ExitLocation, ExitRotation);
	Character->SetActorLocationAndRotation(
		ExitLocation,
		ExitRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	DriverCharacter = nullptr;

	GetMesh()->SetCollisionResponseToChannel(
		ECC_GameTraceChannel4,
		ECR_Block);

	// Client RPC를 먼저 보내 Horse가 아직 해당 클라이언트의 소유일 때 IMC를 제거한다.
	ClientRemoveHorseInputMapping();
	PC->Possess(Character);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[HORSE EXIT] Horse=%s Character=%s CurrentPawn=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Character),
		*GetNameSafe(PC->GetPawn()));
}

AActionCharacter* AHorse::GetDriverCharacter() const
{
	return DriverCharacter;
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

	// Possess 복귀가 먼저 복제된 클라이언트에서도 자신의 PC로 IMC를 제거한다.
	if (!PC && GetWorld())
	{
		PC = Cast<AActionPlayerController>(GetWorld()->GetFirstPlayerController());
	}

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

void AHorse::PressToExitHorse()
{
	ServerExitHorse();
}

void AHorse::ServerExitHorse_Implementation()
{
	if (!DriverCharacter || DriverCharacter->GetCurrentHorse() != this)
	{
		return;
	}

	IVehicleInterface::Execute_ExitVehicle(this, DriverCharacter);
}

void AHorse::ClientRemoveHorseInputMapping_Implementation()
{
	RemoveHorseInputMapping();
}

void AHorse::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	
	UHorseMovementComponent* Movement =
		Cast<UHorseMovementComponent>(GetCharacterMovement());

	if (!Movement)
	{
		return;
	}
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
	DOREPLIFETIME(AHorse, DriverCharacter);
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

	if (IA_Exit)
	{
		EnhancedInput->BindAction(
			IA_Exit,
			ETriggerEvent::Started,
			this,
			&AHorse::PressToExitHorse
		);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[HORSE INPUT] IA_Exit is NULL"));
	}

	//GetCharacterMovement()->bOrientRotationToMovement = false;
}
