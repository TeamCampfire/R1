

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/VehicleBase.h"
#include "InputActionValue.h"
#include "CarBase.generated.h"


class UInputAction;
class UCameraComponent;
class USceneComponent;
class UFloatingPawnMovement;
/**
 * 
 */
UCLASS()
class R1_API ACarBase : public AVehicleBase
{
	GENERATED_BODY()
public:
	// Sets default values for this pawn's properties
	ACarBase();
protected:
	//void SetupInputMappingContext(APlayerController* PlayerController);
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void OnLookInput(const FInputActionValue& InValue);
	void OnMoveInput(const FInputActionValue& InValue);

	virtual void OnRep_ReplicatedMovement() override;
#pragma region IA

	// 이동
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Move;


	// 회전
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Look;

#pragma endregion

	UFUNCTION(Server, Unreliable)
	void ServerMove(const FVector2D& MoveValue);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UFloatingPawnMovement> FloatingMovement;

	// 운전자 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> DriverCamera;



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
};
