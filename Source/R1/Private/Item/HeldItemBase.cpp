/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/HeldItemBase.h"
#include "Character/ActionCharacter.h"

AHeldItemBase::AHeldItemBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	if (ItemMesh1P)
	{
		ItemMesh1P->SetOnlyOwnerSee(true);
		ItemMesh1P->SetCastShadow(false);
	}

	if (ItemMesh3P)
	{
		ItemMesh3P->SetOwnerNoSee(true);
		ItemMesh3P->SetCastHiddenShadow(true);
	}
}

void AHeldItemBase::BeginPlay()
{
	Super::BeginPlay();
}

void AHeldItemBase::OnEquipped(AActionCharacter* InCharacter)
{
	OwnerCharacter = InCharacter;
	SetOwner(InCharacter);
}

void AHeldItemBase::OnUnequipped()
{
	OwnerCharacter = nullptr;
	SetOwner(nullptr);
}
