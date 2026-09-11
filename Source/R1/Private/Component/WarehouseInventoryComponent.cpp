// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/WarehouseInventoryComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "Net/UnrealNetwork.h"

#include "Character/ActionCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UWarehouseInventoryComponent::UWarehouseInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

FVector UWarehouseInventoryComponent::GetInteractionLocation() const
{
	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
		return FVector::ZeroVector;

	// 상호작용 액터가 플레이어 캐릭터인 경우
	if (const AActionCharacter* Character = Cast<AActionCharacter>(OwnerActor))
	{
		// 메시 기준으로 상호작용 위치 판정
		if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			static const FName PelvisBone(TEXT("pelvis"));	// 랙돌의 골반뼈 기준

			if (Mesh->GetBoneIndex(PelvisBone) != INDEX_NONE)
				return Mesh->GetSocketLocation(PelvisBone);

			return Mesh->GetComponentLocation();
		}
	}

	// 상호작용 액터가 플레이어 캐릭터가 아니고, 일반 창고인 경우
	return OwnerActor->GetActorLocation();
}

bool UWarehouseInventoryComponent::AddItem(UItemDataBase* ItemData, int32 Count, int32& OutRemainder)
{
	OutRemainder = Count;

	if (!ItemData || Count <= 0)
	{
		return false;
	}

	if (ItemData->MaxStackSize > 1)
	{
		for (int32 Index = 0; Index < StorageSlots.Num(); ++Index)
		{
			if (OutRemainder <= 0)
			{
				break;
			}

			const FItemInstance& Slot = StorageSlots[Index];
			if (Slot.ItemData == ItemData && Slot.StackCount < ItemData->MaxStackSize)
			{
				const int32 SpaceInSlot = ItemData->MaxStackSize - Slot.StackCount;
				const int32 AmountToAdd = FMath::Min(SpaceInSlot, OutRemainder);

				FItemInstance UpdatedSlot = Slot;
				UpdatedSlot.StackCount += AmountToAdd;
				SetSlotItem(Index, UpdatedSlot);

				OutRemainder -= AmountToAdd;
			}
		}
	}

	for (int32 Index = 0; Index < StorageSlots.Num(); ++Index)
	{
		if (OutRemainder <= 0)
		{
			break;
		}

		if (!StorageSlots[Index].IsValid())
		{
			const int32 AmountToAdd = FMath::Min(ItemData->MaxStackSize, OutRemainder);
			SetSlotItem(Index, FItemInstance(ItemData, AmountToAdd));
			OutRemainder -= AmountToAdd;
		}
	}

	return OutRemainder == 0;
}

void UWarehouseInventoryComponent::SetSlotItem(int32 Index, const FItemInstance& NewValue)
{
	// UInventoryComponent::SetSlot과 동일한 규칙 — 서버 권한이 없으면 무시(클라이언트는
	// 리플리케이트된 배열을 받기만 한다). 여기서 GetOwner()는 "컴포넌트를 소지한 액터"
	// (창고 액터)를 뜻하며, 네트워크 Owner(RPC 라우팅용) 개념과는 별개다.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!StorageSlots.IsValidIndex(Index))
	{
		return;
	}

	StorageSlots[Index] = NewValue;
	OnInventoryChanged.Broadcast();

	// 선택된 슬롯이 이동/소모 등으로 비게 되면 선택도 자동으로 풀어준다(빈 칸이 파란 테두리로
	// 남지 않게) — UInventoryComponent::SetSlot과 동일한 규칙.
	if (bHasSelection && SelectedIndex == Index && !NewValue.IsValid())
	{
		ClearSelection();
	}
}

void UWarehouseInventoryComponent::SelectSlot(int32 Index)
{
	bHasSelection = true;
	SelectedIndex = Index;
	OnSelectionChanged.Broadcast();
}

void UWarehouseInventoryComponent::ClearSelection()
{
	if (!bHasSelection)
	{
		return;
	}

	bHasSelection = false;
	SelectedIndex = INDEX_NONE;
	OnSelectionChanged.Broadcast();
}

FItemInstance UWarehouseInventoryComponent::GetSelectedItemInstance() const
{
	if (!bHasSelection || !StorageSlots.IsValidIndex(SelectedIndex))
	{
		return FItemInstance();
	}

	return StorageSlots[SelectedIndex];
}

void UWarehouseInventoryComponent::OnRep_StorageSlots()
{
	OnInventoryChanged.Broadcast();
}

void UWarehouseInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	StorageSlots.SetNum(StorageSlotCount);
	OnInventoryChanged.Broadcast();
}

void UWarehouseInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 창고는 특정 플레이어를 네트워크 Owner로 갖지 않으므로(레벨 배치 액터), COND_OwnerOnly를
	// 쓰면 아무 클라이언트에도 리플리케이트되지 않는다 — 반드시 전체 공개로 리플리케이트한다.
	DOREPLIFETIME(UWarehouseInventoryComponent, StorageSlots);
}
