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

#include "Net/UnrealNetwork.h"

void AHeldItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeldItemBase, ItemData);
}

void AHeldItemBase::OnRep_ItemData()
{
	if (ItemData)
	{
		InitItemVisual(ItemData);
	}
}

void AHeldItemBase::OnPrimaryActionStarted()
{
	//UE_LOG(LogTemp, Display, TEXT("[AHeldItemBase::OnPrimaryActionStarted] Called. ItemData=%s, Montage=%s"), 
	//	ItemData ? *ItemData->GetName() : TEXT("NULL"), 
	//	(ItemData && ItemData->PrimaryMontage) ? *ItemData->PrimaryMontage->GetName() : TEXT("NULL"));

	if (!ItemData || !ItemData->PrimaryMontage || !OwnerCharacter) return;

	// 1) 3인칭 전신 몽타주 로컬 선행 재생 (노티파이 발생 및 레이캐스트 공격 트리거)
	if (UAnimInstance* AnimInst3P = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
	{
		if (!AnimInst3P->IsAnyMontagePlaying())
		{
			OwnerCharacter->PlayAnimMontage(ItemData->PrimaryMontage);

			// 2) 멀티플레이어 동기화 (서버 및 다른 클라이언트)
			if (!HasAuthority())
			{
				Server_PlayPrimaryActionMontage();
			}
			else
			{
				Multicast_PlayPrimaryActionMontage();
			}
		}
	}
}

void AHeldItemBase::Server_PlayPrimaryActionMontage_Implementation()
{
	Multicast_PlayPrimaryActionMontage();
}

void AHeldItemBase::Multicast_PlayPrimaryActionMontage_Implementation()
{
	if (!ItemData || !ItemData->PrimaryMontage || !OwnerCharacter) return;

	// 로컬 컨트롤러는 이미 선행 재생했으므로 중복 방지
	if (OwnerCharacter->IsLocallyControlled()) return;

	OwnerCharacter->PlayAnimMontage(ItemData->PrimaryMontage);
}

void AHeldItemBase::OnSecondaryActionStarted()
{
	if (!ItemData || !ItemData->SecondaryMontage || !OwnerCharacter) return;

	// 1) 3인칭 전신 몽타주 선행 재생
	if (UAnimInstance* AnimInst3P = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
	{
		if (!AnimInst3P->IsAnyMontagePlaying())
		{
			OwnerCharacter->PlayAnimMontage(ItemData->SecondaryMontage);

			if (!HasAuthority())
			{
				Server_PlaySecondaryActionMontage();
			}
			else
			{
				Multicast_PlaySecondaryActionMontage();
			}
		}
	}
}

void AHeldItemBase::Server_PlaySecondaryActionMontage_Implementation()
{
	Multicast_PlaySecondaryActionMontage();
}

void AHeldItemBase::Multicast_PlaySecondaryActionMontage_Implementation()
{
	if (!ItemData || !ItemData->SecondaryMontage || !OwnerCharacter) return;

	if (OwnerCharacter->IsLocallyControlled()) return;

	OwnerCharacter->PlayAnimMontage(ItemData->SecondaryMontage);
}

