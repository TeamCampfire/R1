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
