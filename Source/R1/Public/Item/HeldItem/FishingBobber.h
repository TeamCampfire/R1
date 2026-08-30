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

	// 캐스팅 발사 (방향 및 속도)
	void LaunchBobber(const FVector& LaunchVelocity);

	// 물고기 입질 시작/종료 설정
	void SetBiting(bool bBiting);

	// 현재 수면에 안착했는지 여부
	FORCEINLINE bool IsInWater() const { return bIsInWater; }

	// 연결된 낚싯대 소유자 설정
	void SetOwnerRod(class AFishingRod* InRod) { OwnerRod = InRod; }

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

	// 착수 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Audio")
	TObjectPtr<USoundBase> WaterSplashSound;

	// 입질 시 찌가 들어가는 깊이 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Tuning")
	float BiteSubmergeDepth = 15.0f;

	// 찰랑거리는 잔물결 오르내림 진폭 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Tuning")
	float BobbingAmplitude = 2.0f;

	// 찰랑거림 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Tuning")
	float BobbingSpeed = 3.0f;

private:
	UPROPERTY()
	TObjectPtr<class AFishingRod> OwnerRod;

	UPROPERTY()
	TObjectPtr<AWaterBody> CachedWaterBody;

	bool bIsInWater = false;
	bool bIsBiting = false;
	float RunningTime = 0.0f;
	float CurrentSubmergeOffset = 0.0f;
	float BaseWaterZ = 0.0f;
};
