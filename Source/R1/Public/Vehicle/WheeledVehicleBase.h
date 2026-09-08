

#pragma once

#include "Interface/Vehicle/VehicleInterface.h"

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "WheeledVehicleBase.generated.h"


class UChaosWheeledVehicleMovementComponent;
class UStaticMeshComponent;
class AActionCharacter;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;

/**
 * 
 */
UCLASS()
class R1_API AWheeledVehicleBase : public AWheeledVehiclePawn, public IVehicleInterface
{
	GENERATED_BODY()

public:
	AWheeledVehicleBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	FORCEINLINE UChaosWheeledVehicleMovementComponent* GetChaosVehicleMovement() const { return ChaosVehicleMovement; }

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// VehicleInterface
	virtual void RequestMountVehicle_Implementation(
		ACharacter* VehicleCharacter) override;

	virtual void EnterVehicle_Implementation(
		APawn* VehicleCharacter, int32 InSeatIndex) override;

	virtual void ExitVehicle_Implementation(
		APawn* VehicleCharacter) override;

	virtual AActionCharacter* GetDriverCharacter() const override;

	// 이미 좌석에 플레이어가 있는가?
	UFUNCTION()
	bool IsSeatOccupied(int32 InSeatIndex) const;


	// 탈것	입력 맵핑 추가 함수
	UFUNCTION()
	void AddVehicleInputMapping();

	// 탈것	입력 맵핑 제거 함수
	UFUNCTION()
	void RemoveVehicleInputMapping();

	// 클라용 IMC 제거 RPC
	UFUNCTION(Client, Reliable)
	void ClientRemoveVehicleInputMapping();

	// 운전자용 하차 함수
	UFUNCTION()
	void PressToExitVehicle();

	// 승객용 하차 함수
	UFUNCTION()
	void PassengerPressToExitVehicle(AActionCharacter* InCharacter);

protected:
	void InitializeSeatPoints();

	UFUNCTION(Server, Reliable)
	void ServerExitVehicle(AActionCharacter* InCharacter);

	virtual void SetupPlayerInputComponent(
		UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void MoveForward(const FInputActionValue& Value);

	UFUNCTION()
	void StopMoveForward(const FInputActionValue& Value);

	UFUNCTION()
	void Brake(const FInputActionValue& Value);

	UFUNCTION()
	void StopBrake(const FInputActionValue& Value);

	UFUNCTION()
	void MoveRight(const FInputActionValue& Value);

	UFUNCTION()
	void StopSteering(const FInputActionValue& Value);

	UFUNCTION()
	void OnLook(const FInputActionValue& InValue);

	UFUNCTION()
	void OnFreeLookPressed(const FInputActionValue& InValue);

	UFUNCTION()
	void OnFreeLookReleased(const FInputActionValue& InValue);

protected:
	// 운전자 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> DriverCamera;

	// 섀시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Chassis;
	// 전방	왼쪽 타이어
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TireFrontLeft;
	// 전방	오른쪽 타이어
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TireFrontRight;
	// 후방	왼쪽 타이어
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TireRearLeft;
	// 후방	오른쪽 타이어
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TireRearRight;
	// 차량 이동 컴포넌트
	UPROPERTY()
	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

public:

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Seats")
	TArray<TObjectPtr<USceneComponent>> SeatPoints;

	UPROPERTY(Replicated)
	TObjectPtr<AActionCharacter> DriverCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_Vehicle;

	// 스로틀
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_VehicleThrottle;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_VehicleBrake;

	// 조향
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_VehicleSteering;

	// 운전자 하차
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_VehicleExit;

	// 카메라 회전
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Look;

	// 카메라 회전
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_FreeLook;


private:
	UPROPERTY()
	TArray<TObjectPtr<APawn>> SeatOccupants;

	bool bCanFreeLook = false;
};
