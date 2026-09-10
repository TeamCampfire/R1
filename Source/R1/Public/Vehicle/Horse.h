

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Interface/Vehicle/VehicleInterface.h"
#include "Interface/Vehicle/VehicleEntryInterface.h"
#include "Interface/Vehicle/HorseInterface.h"
#include "Horse.generated.h"

class AActionCharacter;
class UCameraComponent;
class USceneComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class R1_API AHorse : public ACharacter, public IVehicleInterface, public IVehicleEntryInterface, public IHorseInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHorse(const FObjectInitializer& ObjectInitializer);

public:
	virtual void EnterVehicle_Implementation(APawn* VehicleCharacter, int32 SeatIndex) override;

	virtual void ExitVehicle_Implementation(APawn* VehicleCharacter) override;

	virtual AActionCharacter* GetDriverCharacter() const override;

	virtual void RequestMountVehicle_Implementation(ACharacter* VehicleCharacter) override;

	// 탈것	입력 맵핑 추가 함수
	UFUNCTION()
	void AddHorseInputMapping();

	// 탈것	입력 맵핑 제거 함수
	UFUNCTION()
	void RemoveHorseInputMapping();

	FORCEINLINE float GetTurnInput() const { return TurnInput; }
protected:
	void Move(const FInputActionValue& Value);
	void StopMove(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	UFUNCTION(Server, Unreliable)
	void ServerTurn(float Input);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


protected:
	/// 카메라
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> HorseCameraRoot;

	// 카메라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* HorseCamera;

	// 카메라 상하 회전각 Max
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float ViewPicthMax = 50;

	// 카메라 상하 회전각 Min
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float ViewPicthMin = -60;

	UPROPERTY()
	float CameraYaw = 0.0f;

	UPROPERTY()
	float CameraPitch = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float TurnInput = 0.0f;

	UPROPERTY()
	AActionCharacter* DriverCharacter = nullptr;
	// 입력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> HorseMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> CharacterMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;
	// 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Seats")
	TArray<TObjectPtr<USceneComponent>> SeatPoints;
};
