// 08/28 주형진

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Harvestable.generated.h"

class AActionCharacter;
class UItemDataBase;

/**
 * 자원에서 획득할 수 있는 아이템의 규칙 (기본 수량, 확률 등)
 */
USTRUCT(BlueprintType)
struct FHarvestItemYield
{
	GENERATED_BODY()

	// 획득할 아이템 데이터 (UItemDataBase DataAsset)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
	TObjectPtr<UItemDataBase> ItemData = nullptr;

	// 1회 타격당 기본 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest", meta = (ClampMin = "1"))
	int32 BaseCount = 1;

	// 드랍 확률 (0.0 ~ 1.0, 1.0은 100% 확정 드랍)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;
};

/**
 * 1회 채집 시 획득된 개별 아이템 결과
 */
USTRUCT(BlueprintType)
struct FHarvestItemResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Harvest")
	TObjectPtr<UItemDataBase> ItemData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Harvest")
	int32 Count = 0;
};

/**
 * 채집(OnHitted) 수행 결과
 */
USTRUCT(BlueprintType)
struct FHarvestRes
{
	GENERATED_BODY()

	// 획득된 아이템 목록 (다중 아이템 지원)
	UPROPERTY(BlueprintReadOnly, Category = "Harvest")
	TArray<FHarvestItemResult> HarvestedItems;

	// 채집 성공 여부
	UPROPERTY(BlueprintReadOnly, Category = "Harvest")
	bool HarvesResult = false;

	// 스위트 스팟 적중 여부
	UPROPERTY(BlueprintReadOnly, Category = "Harvest")
	bool bHitSweetSpot = false;

	// 자원 고갈 (마지막 타격) 여부
	UPROPERTY(BlueprintReadOnly, Category = "Harvest")
	bool bIsDepleted = false;
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
	FHarvestRes OnHitted(AActionCharacter* InCharacter, const FVector& HitLocation);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Harvest")
	void SpawnImpactDecal(const FVector SpawnPoint, const FRotator SpawnRotator);

	// 대상의 체력이 0이되어서 없어질 때 호출될 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Harvest")
	void OnHarvestEnd();



};
