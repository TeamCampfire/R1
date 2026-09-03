/// 최초작성 : 2026.09.03
/// 작성자 : 주형진
#include "Character/Animal/AnimalAnimInstance.h"
#include "Character/Animal/AnimalCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAnimalAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	AnimalCharacter = Cast<AAnimalCharacter>(TryGetPawnOwner());
}

void UAnimalAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!AnimalCharacter.IsValid())
	{
		AnimalCharacter = Cast<AAnimalCharacter>(TryGetPawnOwner());
	}

	if (AnimalCharacter.IsValid())
	{
		AAnimalCharacter* CharacterPtr = AnimalCharacter.Get();
		GroundSpeed = CharacterPtr->GetVelocity().Size2D();
		bShouldMove = (GroundSpeed > 5.0f);
		bIsDead = CharacterPtr->IsDead();
	}
}
