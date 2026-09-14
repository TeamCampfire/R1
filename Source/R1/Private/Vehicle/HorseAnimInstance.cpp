


#include "Vehicle/HorseAnimInstance.h"
#include "Vehicle/Horse.h"
#include "Vehicle/HorseMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UHorseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningCharacter = Cast<AHorse>(TryGetPawnOwner());
	if (OwningCharacter)
	{
		MovementComponent = OwningCharacter->GetCharacterMovement();
		SourceSkeletalMesh = OwningCharacter->GetMesh();
	}
}

void UHorseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter || !MovementComponent)
	{
		return;
	}

	const FVector Velocity = OwningCharacter->GetVelocity();

	UHorseMovementComponent* Movement =
		Cast<UHorseMovementComponent>(
			OwningCharacter->GetCharacterMovement());

	

	const float TurnInput =
		Movement ? Movement->GetTurnInput() : 0.0f;

	const float TargetTurnWeight = FMath::Abs(TurnInput) > 0.01f ? 1.0f : 0.0f;

	TurnBlendWeight = FMath::FInterpTo(
		TurnBlendWeight,
		TargetTurnWeight,
		DeltaSeconds,
		10.0f
	);


	Speed = (FMath::Abs(OwningCharacter->GetTurnInput()) > 0.01f) && (Velocity.Size2D() < 300.1f) ? 600.0f : Velocity.Size2D();
	Direction = TurnInput;
}
