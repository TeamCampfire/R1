#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HorseMovementComponent.generated.h"

UCLASS()
class R1_API UHorseMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	UHorseMovementComponent();

	// A/D 회전 입력
	void SetTurnInput(float InTurnInput);

	float GetTurnInput() const
	{
		return TurnInput;
	}

protected:

	// 현재 회전 입력
	float TurnInput = 0.0f;

	// 실제 이동 수행
	virtual void PerformMovement(float DeltaSeconds) override;

	// 서버에서 Client의 SavedMove Custom Flag를 받아 복원
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual class FNetworkPredictionData_Client*
		GetPredictionData_Client() const override;
};
