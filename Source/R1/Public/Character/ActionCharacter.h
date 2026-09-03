/// 최초작성 : 2026.08.25
/// 작 성 자 : 최 요 환

// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Interface/StatInterface.h"
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
class UStatComponent;
class UInventoryComponent;
class UInteractionComponent;
class UEquipmentComponent;
class UItemDataBase;

UCLASS()
class R1_API AActionCharacter : public ACharacter, public IStatInterface
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

	virtual UStatComponent* GetStatComponent() const override;
	FORCEINLINE bool IsSprinting() const { return bIsSprinting; }
	
	// 공격 프로세스
	UFUNCTION(BlueprintCallable)
	void ProcessAttack();

	// 채집(ProcessAttack)으로 얻은 보상 아이템을 서버 권한으로 인벤토리에 지급 요청한다 —
	// 히트 판정(DetectdObjectInAttackRange)/자원 소모(Execute_OnHitted) 자체는 아직 클라이언트
	// 로컬로 계산되고(전투/채집 시스템 자체의 서버 권한화는 별도 작업), 그 결과로 뭘 얼마나
	// 지급할지만 서버가 최종 승인한다 — AFishingRod::Server_FinishFishing과 동일한 패턴.
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_GrantHarvestReward(UItemDataBase* ItemData, int32 Count);

	// 공격 대상(자원 등)에 대한 타격을 서버 권한으로 처리 요청
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ProcessAttackTarget(AActor* TargetActor, const FVector& HitLocation);

	// 사망
	UFUNCTION(BlueprintCallable)
	void Die();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie();

protected:
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;

	// 공격 사거리내에 객체가 있는지 확인하는 함수


protected:
	/// 캐릭터 기본 조작 함수
	void OnMoveAction(const FInputActionValue& Value);	// 이동
	void OnMoveCompleted(const FInputActionValue& Value); // 이동 종료 (키 뗌)
	void OnLookInput(const FInputActionValue& InValue);	// 회전

	void OnSprintPressed();		// 스프린트 누름
	void OnSprintReleased();	// 스프린트 떼기
	void OnCrouchPressed();		// 크라우치 누름
	void OnCrouchReleased();	// 크라우치 떼기
	void OnJumpPressed();		// 점프 누름
	void OnAttackPressed();		// 공격키 누름 (좌클릭 / 도구 주 액션)
	void OnAttackReleased();	// 공격키 뗌 (좌클릭 뗌 / 도구 주 액션 종료)
	void OnSecondaryActionPressed();	// 보조 액션 시작 (우클릭 / 도구 보조 기능 / 조준)
	void OnSecondaryActionReleased();	// 보조 액션 종료 (우클릭 뗌)

	void OnBuildingPlacementPressed();
	void OnRotateBuildingPartPressed();

	void OnInteractPressed();			// 상호작용 시도
	void OnInventoryTogglePressed();	// 인벤토리 패널 토글

	UFUNCTION(BlueprintCallable)		// 블루프린트 테스트로 콜러블 설정
	void OnOptionsTogglePressed();		// 옵션(환경설정) 패널 토글
	void OnUseBeltSlotPressed(int32 BeltIndex);	// 벨트슬롯 단축키(1~6, 0-based 인덱스로 받음)

	// 공격 몽타주 재생 RPC (리슨 서버 및 멀티플레이어 동기화)
	UFUNCTION(Server, Reliable)
	void Server_PlayAttackMontage();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAttackMontage();

	// 무브먼트 값 갱신
	void ApplyMovementSettings();

private:
	// 공격 범위안에 있는 액터를 반환하는 함수
	bool DetectdObjectInAttackRange(FHitResult& OutHitRes);

protected:

	/*--------------------------------
	*			IA 변수
	--------------------------------*/
#pragma region IA

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

	// 건축 파츠 회전 휠클릭
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_RotateBuildingPart;

	// 상호작용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Interact;

	// 인벤토리 토글
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_InventoryToggle;

	// 옵션(환경설정) 패널 토글 (ESC)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_OptionsToggle;

	/// 벨트슬롯 단축키
	// 1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Use_BeltSlot_1;

	// 2
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Use_BeltSlot_2;

	// 3
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Use_BeltSlot_3;

	// 4
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Use_BeltSlot_4;

	// 5
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Use_BeltSlot_5;

	// 6
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_Use_BeltSlot_6;
	//------------------------------------------------------------------

	// 공격(좌클릭 / 도구 주 액션)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Attack;

	// 보조 액션(우클릭 / 도구 보조 기능 / 조준 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_SecondaryAction;
#pragma endregion

public:
	// 손에 든 아이템(도구/무기) 관리 컴포넌트 접근자
	UFUNCTION(BlueprintPure, Category = "Component")
	FORCEINLINE class UHeldItemComponent* GetHeldItemComponent() const { return HeldItemComponent; }

	UFUNCTION(BlueprintPure, Category = "Component")
	class UInventoryComponent* GetInventoryComponent() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<class UHeldItemComponent> HeldItemComponent;


	/*--------------------------------
	*			AM 변수
	--------------------------------*/
#pragma region Anim Montage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AM_Attack;
#pragma endregion
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	float AttackRange = 200.f;

	/// Torso 메시 (Body는 기본으로 있는거 BP에서 할당해서 사용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> TorsoMesh;

	/// LegMesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> LegMesh;

	/// HandMesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> HandMesh;

	/// FeetMesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> FeetMesh;




	/// 컴포넌트
	// 인벤토리
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UInteractionComponent> InteractionComponent;



protected:
	// 스프린트 모드
	bool bIsSprinting = false;

	// 크라우치 모드는 기본 내장 변수 사용

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Input")
	ESprintInputMode SprintInputMode = ESprintInputMode::Hold;	// 기본 Hold

	// 크라우치 모드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Input")
	ECrouchInputMode CrouchInputMode = ECrouchInputMode::Hold; // 기본 Hold

	// 스탯 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStatComponent> StatComponent = nullptr;

	float DefaultEyeHeight = 0.f;
	float CurrentWorldEyeHeight = 0.f; // 로컬이 아니라 "월드" 목표 눈높이
};
