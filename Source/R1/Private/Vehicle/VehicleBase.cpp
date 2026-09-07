#include "Vehicle/VehicleBase.h"
#include "Character/ActionPlayerController.h"
#include "Character/ActionCharacter.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AVehicleBase::AVehicleBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
}

void AVehicleBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(AVehicleBase, DriverCharacter);
}

void AVehicleBase::RequestMountVehicle_Implementation(ACharacter* VehicleCharacter)
{
}

void AVehicleBase::EnterVehicle_Implementation(APawn* VehicleCharacter, int32 InSeatIndex)
{

	AActionCharacter* VehicleChar = Cast<AActionCharacter>(VehicleCharacter);
	AActionPlayerController* PC = Cast<AActionPlayerController>(VehicleChar->GetController());
	bool bIsDriver = (InSeatIndex == 0);

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
		PC->Possess(this);

}

void AVehicleBase::ExitVehicle_Implementation(APawn* VehicleCharacter)
{
}

AActionCharacter* AVehicleBase::GetDriverCharacter() const
{
	return DriverCharacter;
}

void AVehicleBase::InitializeSeatPoints()
{
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
}

void AVehicleBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->AddMappingContext(IMC_Vehicle, 0);
			}
		}
	}
}



// Called when the game starts or when spawned
void AVehicleBase::BeginPlay()
{
	Super::BeginPlay();
	InitializeSeatPoints();
}

// Called every frame
void AVehicleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AVehicleBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

