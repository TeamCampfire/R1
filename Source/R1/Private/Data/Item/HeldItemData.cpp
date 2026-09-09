// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/Item/HeldItemData.h"

UHeldItemData::UHeldItemData()
{
	Category = EItemCategory::HeldItem;
	MaxStackSize = 1;	// 무기/도구는 슬롯당 1개만 보관 가능 (스택 불가) — 기본값일 뿐, bAllowStacking=true인
						// 아이템(붕대 등)은 에디터에서 이보다 큰 값을 지정하면 그대로 유지된다.
	bHasDurability = true;
	MaxDurability = 100.0f;
}

void UHeldItemData::EnforceStackRulesForCategory()
{
	if (bAllowStacking)
	{
		// 붕대처럼 명시적으로 스택을 허용한 HeldItem은 베이스의 "HeldItem은 무조건 1" 강제를 건너뛴다.
		return;
	}

	Super::EnforceStackRulesForCategory();
}
