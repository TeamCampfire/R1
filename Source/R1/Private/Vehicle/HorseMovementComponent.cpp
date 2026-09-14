#include "Vehicle/HorseMovementComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


/*
 * ---------------------------------------------------------
 * Saved Move
 * ---------------------------------------------------------
 */

class FSavedMove_Horse : public FSavedMove_Character
{
public:

	typedef FSavedMove_Character Super;

	// A/D 입력
	float SavedTurnInput = 0.0f;

	/*
	 * CharacterMovement의 Custom Flag
	 *
	 * FLAG_Custom_0 : 좌회전
	 * FLAG_Custom_1 : 우회전
	 */
	enum
	{
		FLAG_TurnLeft = FLAG_Custom_0,
		FLAG_TurnRight = FLAG_Custom_1
	};


	virtual void Clear() override
	{
		Super::Clear();

		SavedTurnInput = 0.0f;
	}


	virtual void SetMoveFor(
		ACharacter* C,
		float InDeltaTime,
		FVector const& NewAccel,
		FNetworkPredictionData_Client_Character& ClientData
	) override
	{
		Super::SetMoveFor(
			C,
			InDeltaTime,
			NewAccel,
			ClientData
		);

		const UHorseMovementComponent* Movement =
			Cast<UHorseMovementComponent>(
				C->GetCharacterMovement()
			);

		if (Movement)
		{
			SavedTurnInput = Movement->GetTurnInput();
		}
	}


	virtual uint8 GetCompressedFlags() const override
	{
		uint8 Result = Super::GetCompressedFlags();

		if (SavedTurnInput < -0.01f)
		{
			Result |= FLAG_TurnLeft;
		}
		else if (SavedTurnInput > 0.01f)
		{
			Result |= FLAG_TurnRight;
		}

		return Result;
	}


	virtual void PrepMoveFor(ACharacter* C) override
	{
		Super::PrepMoveFor(C);

		UHorseMovementComponent* Movement =
			Cast<UHorseMovementComponent>(
				C->GetCharacterMovement()
			);

		if (Movement)
		{
			Movement->SetTurnInput(SavedTurnInput);
		}
	}
};


/*
 * ---------------------------------------------------------
 * Client Prediction Data
 * ---------------------------------------------------------
 */

class FNetworkPredictionData_Client_Horse
	: public FNetworkPredictionData_Client_Character
{
public:

	FNetworkPredictionData_Client_Horse(
		const UCharacterMovementComponent& ClientMovement)
		: FNetworkPredictionData_Client_Character(ClientMovement)
	{
	}

	virtual FSavedMovePtr AllocateNewMove() override
	{
		return FSavedMovePtr(
			new FSavedMove_Horse()
		);
	}
};


/*
 * ---------------------------------------------------------
 * HorseMovementComponent
 * ---------------------------------------------------------
 */

UHorseMovementComponent::UHorseMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


/*
 * A/D 입력 설정
 */

void UHorseMovementComponent::SetTurnInput(float InTurnInput)
{
	TurnInput = FMath::Clamp(
		InTurnInput,
		-1.0f,
		1.0f
	);
}


/*
 * 실제 회전
 *
 * CharacterMovement가 이동을 수행하기 직전에
 * 현재 TurnInput을 이용해서 말의 Actor를 회전시킨다.
 */

void UHorseMovementComponent::PerformMovement(float DeltaSeconds)
{
	if (CharacterOwner)
	{
		if (!FMath::IsNearlyZero(TurnInput))
		{
			const float DeltaYaw =
				TurnInput *
				100.0f *
				DeltaSeconds;

			CharacterOwner->AddActorLocalRotation(
				FRotator(
					0.0f,
					DeltaYaw,
					0.0f
				)
			);
		}
	}

	Super::PerformMovement(DeltaSeconds);
}


/*
 * 서버가 Client의 SavedMove에서 전달된
 * Custom Flag를 해석하는 부분
 */

void UHorseMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	if (Flags & FSavedMove_Horse::FLAG_TurnLeft)
	{
		TurnInput = -1.0f;
	}
	else if (Flags & FSavedMove_Horse::FLAG_TurnRight)
	{
		TurnInput = 1.0f;
	}
	else
	{
		TurnInput = 0.0f;
	}
}


/*
 * Client Prediction Data 생성
 */

FNetworkPredictionData_Client*
UHorseMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);

	if (!ClientPredictionData)
	{
		UHorseMovementComponent* MutableThis =
			const_cast<UHorseMovementComponent*>(this);

		MutableThis->ClientPredictionData =
			new FNetworkPredictionData_Client_Horse(
				*MutableThis
			);

		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.0f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.0f;
	}

	return ClientPredictionData;
}
