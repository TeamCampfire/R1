#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CampfireConfigDataAsset.generated.h"

class UItemDataBase;

USTRUCT(BlueprintType)
struct R1_API FCampfireCookingRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UItemDataBase> BeforeItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UItemDataBase> AfterItem;
};

USTRUCT(BlueprintType)
struct R1_API FCampfireFuelRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UItemDataBase> BeforeItem;

	// 비어 있으면 연료만 소비하고 부산물은 생성하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UItemDataBase> AfterItem;
};

/** 허용 조리 재료와 연료를 변환 전/후 아이템 쌍으로 관리하는 모닥불 설정. */
// [wdk59] DA_Campfire_Default에서 허용 요리·연료와 변환 결과, 처리 시간을 설정하는 데이터 형식이다.
UCLASS(BlueprintType)
class R1_API UCampfireConfigDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking")
	TArray<FCampfireCookingRecipe> CookingRecipes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking", meta = (ClampMin = "0.1"))
	float CookingTime = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fuel")
	TArray<FCampfireFuelRecipe> FuelRecipes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fuel", meta = (ClampMin = "0.1"))
	float BurningTime = 30.f;

	const FCampfireCookingRecipe* FindCookingRecipe(const UItemDataBase* Item) const
	{
		return CookingRecipes.FindByPredicate(
			[Item](const FCampfireCookingRecipe& Recipe) { return Recipe.BeforeItem == Item; });
	}

	const FCampfireFuelRecipe* FindFuelRecipe(const UItemDataBase* Item) const
	{
		return FuelRecipes.FindByPredicate(
			[Item](const FCampfireFuelRecipe& Recipe) { return Recipe.BeforeItem == Item; });
	}
};
