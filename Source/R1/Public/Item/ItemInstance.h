/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 인벤토리 슬롯 하나에 실제로 담기는 "런타임 인스턴스

#pragma once

#include "CoreMinimal.h"
#include "Data/Item/ItemDataBase.h"
#include "ItemInstance.generated.h"

/**
 * 인벤토리 슬롯 하나에 실제로 담기는 "런타임 인스턴스".
 *
 * UItemDataBase(및 서브클래스)가 "정의"(항상 동일)라면, 이 구조체는 그 정의를
 * 참조하면서 슬롯/개체마다 달라지는 값을 들고 있는 쪽이다.
 *
 */

USTRUCT(BlueprintType)
struct FItemInstance
{
	GENERATED_BODY()

public:
	// 이 인스턴스가 참조하는 아이템 정의.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UItemDataBase> ItemData = nullptr;

	// 슬롯 이동/드랍/재획득 등에서 이 개체를 추적하기 위한 고유 ID.
	// 스택 합치기 시에는 ID가 아니라 ItemData(아이템 종류) 동일 여부로 판단한다.
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FGuid InstanceID;

	// 스택형 아이템(소비/기타)의 현재 수량. 장비 아이템은 항상 1로 취급한다.
	UPROPERTY(BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0"))
	int32 StackCount = 1;

	FItemInstance() = default;

	explicit FItemInstance(UItemDataBase* InItemData, int32 InStackCount = 1)
		: ItemData(InItemData)
		, InstanceID(FGuid::NewGuid())
		, StackCount(InStackCount)
	{
	}

	inline bool IsValid() const { return ItemData != nullptr && StackCount > 0; }
};