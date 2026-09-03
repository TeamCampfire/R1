/// 최초작성 : 2026.09.03
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimalAnimInstance.generated.h"

class AAnimalCharacter;

/**
 * 동물(사슴 등) 애니메이션 블루프린트의 부모 C++ AnimInstance
 */
UCLASS()
class R1_API UAnimalAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	TObjectPtr<AAnimalCharacter> AnimalCharacter;

	// 이동 속도 (cm/s)
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed = 0.0f;

	// 이동 중 여부
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bShouldMove = false;

	// 사망 여부
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;
};
