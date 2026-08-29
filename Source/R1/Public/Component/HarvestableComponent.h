// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/Harvestable.h"
#include "HarvestableComponent.generated.h"

/*-----------------------------------------

	자원을 주는 액터가 가질 액터 컴포넌트

-----------------------------------------*/

class UNiagaraSystem;
class USoundBase;
class AItemPickup;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class R1_API UHarvestableComponent : public UActorComponent, public IHarvestable
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UHarvestableComponent();

	// 자원을 획득할 수 있는 대상이 공격 받았을 때 (나무, 돌 등)
	virtual FHarvestRes OnHitted_Implementation(AActionCharacter* InCharacter, const FVector& HitLocation) override;
	// 대상의 체력이 0이되어서 없어질 때 호출될 함수
	virtual void		OnHarvestEnd_Implementation() override;
	virtual void		SpawnImpactDecal_Implementation(const FVector SpawnPoint, const FRotator SpawnRotator) override;

protected:
	// Called when the game starts
	virtual void		BeginPlay() override;
	// Called every frame
	virtual void		TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 맞추면 자원을 더 획득하는 구건을 생성(나무는 빨간 X자 스프레이)
	void				GenerateSweetSpot();

protected:
	// 채집 시 획득할 아이템 목록 (다중 아이템 및 확률 지원)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Item")
	TArray<FHarvestItemYield> HarvestYields;

	// 고갈(파괴) 시 아이템을 인벤토리로 직접 주지 않고 월드 바닥에 AItemPickup 액터를 스폰하여 드랍할지 여부 (드럼통, 상자 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Item")
	bool bDropItemsInWorldOnDepleted = false;

	// 월드 드랍 시 스폰할 ItemPickup 액터 클래스 (기본: AItemPickup)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Item", meta = (EditCondition = "bDropItemsInWorldOnDepleted"))
	TSubclassOf<AItemPickup> ItemPickupClass;

	// 월드 드랍 시 퍼지는 랜덤 반경 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Item", meta = (EditCondition = "bDropItemsInWorldOnDepleted"))
	float DropImpulseRadius = 60.0f;

	//TOOD Data 기반으로 초기화
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Stats")
	int32 MaxHp = 100.f;

	//TODO Data 기반으로 초기화
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Stats")
	int32 CurrentHp = 100.f;

	// Sweet Spot 사용 여부 (false일 경우 시체나 단순 자원처럼 스위트스팟 미니게임 없이 채집)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	bool bUseSweetSpot = true;

	// Sweet Spot 적중시 곱해질 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	float BounusRate = 1.5f;

	// Sweet Spot 적중 판정 반경 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	float SweetSpotHitRadius = 15.0f;

	// Sweet Spot 생성 최소/최대 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	float SweetSpotMinHeight = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	float SweetSpotMaxHeight = 120.0f;

	// Sweet Spot 재배치 시 상하 랜덤 변위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	float SweetSpotHeightDeltaRange = 15.0f;

	// Sweet Spot 재배치 시 좌우 랜덤 각도 변위 (Yaw +- deg)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	float SweetSpotAngleDeltaRange = 22.5f;

	// Sweet Spot 라인트레이스 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	float SweetSpotTraceDistance = 1000.0f;

	// SweetSpot Decal 크기 및 수명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	FVector SweetSpotDecalSize = FVector(10.0f, 10.0f, 10.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	float SweetSpotDecalLifeSpan = 60.0f;

	// SweetSpot Decal 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|SweetSpot")
	TObjectPtr<UMaterial> SweetSpotDecal;

	// 공격 받았을때 소환할 데칼
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Decal")
	TArray<TObjectPtr<UMaterial>> ImpactDecals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Decal")
	FVector ImpactDecalSize = FVector(10.0f, 10.0f, 10.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Decal")
	float ImpactDecalLifeSpan = 60.0f;

	// ---- 사운드 및 이펙트 (FX / Audio) ----
	// 일반 타격 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|FX")
	TObjectPtr<USoundBase> HitSound;

	// 스위트 스팟 적중 시 특수 사운드 (X자 타격음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|FX")
	TObjectPtr<USoundBase> SweetSpotHitSound;

	// 일반 타격 나이아가라 이펙트 (나무 조각, 돌가루, 피 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|FX")
	TObjectPtr<UNiagaraSystem> HitNiagaraFX;

	// 스위트 스팟 적중 시 나이아가라 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|FX")
	TObjectPtr<UNiagaraSystem> SweetSpotNiagaraFX;

	// ---- 고갈 마무리 보너스 (Final Finish Bonus) ----
	// 자원 고갈(마지막 타격) 시 보너스 수확 지급 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Bonus")
	bool bGiveFinalBonus = true;

	// 고갈 시 추가로 지급할 별도 보너스 아이템 목록 (비어있을 경우 기본 HarvestYields에 FinalBonusMultiplier가 적용됨)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Bonus")
	TArray<FHarvestItemYield> FinalBonusYields;

	// FinalBonusYields가 비어있을 때 기본 채집량에 적용할 고갈 보너스 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvestable|Bonus")
	float FinalBonusMultiplier = 2.0f;

private:
	// 피격 피드백(사운드/FX/데칼) 처리 및 스위트스팟 배율 계산
	float ProcessHitFeedback(AActionCharacter* InCharacter, const FVector& HitLocation, bool& bOutHitSweetSpot);

	// 아이템 수확 계산 헬퍼 함수
	void CollectYieldItems(const TArray<FHarvestItemYield>& InYields, float Multiplier, TArray<FHarvestItemResult>& OutResults);

	// 자원 고갈 및 마무리 보너스 처리 헬퍼 함수
	void ProcessDepletion(TArray<FHarvestItemResult>& InOutResults);

	// 월드 바닥에 AItemPickup 액터 스폰
	void SpawnWorldPickups(const TArray<FHarvestItemResult>& ItemsToSpawn);

private:
	// 실제 생성된 SweetSpotDecal
	UPROPERTY()
	TObjectPtr<UDecalComponent> CurrentSweetSpotDecal;
};
