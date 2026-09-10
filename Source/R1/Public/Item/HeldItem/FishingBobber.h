/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishingBobber.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class USoundBase;
class AWaterBody;

/**
 * 낚시 찌(Bobber) 액터
 * 캐스팅 시 포물선으로 비행하며, 수면에 닿으면 WaterBody의 수면 높이를 추적하여 부유합니다.
 */
UCLASS()
class R1_API AFishingBobber : public AActor
{
	GENERATED_BODY()
	
public:	
	AFishingBobber();

	virtual void Tick(float DeltaTime) override;

	// 캐스팅 발사 (방향 및 속도, 선택적 예상 착수 Z 높이)
	void LaunchBobber(const FVector& LaunchVelocity, float InExpectedWaterZ = -999999.0f);

	// 예상 수면 착수 높이 설정
	void SetExpectedWaterZ(float InWaterZ)
	{
		ExpectedWaterZ = InWaterZ;
		bHasExpectedWaterZ = true;
	}

	// 물고기 입질 시작/종료 설정
	void SetBiting(bool bBiting);

	// 현재 수면에 안착했는지 여부
	FORCEINLINE bool IsInWater() const { return bIsInWater; }

	// 수면 기준 높이 반환
	FORCEINLINE float GetBaseWaterZ() const { return BaseWaterZ; }

	// 찌가 물을 탈출하여 육지/수변에 도달했는지 판별
	bool CheckHasEscapedWater() const;

	// 연결된 낚싯대 소유자 설정
	void SetOwnerRod(class AFishingRod* InRod) { OwnerRod = InRod; }

	// 미니게임 도망 방향 및 장력 시각적 피드백 수신
	void SetFishPullFeedback(float InDirection, float InTensionPercent, bool bInCorrectResist);

protected:
	virtual void BeginPlay() override;

	// 충돌(Hit) 및 오버랩(Overlap) 수면 감지
	UFUNCTION()
	void OnBobberHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnBobberOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 수면에 성공적으로 안착했을 때 호출
	void OnEnterWater(AWaterBody* WaterBody, const FVector& SurfaceLocation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BobberMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// 수면 파문/착수 나이아가라 FX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|FX")
	TObjectPtr<UNiagaraSystem> WaterSplashFX;

	// FishingBobber.h
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Audio")
	TObjectPtr<USoundBase> BobberLandSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Audio")
	TObjectPtr<USoundBase> FishBiteSound;

	// 입질 시 찌가 들어가는 깊이 (cm): 15cm -> 28cm로 시각적으로 확실히 물속에 잠기게 함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Tuning")
	float BiteSubmergeDepth = 28.0f;

	// 찰랑거리는 잔물결 오르내림 진폭 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Tuning")
	float BobbingAmplitude = 2.5f;

	// 찰랑거림 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Tuning")
	float BobbingSpeed = 3.5f;


private:
	UPROPERTY()
	TObjectPtr<class AFishingRod> OwnerRod;

	UPROPERTY()
	TObjectPtr<AWaterBody> CachedWaterBody;

	bool bIsInWater = false;
	bool bIsBiting = false;
	bool bHasExpectedWaterZ = false;
	float ExpectedWaterZ = 0.0f;
	float RunningTime = 0.0f;
	float CurrentSubmergeOffset = 0.0f;
	float BaseWaterZ = 0.0f;
	FVector PrevTickLocation = FVector::ZeroVector;

	// 미니게임 피드백용 런타임 변수
	float CurrentFishDirection = 0.0f;
	float CurrentTensionPercent = 0.0f;
	bool bCorrectResistFeedback = true;
	float CurrentRollTilt = 0.0f;
};
