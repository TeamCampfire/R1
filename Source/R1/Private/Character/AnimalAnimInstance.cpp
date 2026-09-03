/// 최초작성 : 2026.09.03
#include "Character/AnimalAnimInstance.h"
#include "Character/AnimalCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAnimalAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	AnimalCharacter = Cast<AAnimalCharacter>(TryGetPawnOwner());
}

void UAnimalAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!AnimalCharacter)
	{
		AnimalCharacter = Cast<AAnimalCharacter>(TryGetPawnOwner());
	}

	if (AnimalCharacter)
	{
		GroundSpeed = AnimalCharacter->GetVelocity().Size2D();
		bShouldMove = GroundSpeed > 3.0f && AnimalCharacter->GetCharacterMovement()->GetCurrentAcceleration().SizeSquared2D() > 0.0f;
		bIsDead = AnimalCharacter->IsDead();
	}
}
