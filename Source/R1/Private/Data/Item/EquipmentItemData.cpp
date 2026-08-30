// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/Item/EquipmentItemData.h"

UEquipmentItemData::UEquipmentItemData()
{
	Category = EItemCategory::Equipment;
	MaxStackSize = 1;	// 장비는 슬롯당 1개만 보관 가능 (스택 불가)
	bHasDurability = true;
	MaxDurability = 100.0f;
}
