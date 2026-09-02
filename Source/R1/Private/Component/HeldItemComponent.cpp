/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/HeldItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Item/HeldItemBase.h"
#include "Data/Item/EquipmentItemData.h"
#include "Character/ActionCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "UObject/ConstructorHelpers.h"

UHeldItemComponent::UHeldItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHeldItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHeldItemComponent, CurrentHeldItem);
	DOREPLIFETIME(UHeldItemComponent, CurrentEquippedItemData);
}

void UHeldItemComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AActionCharacter>(GetOwner());

	// 서버에서만 초기 아이템 자동 장착 수행
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (DefaultItemData)
		{
			EquipHeldItemByData(DefaultItemData);
		}
		else if (DefaultHeldItemClass)
		{
			EquipHeldItemByClass(DefaultHeldItemClass);
		}
	}
}

void UHeldItemComponent::AttachHeldItemToCharacter(AHeldItemBase* ItemToAttach)
{
	if (!ItemToAttach) return;
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AActionCharacter>(GetOwner());
	}
	if (!OwnerCharacter) return;

	USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();
	const FName HandSocket = (CharacterMesh && CharacterMesh->DoesSocketExist(FName(TEXT("r_handSocket"))))
		? FName(TEXT("r_handSocket"))
		: ((CharacterMesh && CharacterMesh->DoesSocketExist(FName(TEXT("RightHandSocket")))) ? FName(TEXT("RightHandSocket")) : FName(NAME_None));

	if (HandSocket != NAME_None && CharacterMesh)
	{
		ItemToAttach->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);
	}
	else
	{
		ItemToAttach->AttachToActor(OwnerCharacter, FAttachmentTransformRules::KeepRelativeTransform);
		//ItemToAttach->SetActorRelativeLocation(FVector(40.f, 30.f, 0.f));
	}
}

void UHeldItemComponent::OnRep_CurrentHeldItem(AHeldItemBase* PreviousHeldItem)
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AActionCharacter>(GetOwner());
	}

	// 1. 이전 아이템 해제 라이프사이클 처리
	if (PreviousHeldItem && IsValid(PreviousHeldItem))
	{
		PreviousHeldItem->OnUnequipped();
	}

	// 2. 새 아이템 장착 및 소켓 부착
	if (CurrentHeldItem && IsValid(CurrentHeldItem))
	{
		AttachHeldItemToCharacter(CurrentHeldItem);
		CurrentHeldItem->OnEquipped(OwnerCharacter);

		// 로컬 컨트롤러인 경우 입력 컴포넌트 바인딩 전달
		if (OwnerCharacter && OwnerCharacter->IsLocallyControlled() && OwnerCharacter->InputComponent)
		{
			if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(OwnerCharacter->InputComponent))
			{
				CurrentHeldItem->SetupInputComponent(EIC);
			}
		}
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
	// 장착/스폰은 반드시 서버에서만 실행
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return nullptr;
	}

	UnequipHeldItem();

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AActionCharacter>(GetOwner());
	}

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
		AttachHeldItemToCharacter(CurrentHeldItem);
		CurrentHeldItem->OnEquipped(OwnerCharacter);

		// 호스트(리슨 서버)의 로컬 캐릭터인 경우 입력 바인딩 설정
		if (OwnerCharacter->IsLocallyControlled() && OwnerCharacter->InputComponent)
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

		// 액터 소멸은 서버에서만 호출 (클라이언트는 Replication으로 소멸됨)
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			CurrentHeldItem->Destroy();
		}
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
