// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ActionAnimInstance.h"
#include "Character/ActionCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UActionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningCharacter = Cast<AActionCharacter>(TryGetPawnOwner());
	if (OwningCharacter)
	{
		MovementComponent = OwningCharacter->GetCharacterMovement();
		SourceSkeletalMesh = OwningCharacter->GetMesh();
	}
}

void UActionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter || !MovementComponent)
	{
		return;
	}

	
	const FVector Velocity = OwningCharacter->GetVelocity();
	Speed = Velocity.Size2D();
	
	// CalculateDirection(Velocity, OwningCharacter->GetActorRotation()) 는
	// 내부적으로 Velocity를 액터의 Rotation 기준 로컬 좌표계로 변환한 다음 Atan2(Y, X)로 각도를 구함.
	//   0도 : 정면(W)
	// +90도 : 오른쪽(D)
	// -90도 : 왼쪽(A)
	// +-180 : 뒤쪽(S)
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, OwningCharacter->GetActorRotation());

	bIsCrouched = OwningCharacter->bIsCrouched;       // 엔진 내장 (public)
	bIsFalling = MovementComponent->IsFalling();
	bIsSprinting = OwningCharacter->IsSprinting();    
}
