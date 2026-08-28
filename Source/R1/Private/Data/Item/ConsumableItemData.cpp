// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/Item/ConsumableItemData.h"

UConsumableItemData::UConsumableItemData()
{
	Category = EItemCategory::Consumable;
	// 아이템별로 에디터에서 필요한 최대 스택 수(예: 붕대 10, 물약 5 등)를 직접 지정하면 된다.
}
