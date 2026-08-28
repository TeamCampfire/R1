// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Harvestable.generated.h"

class AActionCharacter;

USTRUCT(BlueprintType)
struct FHarvestRes
{
	GENERATED_BODY()

	// 획득할 아이템 개수
	UPROPERTY(BlueprintReadOnly)
	int32 Count = 1;

	// 떨어질 아이템 DATA
	// TODO ItemData와 연동
	UPROPERTY(BlueprintReadOnly)
	FString ItemData = TEXT("떨어질 아이템 Data");

	bool HarvesResult = false;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UHarvestable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class R1_API IHarvestable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// 자원을 획득할 수 있는 대상이 공격 받았을 때 (나무, 돌 등)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Harvest")
	FHarvestRes OnHitted(AActionCharacter* InCharacter);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Harvest")
	void SpawnImpactDecal(const FVector SpawnPoint, const FRotator SpawnRotator);

	// 대상의 체력이 0이되어서 없어질 때 호출될 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Harvest")
	void OnHarvestEnd();



};
