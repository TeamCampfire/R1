// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/Item/ItemDataBase.h"

FPrimaryAssetId UItemDataBase::GetPrimaryAssetId() const
{
	// PrimaryAssetType은 "Item" 하나로 통일한다. Equipment/Consumable/Misc를
	// 굳이 다른 PrimaryAssetType으로 나누지 않아도 Category 필드로 구분할 수 있고,
	// Asset Manager 쪽 스캔 경로/규칙(Primary Asset Types to Scan)을 하나로만
	// 관리하면 되어 설정이 단순해진다.
	return FPrimaryAssetId(TEXT("Item"), GetFName());
}

void UItemDataBase::PostLoad()
{
	Super::PostLoad();
	EnforceStackRulesForCategory();
}

#if WITH_EDITOR
void UItemDataBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	EnforceStackRulesForCategory();
}
#endif

void UItemDataBase::EnforceStackRulesForCategory()
{
	// 장비/무기/도구는 슬롯당 1개만 보관 가능(스택 불가) — Weapon/Tool은 전용 서브클래스가
	// 없어서(EquipmentItemData처럼 생성자에서 고정할 수 없음) Category 기준으로 여기서 강제한다.
	if (Category == EItemCategory::Equipment || Category == EItemCategory::Weapon || Category == EItemCategory::Tool)
	{
		MaxStackSize = 1;
	}
}
