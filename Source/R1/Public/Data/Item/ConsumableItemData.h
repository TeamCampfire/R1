/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 소비아이템 정의 클래스


// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Item/ItemDataBase.h"
#include "ConsumableItemData.generated.h"

/**
 * 소비 아이템 정의. 장비 슬롯에 착용 불가하며, 사용 시 Effects에 정의된 효과를
 * 순서대로 적용한다. 효과 종류가 늘어나도 EItemEffectType(ItemTypes.h)에 값만
 * 추가하면 되고 이 클래스 구조는 그대로 둔다.
 */
UCLASS()
class R1_API UConsumableItemData : public UItemDataBase
{
	GENERATED_BODY()
	
public:
	UConsumableItemData();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	TArray<FItemEffect> Effects;
};
