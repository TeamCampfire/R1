/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/HeldItemBase.h"
#include "Character/ActionCharacter.h"
#include "Data/Item/HeldItemData.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

AHeldItemBase::AHeldItemBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);

	ItemMesh3P = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh3P"));
	RootComponent = ItemMesh3P;
	ItemMesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh3P->SetCollisionResponseToAllChannels(ECR_Ignore);
	ItemMesh3P->SetOwnerNoSee(true);
	ItemMesh3P->SetCastHiddenShadow(true);

	ItemMesh1P = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh1P"));
	ItemMesh1P->SetupAttachment(RootComponent);
	ItemMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
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
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AHeldItemBase::InitItemVisual] InItemData is NULL!"));
		return;
	}

	if (!ItemData->WeaponMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AHeldItemBase::InitItemVisual] WeaponMesh is NULL in ItemData: %s"), *ItemData->GetName());
		return;
	}

	if (ItemMesh1P)
	{
		ItemMesh1P->SetStaticMesh(ItemData->WeaponMesh);
		ItemMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemMesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (ItemMesh3P)
	{
		ItemMesh3P->SetStaticMesh(ItemData->WeaponMesh);
		ItemMesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemMesh3P->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	UE_LOG(LogTemp, Log, TEXT("[AHeldItemBase::InitItemVisual] Successfully set WeaponMesh: %s"), *ItemData->WeaponMesh->GetName());
}

void AHeldItemBase::OnPrimaryActionStarted()
{
	if (!ItemData || !ItemData->PrimaryMontage || !OwnerCharacter) return;

	// 좌클릭 기본 동작은 몽타쥬 재생
	if (USkeletalMeshComponent* TPMesh = OwnerCharacter->GetMesh())
	{
		if (UAnimInstance* AnimInst = TPMesh->GetAnimInstance())
		{
			if (!AnimInst->IsAnyMontagePlaying()) return;
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
