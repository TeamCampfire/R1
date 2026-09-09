#pragma once

#include "CoreMinimal.h"
#include "CampfireTypes.generated.h"

// 모닥불 슬롯 종류
UENUM(BlueprintType)
enum class ECampfireSlotType : uint8
{
	Input,
	Fuel,
	Output
};

// 연료, 입력, 출력 종류와 인덱스로 모닥불 슬롯을 식별하여 UI와 서버 이동 요청에 공유
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

// 아이템 드래그 이동 시 아이템 출처가 플레이어의 인벤토리인지 모닥불인지 구분
UENUM(BlueprintType)
enum class EItemDragSourceType : uint8
{
	PlayerInventory,
	Campfire
};
