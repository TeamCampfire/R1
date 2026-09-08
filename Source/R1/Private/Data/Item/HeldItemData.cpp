// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/Item/HeldItemData.h"

UHeldItemData::UHeldItemData()
{
	Category = EItemCategory::HeldItem;
	MaxStackSize = 1;	// 무기/도구는 슬롯당 1개만 보관 가능 (스택 불가)
	bHasDurability = true;
	MaxDurability = 100.0f;
	FirstPersonMeshLocation = FVector(0.f, 0.f, -130.f);
	FirstPersonMeshRotation = FRotator(0.f, -90.f, 0.f);
	ItemMesh1POffset = FTransform::Identity;
	ItemMesh3POffset = FTransform::Identity;
}
