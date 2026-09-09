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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 도구 공통 액션 오버라이드
	virtual void OnPrimaryActionStarted() override;
	virtual void OnPrimaryActionCompleted() override;
	virtual void OnSecondaryActionStarted() override { StartAim(); }
	virtual void OnSecondaryActionCompleted() override { StopAim(); }
	virtual void OnCancelAction() override { Input_Cancel(); }

	// 캐릭터 이동 입력 중계 (낚시 중 A/D 저항, S 릴 감기)
	virtual void OnMoveInput(const FVector2D& MoveValue) override;

	virtual void SetupInputComponent(class UEnhancedInputComponent* PlayerEIC) override;

	virtual bool BlocksCharacterMovement() const override { return IsFishingActive() || IsFinishCooldown(); }


	// 찌가 수면에 닿았을 때 찌 액터가 호출
	void OnBobberLandedInWater();

	// ---- 입력 핸들러 (캐릭터 또는 입력 컴포넌트에서 호출) ----
	UFUNCTION(BlueprintCallable, Category = "Fishing|Input")
	void StartAim();

	UFUNCTION(BlueprintCallable, Category = "Fishing|Input")
	void StopAim();

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
	//BlueprintPure는 실행 핀 없이 값을 가져올 수 있다.
	UFUNCTION(BlueprintPure, Category = "Fishing|State")
	FORCEINLINE EFishingState GetFishingState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Fishing|State")
	FORCEINLINE float GetTensionPercent() const { return FMath::Clamp(CurrentTension / 100.0f, 0.0f, 1.0f); }

	UFUNCTION(BlueprintPure, Category = "Fishing|State")
	FORCEINLINE float GetRemainingDistance() const { return CurrentDistance; }

	UFUNCTION(BlueprintPure, Category = "Fishing|State")
	FORCEINLINE bool IsFishingActive() const { return bIsFishingActive; }

	UFUNCTION(BlueprintPure, Category = "Fishing|State")
	FORCEINLINE bool IsFinishCooldown() const { return bIsFinishCooldown; }

	UFUNCTION(BlueprintPure, Category = "Fishing|Animation")
	FORCEINLINE float GetPlayerPullInput() const { return AnimPullInput; }

	// 감쇄되지 않은 순수 A/D 플레이어 저항 입력값 (-1.0 ~ +1.0)
	UFUNCTION(BlueprintPure, Category = "Fishing|Animation")
	FORCEINLINE float GetRawPlayerPullInput() const { return PlayerPullInput; }

	UFUNCTION(BlueprintPure, Category = "Fishing|Animation")
	FORCEINLINE bool IsReelingInput() const { return bIsReelingInput || bIsReelingByLMB; }

	// 낚싯대 끝 위치 반환 (소켓이 없어도 안전)
	FVector GetRodTipLocation() const;

protected:
	virtual void BeginPlay() override;

	// 캐스팅 궤적 연산 및 프리뷰
	void UpdateCastingTrajectory(float DeltaTime);

	// 물리 포물선 초기 발사 속도 및 비행 시간 정밀 계산
	bool CalculateCastVelocity(const FVector& StartPos, const FVector& TargetPos, FVector& OutVelocity, float& OutFlightTime) const;

	// 미니게임 메인 틱 (장력 및 거리 연산)
	void UpdateMinigame(float DeltaTime);

	// 물고기 입질 시작
	void TriggerBite();

	// 입질 시간 초과 (놓침)
	void OnBiteMissed();

	// 낚시 종료 처리 (성공/실패, 선택적 보상 아이템 데이터 및 수량 오버라이드)
	void FinishFishing(bool bSuccess, UItemDataBase* OptionalRewardItem = nullptr, int32 OptionalRewardCount = 1);

	// 낚시 상태 전환 및 서버 동기화
	void SetFishingState(EFishingState NewState);

	// 서버 권한 보상 지급 Server RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FinishFishing(bool bSuccess, UItemDataBase* InRewardItem = nullptr, int32 InCount = 1);

	// 서버 상태 동기화 Server RPC
	UFUNCTION(Server, Reliable)
	void Server_SetFishingState(EFishingState NewState);

	// 미니게임 입력 서버 동기화 (애니메이션 동기화용)
	UFUNCTION(Server, Reliable)
	void Server_SetPull(float PullAxis);

	UFUNCTION(Server, Reliable)
	void Server_SetReeling(bool bReeling);

	// 원격 머신 시각적 캐스팅 및 찌/케이블 생성
	void StartCast_Visuals(const FVector& TipLoc, const FVector& TargetLoc, const FVector& LaunchVelocity);

	// 캐스팅 시작 Server RPC 및 Multicast RPC
	UFUNCTION(Server, Reliable)
	void Server_StartCast(const FVector& TipLoc, const FVector& TargetLoc, const FVector& LaunchVelocity);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartCast(const FVector& TipLoc, const FVector& TargetLoc, const FVector& LaunchVelocity);

	// 미니게임 중 찌 위치 동기화 RPC (20 FPS)
	UFUNCTION(Server, Unreliable)
	void Server_UpdateBobberLocation(const FVector_NetQuantize& InLocation);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_UpdateBobberLocation(const FVector_NetQuantize& InLocation);

	// 낚시 종료 시 모든 머신에서 찌 파괴 및 케이블 숨김
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_EndFishing();

	// 낚시 종료(성공/실패) 세레머니 연출 멀티캐스트
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayEndState(bool bSuccess);

	// 모든 낚시 상태 및 스폰된 액터 초기화
	void ResetFishing();

	// 입력 컨텍스트 전환 및 지연 초기화 안전장치
	void TryInitializeInputs();
	void PushFishingInputContext();
	void PopFishingInputContext();

protected:

	// 낚싯대 끝 ➔ 찌를 잇는 낚싯줄 케이블
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCableComponent> FishingLineCable;

	// 찌 액터 클래스 (AFishingBobber)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Classes")
	TSubclassOf<AFishingBobber> BobberClass;

	// 낚시 성공 시 지급할 보상 아이템 DataAsset (에디터 및 블루프린트에서 변경 가능한 변수)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Reward")
	TObjectPtr<UItemDataBase> FishRewardItemData;

	// 낚시 성공 시 지급할 보상 아이템 수량 (변수)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Reward")
	int32 FishRewardCount = 1;

	// 보상 아이템 데이터 및 수량을 외부에서 동적으로 주입/설정하는 함수
	UFUNCTION(BlueprintCallable, Category = "Fishing|Reward")
	void SetFishRewardItemData(UItemDataBase* NewRewardItem, int32 NewCount = 1)
	{
		FishRewardItemData = NewRewardItem;
		FishRewardCount = FMath::Max(1, NewCount);
	}

	UFUNCTION(BlueprintPure, Category = "Fishing|Reward")
	UItemDataBase* GetFishRewardItemData() const { return FishRewardItemData; }

	UFUNCTION(BlueprintPure, Category = "Fishing|Reward")
	int32 GetFishRewardCount() const { return FishRewardCount; }

	// 기본 매핑 컨텍스트 (IMC_Default: 조준 해제 시 복구용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Input")
	TObjectPtr<UInputMappingContext> IMC_Default;

	// 낚시 전용 입력 매핑 컨텍스트 (IMC_Fishing)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Input")
	TObjectPtr<UInputMappingContext> IMC_Fishing;

	// 낚시 전용 입력 액션들 (낚싯대 자체 소유)
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
	float MaxCastDistance = 2000.0f; // 최대 캐스팅 가능 거리 (20m)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Casting")
	float MinCastDistance = 200.0f;  // 최소 캐스팅 거리 (2m)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Casting")
	float MinCatchDistance = 200.0f;  // 플레이어 앞 찌 최소 접근 거리 한계 (2.0m 도달 시 성공)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Casting")
	float MinCastPower = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Casting")
	float MaxCastPower = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Casting")
	FName RodTipSocketName = TEXT("FishRod_End");

	// 소켓이 없거나 미세조정이 필요할 때 적용할 로컬 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Casting")
	FVector CustomRodTipOffset = FVector::ZeroVector;

	// 낚싯대 끝점 로컬 오프셋 반환
	FVector GetLocalRodTipOffset() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentState();

	// ---- 미니게임 파라미터 (여유롭고 손맛 위주의 캐주얼 밸런스) ----
	// 릴 감는 속도 (m/s): 제압 성공 시 시원하게 당겨지는 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float ReelSpeed = 1.6f;

	// 물고기가 끌고 도망가는 속도 (m/s): A/D 저항이 없을 때 물고기가 줄을 끌고 나가는 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float FishPullSpeed = 1.2f;

	// 올바른 저항(중앙 정렬) 시 릴링 장력 증가율 (초당 8%): 약 10초간 안정적으로 릴링 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float TensionGainCorrect = 8.0f;

	// 중앙 정렬 없이 S만 누르거나 잘못된 방향일 때 릴링 장력 증가율 (초당 48%): 2초 만에 100% 도달하여 줄 끊어짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float TensionGainWrong = 48.0f;

	// 릴을 안 감을 때 장력 자연 감소 속도 (초당 35%): 손 떼면 즉시 안정화
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float TensionDecayRate = 35.0f;

	// 최대 허용 좌우 편차 (cm): 이 거리 이상 벗어나면 중심 복귀 필요
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float MaxLateralOffset = 350.0f;

	// 플레이어의 A/D 당기기 수평 이동 속도 (cm/s): 물고기를 힘겹게 끌어오는 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float PlayerPullPower = 340.0f;

	// 물고기가 스스로 좌우로 도망치는 수평 헤엄 속도 (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Minigame")
	float FishEscapeSpeed = 220.0f;

	// 입질 대기 최소/최대 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Minigame")
	float MinBiteWaitTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Minigame")
	float MaxBiteWaitTime = 5.0f;

	// 입질 후 챔질 가능 유효 시간 (초): 2.0초 -> 4.5초로 대폭 늘려 여유 부여
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Minigame")
	float BiteReactionWindow = 4.5f;

	// 릴링(감기) 시 좌우 기울기(저항) 감쇄 배율 (0.0: 완전 정면, 0.25: 25%만 약하게 기울임, 1.0: 감쇄 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Animation")
	float ReelingTiltDamping = 0.25f;

	// 좌우 기울기 전환 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Animation")
	float TiltInterpSpeed = 8.0f;

	// 낚시 성공 세레머니 지속 시간 (초) - 성공 애니메이션(catch_success) 완주 대기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Tuning")
	float SuccessCeremonyDuration = 2.6f;

	// 낚시 실패/취소 쿨다운 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fishing|Tuning")
	float FailureDuration = 1.0f;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, BlueprintReadOnly, Category = "Fishing|Runtime")
	EFishingState CurrentState = EFishingState::Idle;

	UPROPERTY()
	TObjectPtr<AFishingBobber> SpawnedBobber;

	// 런타임 미니게임 변수
	float CurrentTension = 0.0f;       // 0 ~ 100%
	float CurrentDistance = 0.0f;      // 남은 거리 (m)
	float CurrentLateralOffset = 0.0f; // 중심선 기준 좌우 편차 (-350cm ~ +350cm)
	float FishEscapeDirection = 1.0f;  // -1.0(좌) ~ +1.0(우)
	float FishTurnTimer = 0.0f;        // 물고기 방향 전환 타이머
	float PlayerPullInput = 0.0f;      // 플레이어 A(-1.0) / D(+1.0)

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Fishing|Runtime")
	float AnimPullInput = 0.0f;        // 애니메이션용 보간 및 감쇄된 좌우 기울기 (-1.0 ~ +1.0)

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Fishing|Runtime")
	bool bIsReelingInput = false;      // S키 / LMB로 릴 감는 중인지 여부

	bool bIsReelingByLMB = false;      // 마우스 좌클릭(LMB)으로 릴 감는 중인지 여부
	bool bInputInitialized = false;    // 입력 초기화 완료 여부

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Fishing|Runtime")
	bool bIsFishingActive = false;     // 낚시 세션 진행 중 (던진 후 락)

	bool bIsFinishCooldown = false;    // 낚시 종료 직후 완충 시간 (이동 차단)
	float BobberSyncTimer = 0.0f;      // 찌 위치 네트워크 동기화 타이머

	// 타이머 핸들
	FTimerHandle BiteTimerHandle;
	FTimerHandle ReactionTimerHandle;
	FTimerHandle FinishCooldownTimerHandle;

	void OnFinishCeremonyEnded();

	// 프리뷰 착수 유효성
	bool bValidWaterHit = false;
	FVector PredictedLandingLocation = FVector::ZeroVector;
};
