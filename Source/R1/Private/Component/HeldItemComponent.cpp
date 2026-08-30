/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/HeldItemComponent.h"
#include "Item/HeldItemBase.h"
#include "Data/Item/EquipmentItemData.h"
#include "Character/ActionCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"

UHeldItemComponent::UHeldItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHeldItemComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AActionCharacter>(GetOwner());

	// 에디터 디테일 패널에서 설정된 기본 아이템 데이터 자동 장착
	if (DefaultItemData)
	{
		EquipHeldItemByData(DefaultItemData);
	}
	else if (DefaultHeldItemClass)
	{
		EquipHeldItemByClass(DefaultHeldItemClass);
	}
}

AHeldItemBase* UHeldItemComponent::EquipItem(UItemDataBase* ItemData)
{
	if (!ItemData)
	{
		UnequipHeldItem();
		return nullptr;
	}

	UEquipmentItemData* EquipData = Cast<UEquipmentItemData>(ItemData);
	if (EquipData)
	{
		return EquipHeldItemByData(EquipData);
	}

	return nullptr;
}

AHeldItemBase* UHeldItemComponent::EquipHeldItemByData(UEquipmentItemData* EquipItemData)
{
	if (!EquipItemData)
	{
		UnequipHeldItem();
		return nullptr;
	}

	// 아이템 데이터에 정의된 손에 쥘 액터 클래스 스폰 및 장착
	if (EquipItemData->HeldItemClass)
	{
		CurrentEquippedItemData = EquipItemData;
		return EquipHeldItemByClass(EquipItemData->HeldItemClass);
	}

	return nullptr;
}

AHeldItemBase* UHeldItemComponent::EquipHeldItemByClass(TSubclassOf<AHeldItemBase> ItemClass)
{
	UnequipHeldItem();

	if (!OwnerCharacter || !ItemClass || !GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentHeldItem = GetWorld()->SpawnActor<AHeldItemBase>(ItemClass, OwnerCharacter->GetActorLocation(), OwnerCharacter->GetActorRotation(), SpawnParams);
	if (CurrentHeldItem)
	{
		CurrentHeldItem->SetOwner(OwnerCharacter);

		USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();
		const FName HandSocket = (CharacterMesh && CharacterMesh->DoesSocketExist(FName(TEXT("r_handSocket"))))
			? FName(TEXT("r_handSocket"))
			: ((CharacterMesh && CharacterMesh->DoesSocketExist(FName(TEXT("RightHandSocket")))) ? FName(TEXT("RightHandSocket")) : FName(NAME_None));

		if (HandSocket != NAME_None && CharacterMesh)
		{
			CurrentHeldItem->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);
		}
		else
		{
			CurrentHeldItem->AttachToActor(OwnerCharacter, FAttachmentTransformRules::KeepRelativeTransform);
			CurrentHeldItem->SetActorRelativeLocation(FVector(40.f, 30.f, 0.f));
		}

		CurrentHeldItem->OnEquipped(OwnerCharacter);

		if (OwnerCharacter && OwnerCharacter->InputComponent)
		{
			if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(OwnerCharacter->InputComponent))
			{
				CurrentHeldItem->SetupInputComponent(EIC);
			}
		}
	}

	return CurrentHeldItem;
}

void UHeldItemComponent::UnequipHeldItem()
{
	if (CurrentHeldItem)
	{
		CurrentHeldItem->OnUnequipped();
		CurrentHeldItem->Destroy();
		CurrentHeldItem = nullptr;
	}
	CurrentEquippedItemData = nullptr;
}

void UHeldItemComponent::UsePrimaryAction(bool bStarted)
{
	if (CurrentHeldItem)
	{
		if (bStarted)
		{
			CurrentHeldItem->OnPrimaryActionStarted();
		}
		else
		{
			CurrentHeldItem->OnPrimaryActionCompleted();
		}
	}
}

void UHeldItemComponent::UseSecondaryAction(bool bStarted)
{
	if (CurrentHeldItem)
	{
		if (bStarted)
		{
			CurrentHeldItem->OnSecondaryActionStarted();
		}
		else
		{
			CurrentHeldItem->OnSecondaryActionCompleted();
		}
	}
}

void UHeldItemComponent::CancelAction()
{
	if (CurrentHeldItem)
	{
		CurrentHeldItem->OnCancelAction();
	}
}

void UHeldItemComponent::OnMoveInput(const FVector2D& MoveValue)
{
	if (CurrentHeldItem)
	{
		CurrentHeldItem->OnMoveInput(MoveValue);
	}
}

bool UHeldItemComponent::BlocksCharacterMovement() const
{
	if (CurrentHeldItem)
	{
		return CurrentHeldItem->BlocksCharacterMovement();
	}
	return false;
}

bool UHeldItemComponent::BlocksDefaultAttack() const
{
	if (CurrentHeldItem)
	{
		return CurrentHeldItem->BlocksDefaultAttack();
	}
	return false;
}
