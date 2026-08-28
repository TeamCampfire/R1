// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/Harvestable.h"
#include "HarvestableComponent.generated.h"

/*-----------------------------------------

	자원을 주는 액터가 가질 액터 컴포넌트

-----------------------------------------*/

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
	//TODO Data 기반으로 초기화
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ItemData = TEXT("나무");

	//TOOD Data 기반으로 초기화
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHp = 100.f;

	//TODO Data 기반으로 초기화
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentHp = 100.f;

	// Sweet Spot 적중시 곱해질 배율
	UPROPERTY(BlueprintReadOnly)
	float BounusRate = 1.5f;

	// 공격 받았을때 소환할 데칼
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UMaterial>> ImpactDecals;

	// SweetSpot Decal
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterial> SweetSpotDecal;

private:
	// 실제 생성된 SweetSpotDecal
	UPROPERTY()
	TObjectPtr<UDecalComponent> CurrentSweetSpotDecal;
};
