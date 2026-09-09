#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CraftingRecipeCatalog.generated.h"

class UItemDataBase;

// 모든 제작 컴포넌트가 공유하는 허용 목록. 재료와 제작 조건은 아이템 데이터에 둔다.
UCLASS(BlueprintType)
class R1_API UCraftingRecipeCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	TArray<TObjectPtr<UItemDataBase>> Recipes;
};
