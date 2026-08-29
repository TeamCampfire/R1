/// 최초작성 : 2026.08.25
/// 작 성 자 : 최 요 환

// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ActionCharacter.generated.h"

UENUM(BlueprintType)
enum class ESprintInputMode : uint8
{
	Hold	 UMETA(DisplayName = "Hold to Sprint"),
	Toggle	 UMETA(DisplayName = "Toggle Sprint")
};

UENUM(BlueprintType)
enum class ECrouchInputMode : uint8
{
	Hold    UMETA(DisplayName = "Hold to Crouch"),
	Toggle  UMETA(DisplayName = "Toggle Crouch")
};

class UInputAction;
class UCameraComponent;

UCLASS()
class R1_API AActionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AActionCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Movement|Input")
	void SetSprintInputMode(ESprintInputMode NewNode);

	UFUNCTION(BlueprintCallable, Category = "Movement|Input")
	void SetCrouchInputMode(ECrouchInputMode NewMode);

	FORCEINLINE bool IsSprinting() const { return bIsSprinting; }

protected:
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;

protected:
	/// 캐릭터 기본 조작 함수
	void OnMoveAction(const FInputActionValue& Value);	// 이동
	void OnLookInput(const FInputActionValue& InValue);	// 회전

	void OnSprintPressed();		// 스프린트 누름
	void OnSprintReleased();	// 스프린트 떼기
	void OnCrouchPressed();		// 크라우치 누름
	void OnCrouchReleased();	// 크라우치 떼기

	void OnJumpPressed();		// 점프 누름

	void OnBuildingPlacementPressed();

	// 무브먼트 값 갱신
	void ApplyMovementSettings();

protected:
	/// 키 맵핑
	// 이동
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Move;

	// 회전
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Look;

	// 점프
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Jump;

	// 스프린트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Sprint;

	// 크라우치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Crouch;

	// 건축물 설치 확정 좌클릭
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_BuildingPlacement;
	//------------------------------------------------------------------
	
	/// 카메라
	// 카메라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FirstPersonCamera;

	// 카메라 상하 회전각 Max
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float ViewPicthMax = 50;

	// 카메라 상하 회전각 Min
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float ViewPicthMin = -60;

	/// 이동 관련 파라미터 (BP에서 조정 가능)
	// 걷기 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float WalkSpeed = 500.f;

	// 점프 파워
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float JumpPower = 400.f;

	// 스프린트 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float SprintSpeed = 900;

	// 크라우치 이동속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float CrouchSpeed = 300.0f;

	// 크라우치 카메라 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Crouch")
	float CrouchInterpSpeed = 5.f; // 값이 클수록 더 빠르게 전환됨

	/// Head 메시 (Body는 기본으로 있는거 BP에서 할당해서 사용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> HeadMesh;

protected:
	// 스프린트 모드
	bool bIsSprinting = false;

	// 크라우치 모드는 기본 내장 변수 사용

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Input")
	ESprintInputMode SprintInputMode = ESprintInputMode::Hold;	// 기본 Hold

	// 크라우치 모드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Input")
	ECrouchInputMode CrouchInputMode = ECrouchInputMode::Hold; // 기본 Hold

	float DefaultEyeHeight = 0.f;
	float CurrentWorldEyeHeight = 0.f; // 로컬이 아니라 "월드" 목표 눈높이
};
