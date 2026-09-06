/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/HeldItemBase.h"
#include "Character/ActionCharacter.h"
#include "Data/Item/HeldItemData.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

AHeldItemBase::AHeldItemBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	ItemMesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh3P"));
	RootComponent = ItemMesh3P;
	ItemMesh3P->SetOwnerNoSee(true);
	ItemMesh3P->SetCastHiddenShadow(true);

	ItemMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh1P"));
	ItemMesh1P->SetupAttachment(RootComponent);
	ItemMesh1P->SetOnlyOwnerSee(true);
	ItemMesh1P->SetCastShadow(false);
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

void AHeldItemBase::InitItemVisual(UHeldItemData* InItemData)
{
	ItemData = InItemData;
	if (!ItemData || !ItemData->WeaponMesh) return;

	if (ItemMesh1P)
	{
		ItemMesh1P->SetSkeletalMeshAsset(ItemData->WeaponMesh);
	}

	if (ItemMesh3P)
	{
		ItemMesh3P->SetSkeletalMeshAsset(ItemData->WeaponMesh);
	}
}

void AHeldItemBase::OnPrimaryActionStarted()
{
	if (!ItemData || !ItemData->PrimaryMontage || !OwnerCharacter) return;

	// 좌클릭 기본 동작은 몽타쥬 재생
	if (USkeletalMeshComponent* TPMesh = OwnerCharacter->GetMesh())
	{
		if (UAnimInstance* AnimInst = TPMesh->GetAnimInstance())
		{
			AnimInst->Montage_Play(ItemData->PrimaryMontage);
		}
	}
}

void AHeldItemBase::OnSecondaryActionStarted()
{
	if (!ItemData || !ItemData->SecondaryMontage || !OwnerCharacter) return;


	//  우클릭 기본 동작은 몽타쥬 재생
	if (USkeletalMeshComponent* TPMesh = OwnerCharacter->GetMesh())
	{
		if (UAnimInstance* AnimInst = TPMesh->GetAnimInstance())
		{
			AnimInst->Montage_Play(ItemData->SecondaryMontage);
		}
	}
}
