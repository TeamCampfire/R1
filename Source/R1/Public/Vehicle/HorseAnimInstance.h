

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HorseAnimInstance.generated.h"

class AHorse;
class UCharacterMovementComponent;
class USkeletalMeshComponent;
/**
 *
 */
UCLASS()
class R1_API UHorseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	double Speed = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	double Direction = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	TObjectPtr<USkeletalMeshComponent> SourceSkeletalMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	bool bIsActive = false;

	UPROPERTY(BlueprintReadOnly)
	float TurnBlendWeight = 0.0f;
private:
	UPROPERTY()
	TObjectPtr<AHorse> OwningCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

};
