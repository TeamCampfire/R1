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
	// 장비/무기·도구/설치물은 슬롯당 1개만 보관 가능(스택 불가) — 설치물(모닥불/제작대
	// 등)도 러스트 기준 스택되지 않는다. 각 서브클래스 생성자에서도 직접 고정하지만,
	// Category만 바뀌고 클래스가 다른 실수 등을 막기 위한 안전망.
	if (Category == EItemCategory::Equipment || Category == EItemCategory::HeldItem || Category == EItemCategory::Placeable)
	{
		MaxStackSize = 1;
	}
}
