


#include "Vehicle/HorseAnimInstance.h"
#include "Vehicle/Horse.h"
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

	Speed = FMath::Abs(OwningCharacter->GetTurnInput()) > 0.01f && Velocity.Size2D() < 20 ? 200.0f : Velocity.Size2D();

	const float TargetTurnWeight = FMath::Abs(OwningCharacter->GetTurnInput()) > 0.01f ? 1.0f : 0.0f;

	TurnBlendWeight = FMath::FInterpTo(
		TurnBlendWeight,
		TargetTurnWeight,
		DeltaSeconds,
		5.0f
	);
	// CalculateDirection(Velocity, OwningCharacter->GetActorRotation()) 는
	// 내부적으로 Velocity를 액터의 Rotation 기준 로컬 좌표계로 변환한 다음 Atan2(Y, X)로 각도를 구함.
	//   0도 : 정면(W)
	// +90도 : 오른쪽(D)
	// -90도 : 왼쪽(A)
	// +-180 : 뒤쪽(S)
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, OwningCharacter->GetActorRotation());
}
