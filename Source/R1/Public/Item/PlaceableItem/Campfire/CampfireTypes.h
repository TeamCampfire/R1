#pragma once

#include "CoreMinimal.h"
#include "CampfireTypes.generated.h"

UENUM(BlueprintType)
enum class ECampfireSlotType : uint8
{
	Input,
	Fuel,
	Output
};

USTRUCT(BlueprintType)
struct FCampfireSlotRef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Campfire")
	ECampfireSlotType Type = ECampfireSlotType::Input;

	UPROPERTY(BlueprintReadWrite, Category = "Campfire")
	int32 Index = INDEX_NONE;

	bool IsValid() const { return Index != INDEX_NONE; }
};

UENUM(BlueprintType)
enum class EItemDragSourceType : uint8
{
	PlayerInventory,
	Campfire
};
