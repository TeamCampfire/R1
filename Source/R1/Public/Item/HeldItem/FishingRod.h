/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/HeldItemBase.h"
#include "InputActionValue.h"
#include "FishingRod.generated.h"

class UStaticMeshComponent;
class UCableComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class AFishingBobber;
class AActionCharacter;
class UItemDataBase;
class UInputMappingContext;
class UInputAction;

/**
 * 낚시 상태 열거형
 */
UENUM(BlueprintType)
enum class EFishingState : uint8
{
	Idle			UMETA(DisplayName = "Idle (대기)"),
	Aiming			UMETA(DisplayName = "Aiming (조준 중)"),
	Casting			UMETA(DisplayName = "Casting (비행 중)"),
	WaitingBite		UMETA(DisplayName = "WaitingBite (입질 대기)"),
	Biting			UMETA(DisplayName = "Biting (입질 발생)"),
	Minigame		UMETA(DisplayName = "Minigame (장력 싸움)"),
	ReelingSuccess	UMETA(DisplayName = "ReelingSuccess (성공 회수)"),
	Failed			UMETA(DisplayName = "Failed (실패)")
};

/**
 * 낚싯대 도구 액터
 * 손에 장착되어 캐스팅 프리뷰, 찌 스폰, 낚싯줄 렌더링, 러스트식 텐션 미니게임을 전담합니다.
 */
UCLASS()
class R1_API AFishingRod : public AHeldItemBase
{
	GENERATED_BODY()
	
public:	
	AFishingRod();

	virtual void Tick(float DeltaTime) override;

	// 도구 장착/해제 오버라이드
	virtual void OnEquipped(AActionCharacter* InCharacter) override;
	virtual void OnUnequipped() override;

	// 도구 공통 액션 오버라이드
	virtual void OnPrimaryActionStarted() override;
	virtual void OnPrimaryActionCompleted() override;
	virtual void OnSecondaryActionStarted() override { Input_StartAim(); }
	virtual void OnSecondaryActionCompleted() override { Input_StopAim(); }
	virtual void OnCancelAction() override { Input_Cancel(); }

	// 캐릭터 이동 입력 중계 (낚시 중 A/D 저항, S 릴 감기)
	virtual void OnMoveInput(const FVector2D& MoveValue) override;

	virtual void SetupInputComponent(class UEnhancedInputComponent* PlayerEIC) override;

	virtual bool BlocksCharacterMovement() const override { return IsFishingActive() || IsFinishCooldown(); }
	virtual bool BlocksDefaultAttack() const override { return IsFishingActive() || IsFinishCooldown() || (CurrentState != EFishingState::Idle); }

	// 찌가 수면에 닿았을 때 찌 액터가 호출
	void OnBobberLandedInWater();

	// ---- 입력 핸들러 (캐릭터 또는 입력 컴포넌트에서 호출) ----
	UFUNCTION(BlueprintCallable, Category = "Fishing|Input")
	void Input_StartAim();

	UFUNCTION(BlueprintCallable, Category = "Fishing|Input")
	void Input_StopAim();

	UFUNCTION(BlueprintCallable, Category = "Fishing|Input")
	void Input_CastOrHook();

	void Input_OnCastStarted();
	void Input_OnCastCompleted();

	UFUNCTION(BlueprintCallable, Category = "Fishing|Input")
	void Input_SetReeling(bool bReeling);

	void Input_OnReelTriggered();
	void Input_OnReelCompleted();

	UFUNCTION(BlueprintCallable, Category = "Fishing|Input")
	void Input_SetPull(float PullAxis);

	void Input_OnPullTriggered(const FInputActionValue& Value);
	void Input_OnPullCompleted();

	UFUNCTION(BlueprintCallable, Category = "Fishing|Input")
	void Input_Cancel();

	// ---- 게터 ----
	FORCEINLINE EFishingState GetFishingState() const { return CurrentState; }
	FORCEINLINE float GetTensionPercent() const { return FMath::Clamp(CurrentTension / 100.0f, 0.0f, 1.0f); }
	FORCEINLINE float GetRemainingDistance() const { return CurrentDistance; }
	FORCEINLINE bool IsFishingActive() const { return bIsFishingActive; }
	FORCEINLINE bool IsFinishCooldown() const { return bIsFinishCooldown; }

	// 낚싯대 끝 위치 반환 (소켓이 없어도 안전)
	FVector GetRodTipLocation() const;

protected:
	virtual void BeginPlay() override;

	// 캐스팅 궤적 연산 및 프리뷰
	void UpdateCastingTrajectory(float DeltaTime);

	// 미니게임 메인 틱 (장력 및 거리 연산)
	void UpdateMinigame(float DeltaTime);

	// 물고기 입질 시작
	void TriggerBite();

	// 입질 시간 초과 (놓침)
	void OnBiteMissed();

	// 낚시 종료 처리 (성공/실패)
	void FinishFishing(bool bSuccess);

	// 서버 권한 보상 지급 Server RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FinishFishing(bool bSuccess);

	// 모든 낚시 상태 및 스폰된 액터 초기화
	void ResetFishing();

	// 입력 컨텍스트 전환 및 지연 초기화 안전장치
	void TryInitializeInputs();
	void PushFishingInputContext();
	void PopFishingInputContext();

protected:
	// 낚싯대 외형 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RodMesh;

	// 낚싯대 끝 ➔ 찌를 잇는 낚싯줄 케이블
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCableComponent> FishingLineCable;

	// 찌 액터 클래스 (AFishingBobber)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Classes")
	TSubclassOf<AFishingBobber> BobberClass;

	// 낚시 성공 시 지급할 기본 물고기 DataAsset
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Reward")
	TObjectPtr<UItemDataBase> FishRewardItemData;

	// 기본 매핑 컨텍스트 (IMC_Default: 조준 해제 시 복구용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Input")
	TObjectPtr<UInputMappingContext> IMC_Default;

	// 낚시 전용 입력 매핑 컨텍스트 (IMC_Fishing)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Input")
	TObjectPtr<UInputMappingContext> IMC_Fishing;

	// 낚시 전용 입력 액션들 (낚싯대 자체 소유)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Input")
	TObjectPtr<UInputAction> IA_Fishing_Aim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Input")
	TObjectPtr<UInputAction> IA_Fishing_Cast;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Input")
	TObjectPtr<UInputAction> IA_Fishing_Reel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Input")
	TObjectPtr<UInputAction> IA_Fishing_Pull;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Input")
	TObjectPtr<UInputAction> IA_Fishing_Cancel;

	// 입력 컴포넌트에 액션 직접 바인딩
	void BindRodInputs(class UEnhancedInputComponent* EIC);

	// ---- 캐스팅 파라미터 ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Casting")
	float FixedCastDistance = 750.0f; // 고정 캐스팅 거리 (7.5m)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Casting")
	float MinCatchDistance = 200.0f;  // 플레이어 앞 찌 최소 접근 거리 한계 (2.0m 도달 시 성공)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Casting")
	float MinCastPower = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Casting")
	float MaxCastPower = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Casting")
	FName RodTipSocketName = TEXT("RodTipSocket");

	// 소켓이 없거나 미세조정이 필요할 때 적용할 로컬 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Casting")
	FVector CustomRodTipOffset = FVector::ZeroVector;

	// 낚싯대 끝점 로컬 오프셋 반환
	FVector GetLocalRodTipOffset() const;

	// ---- 미니게임 파라미터 (Rust 스타일 손맛 밸런스) ----
	// 릴 감는 속도 (m/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float ReelSpeed = 1.6f;

	// 물고기가 끌고 도망가는 속도 (m/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float FishPullSpeed = 1.0f;

	// 올바른 저항 시 릴링 장력 증가율 (초당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float TensionGainCorrect = 12.0f;

	// 잘못된 저항(같은 방향) 시 릴링 장력 증가율 (초당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float TensionGainWrong = 35.0f;

	// 릴을 안 감을 때 장력 자연 감소 속도 (초당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float TensionDecayRate = 16.0f;

	// 입질 대기 최소/최대 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Minigame")
	float MinBiteWaitTime = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Minigame")
	float MaxBiteWaitTime = 7.0f;

	// 입질 후 챔질 가능 유효 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Minigame")
	float BiteReactionWindow = 2.0f;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Fishing|Runtime")
	EFishingState CurrentState = EFishingState::Idle;

	UPROPERTY()
	TObjectPtr<AFishingBobber> SpawnedBobber;

	// 런타임 미니게임 변수
	float CurrentTension = 0.0f;       // 0 ~ 100%
	float CurrentDistance = 0.0f;      // 남은 거리 (m)
	float FishEscapeDirection = 1.0f;  // -1.0(좌) ~ +1.0(우)
	float FishTurnTimer = 0.0f;        // 물고기 방향 전환 타이머
	float PlayerPullInput = 0.0f;      // 플레이어 A(-1.0) / D(+1.0)
	bool bIsReelingInput = false;      // S키 / LMB로 릴 감는 중인지 여부
	bool bIsReelingByLMB = false;      // 마우스 좌클릭(LMB)으로 릴 감는 중인지 여부
	bool bInputInitialized = false;    // 입력 초기화 완료 여부
	bool bIsFishingActive = false;     // 낚시 세션 진행 중 (던진 후 락)
	bool bIsFinishCooldown = false;    // 낚시 종료 직후 완충 시간 (0.6초 이동 차단)

	// 타이머 핸들
	FTimerHandle BiteTimerHandle;
	FTimerHandle ReactionTimerHandle;
	FTimerHandle FinishCooldownTimerHandle;

	void OnFinishCooldownEnded();

	// 프리뷰 착수 유효성
	bool bValidWaterHit = false;
	FVector PredictedLandingLocation = FVector::ZeroVector;
};
