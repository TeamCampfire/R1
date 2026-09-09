// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/InventoryComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "Data/Item/EquipmentItemData.h"
#include "Data/Item/HeldItemData.h"
#include "Item/ItemPickup.h"
#include "GameFramework/Character.h"
#include "Component/HeldItemComponent.h"
#include "Component/StatComponent.h"
#include "Component/WarehouseInventoryComponent.h"
#include "Character/ActionCharacter.h"
#include "Character/ActionPlayerController.h"
#include "Net/UnrealNetwork.h"   // DOREPLIFETIME 계열 매크로가 여기 정의돼 있음

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// 리플리케이트 되는 컴포넌트 설정
	SetIsReplicatedByDefault(true);
}


bool UInventoryComponent::AddItem(UItemDataBase* ItemData, int32 Count, int32& OutRemainder)
{
	OutRemainder = Count;

	if (!ItemData || Count <= 0)
	{
		return false;
	}

	// 1) 스택 가능한 아이템이면 같은 종류이고 아직 여유가 있는 기존 슬롯에 먼저 채운다.
	if (ItemData->MaxStackSize > 1)
	{
		for (int32 Index = 0; Index < MainSlots.Num(); ++Index)
		{
			if (OutRemainder <= 0)
			{
				break;
			}

			const FItemInstance& Slot = MainSlots[Index];
			if (Slot.ItemData == ItemData && Slot.StackCount < ItemData->MaxStackSize)
			{
				const int32 SpaceInSlot = ItemData->MaxStackSize - Slot.StackCount;	// 여유 공간(스택) 계산
				const int32 AmountToAdd = FMath::Min(SpaceInSlot, OutRemainder);	// 추가할 스택 수 계산

				FItemInstance UpdatedSlot = Slot; // 기존 슬롯 복사
				UpdatedSlot.StackCount += AmountToAdd;
				SetSlot(EInventorySlotCategory::Main, Index, UpdatedSlot);	// 슬롯 업데이트

				OutRemainder -= AmountToAdd;
			}
		}
	}

	// 2) 남은 수량은 빈 슬롯에 새로 채운다. 장비처럼 스택 불가(MaxStackSize == 1)면
	// 슬롯 하나에 항상 1개씩만 들어가므로, 여러 개면 자연스럽게 슬롯 여러 개를 쓴다.
	for (int32 Index = 0; Index < MainSlots.Num(); ++Index)
	{
		if (OutRemainder <= 0)
		{
			break;
		}

		// 빈슬롯 체크
		if (!MainSlots[Index].IsValid())
		{
			const int32 AmountToAdd = FMath::Min(ItemData->MaxStackSize, OutRemainder);
			SetSlot(EInventorySlotCategory::Main, Index, FItemInstance(ItemData, AmountToAdd));
			OutRemainder -= AmountToAdd;
		}
	}

	/// Test Call
	//PrintInventoryInfo();

	// 모든 수량이 완전히 들어갔으면 true 리턴
	// 1개도 추가되지 않았거나 일부만 추가됬으면 false 리턴
	return OutRemainder == 0;
}

void UInventoryComponent::NotifyItemAcquired(UItemDataBase* ItemData, int32 GainedAmount) const
{
	if (!ItemData || GainedAmount <= 0)
	{
		return;
	}

	APawn* OwningPawn = Cast<APawn>(GetOwner());
	AActionPlayerController* PC = OwningPawn ? Cast<AActionPlayerController>(OwningPawn->GetController()) : nullptr;
	if (PC)
	{
		PC->Client_NotifyItemAcquired(ItemData, GainedAmount, GetItemCount(ItemData));
	}
}

TArray<FItemInstance>& UInventoryComponent::GetSlotArray(EInventorySlotCategory Category)
{
	switch (Category)
	{
		case EInventorySlotCategory::Equipment:
			return EquipmentSlots;
		case EInventorySlotCategory::Belt:
			return BeltSlots;
		case EInventorySlotCategory::Main:
		default:
			return MainSlots;
	}
}

const TArray<FItemInstance>& UInventoryComponent::GetSlotArray(EInventorySlotCategory Category) const
{
	switch (Category)
	{
		case EInventorySlotCategory::Equipment:
			return EquipmentSlots;
		case EInventorySlotCategory::Belt:
			return BeltSlots;
		case EInventorySlotCategory::Main:
		default:
			return MainSlots;
	}
}

void UInventoryComponent::SetSlot(EInventorySlotCategory Category, int32 Index, const FItemInstance& NewValue)
{
	// 서버	권한이 없는 클라이언트에서 SetSlot을 호출하면 무시한다.
	// (클라이언트는 서버가 리플리케이트한 슬롯 배열을 그대로 받아서 쓰기만 한다.)
	if(!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	TArray<FItemInstance>& Array = GetSlotArray(Category);
	if (!Array.IsValidIndex(Index))
	{
		return;
	}

	// 손에 들고 있는 벨트 슬롯(HeldBeltIndex)의 아이템 종류가 이번 변경으로 바뀌면(다른 슬롯으로
	// 옮겨져 비워지거나, 다른 아이템으로 교체됨) 인벤토리 데이터와 캐릭터가 실제로 들고 있는 손
	// 아이템이 어긋나지 않도록 처리한다. 같은 아이템의 수량만 바뀌는 경우는 계속 들고 있어야
	// 하므로 제외한다. SetSlot이 모든 슬롯 변경의 단일 관문이라 여기 한 곳에서만 처리하면
	// TransferItem/DropItem 등 어떤 경로로 벨트 슬롯이 바뀌든 빠짐없이 커버된다.
	if (Category == EInventorySlotCategory::Belt && Index == HeldBeltIndex && Array[Index].ItemData != NewValue.ItemData)
	{
		UHeldItemComponent* HeldItemComp = GetOwner()->FindComponentByClass<UHeldItemComponent>();
		UHeldItemData* NewHeldData = NewValue.IsValid() ? Cast<UHeldItemData>(NewValue.ItemData) : nullptr;

		if (NewHeldData)
		{
			// 새 내용물도 여전히 HeldItem이면(예: 빈 물병 ↔ 채워진 물병처럼 같은 도구의 상태만
			// 바뀌는 경우) 손에서 내렸다가 다시 드는 대신 같은 자리에서 데이터/메시만 바꿔 끼운다 —
			// HeldBeltIndex도 그대로 유지(계속 이 슬롯을 손에 들고 있는 상태).
			if (HeldItemComp)
			{
				HeldItemComp->SwapEquippedItemData(NewHeldData);
			}
		}
		else
		{
			// 그 외(슬롯이 비워지거나 HeldItem이 아닌 다른 아이템으로 바뀜)는 기존과 동일하게 해제.
			HeldBeltIndex = INDEX_NONE;
			if (HeldItemComp)
			{
				HeldItemComp->UnequipHeldItem();
			}
		}
	}

	Array[Index] = NewValue;
	OnInventoryChanged.Broadcast();

	// 선택된 슬롯이 이동/소모 등으로 비게 되면 선택도 자동으로 풀어준다(빈 칸이 파란 테두리로 남지 않게).
	if (bHasSelection && SelectedSlotRef.Category == Category && SelectedSlotRef.Index == Index && !NewValue.IsValid())
	{
		ClearSelection();
	}
}

void UInventoryComponent::SelectSlot(const FInventorySlotRef& SlotRef)
{
	bHasSelection = true;
	SelectedSlotRef = SlotRef;
	OnSelectionChanged.Broadcast();
}

void UInventoryComponent::ClearSelection()
{
	if (!bHasSelection)
	{
		return;
	}

	bHasSelection = false;
	SelectedSlotRef = FInventorySlotRef();
	OnSelectionChanged.Broadcast();
}

bool UInventoryComponent::IsSlotSelected(const FInventorySlotRef& SlotRef) const
{
	return bHasSelection && SelectedSlotRef.Category == SlotRef.Category && SelectedSlotRef.Index == SlotRef.Index;
}

FItemInstance UInventoryComponent::GetSelectedItemInstance() const
{
	if (!bHasSelection)
	{
		return FItemInstance();
	}

	const TArray<FItemInstance>& Array = GetSlotArray(SelectedSlotRef.Category);
	return Array.IsValidIndex(SelectedSlotRef.Index) ? Array[SelectedSlotRef.Index] : FItemInstance();
}

EMoveSlotResult UInventoryComponent::EquipToSlot(const FInventorySlotRef& From, const FItemInstance& SourceInstance)
{
	const UEquipmentItemData* EquipData = Cast<UEquipmentItemData>(SourceInstance.ItemData);
	if (!EquipData || EquipData->EquipSlot == EEquipmentSlotType::None)
	{
		// Equipment 카테고리가 아니거나(Weapon/Tool/Consumable/Misc) 부위가 없는 아이템은 장착 불가.
		return EMoveSlotResult::Failed;
	}

	// 부위별 고정 인덱스는 없다 — 같은 부위(EquipSlot)를 이미 착용 중이면 그 칸을 교체 대상으로 삼는다.
	int32 TargetIndex = EquipmentSlots.IndexOfByPredicate([EquipData](const FItemInstance& Slot)
	{
		const UEquipmentItemData* Existing = Slot.IsValid() ? Cast<UEquipmentItemData>(Slot.ItemData) : nullptr;
		return Existing && Existing->EquipSlot == EquipData->EquipSlot;
	});

	// 같은 부위 착용품이 없으면 빈 칸 아무데나 채운다.
	if (TargetIndex == INDEX_NONE)
	{
		TargetIndex = EquipmentSlots.IndexOfByPredicate([](const FItemInstance& Slot) { return !Slot.IsValid(); });
	}

	if (TargetIndex == INDEX_NONE)
	{
		// 같은 부위로 교체할 것도 없고 빈 칸도 없음(장비 칸이 꽉 참).
		return EMoveSlotResult::Failed;
	}

	// 장비는 항상 스택 1개 단위 — 기존에 그 칸에 있던 아이템은 새 아이템이 있던 자리로 되돌린다.
	const FItemInstance PreviouslyEquipped = EquipmentSlots[TargetIndex];

	SetSlot(EInventorySlotCategory::Equipment, TargetIndex, SourceInstance);
	SetSlot(From.Category, From.Index, PreviouslyEquipped);

	return EMoveSlotResult::Equipped;
}

void UInventoryComponent::OnRep_Slots()
{
	OnInventoryChanged.Broadcast();
}

EMoveSlotResult UInventoryComponent::TransferItem(FInventorySlotRef From, FInventorySlotRef To, int32 Count, bool bAutoHalfSplitIfTargetEmpty)
{
	if (From.Category == To.Category && From.Index == To.Index)
	{
		return EMoveSlotResult::Failed;
	}

	TArray<FItemInstance>& FromArray = GetSlotArray(From.Category);
	if (!FromArray.IsValidIndex(From.Index) || !FromArray[From.Index].IsValid())
	{
		return EMoveSlotResult::Failed;
	}

	const FItemInstance SourceInstance = FromArray[From.Index];
	const int32 MoveCount = (Count > 0) ? FMath::Min(Count, SourceInstance.StackCount) : SourceInstance.StackCount;

	// 대상이 장비슬롯이면 자동 장착 경로로 분기(부위 일치 검사 + 스왑은 EquipToSlot이 전담).
	if (To.Category == EInventorySlotCategory::Equipment)
	{
		return EquipToSlot(From, SourceInstance);
	}

	TArray<FItemInstance>& ToArray = GetSlotArray(To.Category);
	if (!ToArray.IsValidIndex(To.Index))
	{
		return EMoveSlotResult::Failed;
	}

	const FItemInstance TargetInstance = ToArray[To.Index];
	const bool bFromEquipment = (From.Category == EInventorySlotCategory::Equipment);

	// 대상이 비어있음 → 그냥 이동(장비슬롯에서 나오는 경우면 "해제"로 취급).
	if (!TargetInstance.IsValid())
	{
		// 휠클릭 드래그(bAutoHalfSplitIfTargetEmpty)로 빈 슬롯에 놓았고 Count를 따로 지정하지
		// 않았으면(0 이하), 2개 이상 쌓여있는 경우에 한해 절반만(내림) 떼어 옮긴다 — 나머지는
		// 원래 자리에 남는다. 그 외에는 기존과 동일하게 MoveCount(전량 또는 지정 수량) 그대로.
		const int32 ActualMoveCount = (bAutoHalfSplitIfTargetEmpty && Count <= 0 && SourceInstance.StackCount >= 2)
			? (SourceInstance.StackCount / 2)
			: MoveCount;

		FItemInstance Moved = SourceInstance;
		Moved.StackCount = ActualMoveCount;
		SetSlot(To.Category, To.Index, Moved);

		const int32 Remaining = SourceInstance.StackCount - ActualMoveCount;
		SetSlot(From.Category, From.Index, Remaining > 0 ? FItemInstance(SourceInstance.ItemData, Remaining) : FItemInstance());

		return bFromEquipment ? EMoveSlotResult::Unequipped : EMoveSlotResult::Moved;
	}

	// 대상에 같은 아이템이 있고 여유가 있으면 병합.
	if (TargetInstance.ItemData == SourceInstance.ItemData && TargetInstance.StackCount < TargetInstance.ItemData->MaxStackSize)
	{
		const int32 SpaceInTarget = TargetInstance.ItemData->MaxStackSize - TargetInstance.StackCount;
		const int32 AmountToMerge = FMath::Min(SpaceInTarget, MoveCount);

		FItemInstance MergedTarget = TargetInstance;
		MergedTarget.StackCount += AmountToMerge;
		SetSlot(To.Category, To.Index, MergedTarget);

		const int32 Remaining = SourceInstance.StackCount - AmountToMerge;
		SetSlot(From.Category, From.Index, Remaining > 0 ? FItemInstance(SourceInstance.ItemData, Remaining) : FItemInstance());

		return EMoveSlotResult::Merged;
	}

	// 대상에 다른 아이템 → 자리 교환. 장비슬롯이 얽힌 경우는 EquipToSlot이 전담하므로 여기선
	// From/To 둘 다 Main/Belt일 때만 스왑을 허용한다.
	if (!bFromEquipment)
	{
		SetSlot(To.Category, To.Index, SourceInstance);
		SetSlot(From.Category, From.Index, TargetInstance);
		return EMoveSlotResult::Swapped;
	}

	return EMoveSlotResult::Failed;
}

bool UInventoryComponent::Server_TransferItem_Validate(FInventorySlotRef From, FInventorySlotRef To, int32 Count, bool bAutoHalfSplitIfTargetEmpty)
{
	return true;	// 필요하면 인덱스 범위 등 검증 추가
}


void UInventoryComponent::Server_TransferItem_Implementation(FInventorySlotRef From, FInventorySlotRef To, int32 Count, bool bAutoHalfSplitIfTargetEmpty)
{
	TransferItem(From, To, Count, bAutoHalfSplitIfTargetEmpty);
}

EMoveSlotResult UInventoryComponent::QuickMoveItem(const FInventorySlotRef& SlotRef)
{
	const TArray<FItemInstance>& FromArray = GetSlotArray(SlotRef.Category);
	if (!FromArray.IsValidIndex(SlotRef.Index) || !FromArray[SlotRef.Index].IsValid())
	{
		return EMoveSlotResult::Failed;
	}

	const FItemInstance SourceInstance = FromArray[SlotRef.Index];

	// 장비슬롯에서 우클릭 → 빈 메인 슬롯이 있을 때만 해제. 없으면 다른 아이템과 바꿔치기하지 않고 무동작.
	if (SlotRef.Category == EInventorySlotCategory::Equipment)
	{
		const int32 EmptyMainIndex = MainSlots.IndexOfByPredicate([](const FItemInstance& Slot) { return !Slot.IsValid(); });
		if (EmptyMainIndex == INDEX_NONE)
		{
			return EMoveSlotResult::Failed;
		}
		return TransferItem(SlotRef, FInventorySlotRef{ EInventorySlotCategory::Main, EmptyMainIndex }, 0);
	}

	// 메인/벨트에서 우클릭한 게 장비 아이템이면 대상 인덱스와 무관하게 EquipToSlot이 알아서
	// 같은 부위 교체 또는 빈 칸 장착을 처리한다(TransferItem이 To.Category만 보고 분기).
	if (SourceInstance.ItemData && SourceInstance.ItemData->Category == EItemCategory::Equipment)
	{
		return TransferItem(SlotRef, FInventorySlotRef{ EInventorySlotCategory::Equipment, 0 }, 0);
	}

	// 그 외(Weapon/Tool/Consumable/Misc)는 메인 ↔ 벨트 반대편의 빈 슬롯으로 이동. 빈 칸이 없으면 무동작.
	const EInventorySlotCategory TargetCategory =
		(SlotRef.Category == EInventorySlotCategory::Main) ? EInventorySlotCategory::Belt : EInventorySlotCategory::Main;

	const TArray<FItemInstance>& TargetArray = GetSlotArray(TargetCategory);
	const int32 EmptyIndex = TargetArray.IndexOfByPredicate([](const FItemInstance& Slot) { return !Slot.IsValid(); });
	if (EmptyIndex == INDEX_NONE)
	{
		return EMoveSlotResult::Failed;
	}

	return TransferItem(SlotRef, FInventorySlotRef{ TargetCategory, EmptyIndex }, 0);
}

bool UInventoryComponent::Server_QuickMoveItem_Validate(FInventorySlotRef SlotRef)
{
	return true;	// 필요하면 인덱스 범위 등 검증 추가
}

void UInventoryComponent::Server_QuickMoveItem_Implementation(FInventorySlotRef SlotRef)
{
	QuickMoveItem(SlotRef);
}

void UInventoryComponent::UseBeltSlot(int32 BeltIndex)
{
	if (!BeltSlots.IsValidIndex(BeltIndex))
	{
		return;
	}

	const FItemInstance Instance = BeltSlots[BeltIndex];
	if (!Instance.IsValid())
	{
		// 빈 슬롯 단축키 — 지금 손에 든 무기/도구가 있으면 맨손으로 내려놓는다(Rust처럼 빈 칸
		// 단축키가 "무장 해제" 역할). 아무것도 안 들고 있었으면 그대로 무동작.
		if (HeldBeltIndex != INDEX_NONE)
		{
			HeldBeltIndex = INDEX_NONE;
			if (UHeldItemComponent* HeldItemComp = GetOwner() ? GetOwner()->FindComponentByClass<UHeldItemComponent>() : nullptr)
			{
				HeldItemComp->UnequipHeldItem();
			}
			OnInventoryChanged.Broadcast();
		}
		return;
	}

	switch (Instance.ItemData->Category)
	{
		case EItemCategory::Equipment:
			// 장착/교체는 QuickMoveItem의 벨트→장비 경로와 동일 — 대상 인덱스는 EquipToSlot이 알아서 찾는다.
			TransferItem(FInventorySlotRef{ EInventorySlotCategory::Belt, BeltIndex }, FInventorySlotRef{ EInventorySlotCategory::Equipment, 0 }, 0);
			break;

		case EItemCategory::HeldItem:
		{
			// 이미 이 슬롯의 아이템을 들고 있는 경우 -> 손에서 내리기 (토글)
			if (HeldBeltIndex == BeltIndex)
			{
				HeldBeltIndex = INDEX_NONE;
				if (UHeldItemComponent* HeldItemComp = GetOwner() ? GetOwner()->FindComponentByClass<UHeldItemComponent>() : nullptr)
				{
					HeldItemComp->UnequipHeldItem();
				}
				OnInventoryChanged.Broadcast();
				break;
			}

			// 1. 현재 선택된 벨트 슬롯 갱신
			HeldBeltIndex = BeltIndex;
			// 2. HeldItemComponent를 찾아 도구 장착 실행
			if (UHeldItemComponent* HeldItemComp = GetOwner() ? GetOwner()->FindComponentByClass<UHeldItemComponent>() : nullptr)
			{
				if (UHeldItemData* EquipData = Cast<UHeldItemData>(Instance.ItemData))
				{
					HeldItemComp->EquipHeldItemByData(EquipData);
				}
			}
			OnInventoryChanged.Broadcast();
			break;
		}

		case EItemCategory::Consumable:
		{
			ApplyItemEffects(Instance.ItemData);

			const int32 Remaining = Instance.StackCount - 1;
			SetSlot(EInventorySlotCategory::Belt, BeltIndex, Remaining > 0 ? FItemInstance(Instance.ItemData, Remaining) : FItemInstance());
			break;
		}

		default:
			// Misc 등 — 벨트에 있을 수는 있지만 사용 액션은 무동작.
			break;
	}
}

bool UInventoryComponent::Server_UseBeltSlot_Validate(int32 BeltIndex)
{
	return true;	// 필요하면 인덱스 범위 등 검증 추가
}

void UInventoryComponent::Server_UseBeltSlot_Implementation(int32 BeltIndex)
{
	UseBeltSlot(BeltIndex);
}

bool UInventoryComponent::UseSelectedItem(const FInventorySlotRef& SlotRef)
{
	const TArray<FItemInstance>& Array = GetSlotArray(SlotRef.Category);
	if (!Array.IsValidIndex(SlotRef.Index) || !Array[SlotRef.Index].IsValid())
	{
		return false;
	}

	const FItemInstance Instance = Array[SlotRef.Index];
	if (Instance.ItemData->Category != EItemCategory::Consumable)
	{
		return false;
	}

	ApplyItemEffects(Instance.ItemData);

	const int32 Remaining = Instance.StackCount - 1;
	SetSlot(SlotRef.Category, SlotRef.Index, Remaining > 0 ? FItemInstance(Instance.ItemData, Remaining) : FItemInstance());
	return true;
}

void UInventoryComponent::ApplyItemEffects(const UItemDataBase* ItemData)
{
	if (!ItemData)
	{
		return;
	}

	// FindComponentByClass<UStatComponent>()를 쓰면 안 된다 — BP_PlayerV3의 상속 컴포넌트
	// 템플릿 문제로 실제 게임에 쓰이는 것과 다른(InitializeStat을 거치지 않은) StatComponent
	// 인스턴스를 찾아오는 게 확인됐다. 코드베이스 전역에서 StatComponent는 항상
	// AActionCharacter::GetStatComponent()(멤버 포인터 접근자)로만 얻는다 — 그 관례를 따른다.
	AActionCharacter* OwningCharacter = Cast<AActionCharacter>(GetOwner());
	UStatComponent* StatComp = OwningCharacter ? OwningCharacter->GetStatComponent() : nullptr;
	if (!StatComp)
	{
		return;
	}

	for (const FItemEffect& Effect : ItemData->Effects)
	{
		StatComp->ApplyItemEffect(Effect);
	}
}

bool UInventoryComponent::Server_UseSelectedItem_Validate(FInventorySlotRef SlotRef)
{
	return true;
}

void UInventoryComponent::Server_UseSelectedItem_Implementation(FInventorySlotRef SlotRef)
{
	UseSelectedItem(SlotRef);
}

bool UInventoryComponent::DropItem(FInventorySlotRef Slot, int32 Count, const FTransform& DropTransform, const FVector& ThrowImpulse)
{
	TArray<FItemInstance>& Array = GetSlotArray(Slot.Category);
	if (!Array.IsValidIndex(Slot.Index) || !Array[Slot.Index].IsValid())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FItemInstance Instance = Array[Slot.Index];
	const int32 DropCount = (Count > 0) ? FMath::Min(Count, Instance.StackCount) : Instance.StackCount;

	AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), DropTransform);
	if (!Pickup)
	{
		return false;
	}
	Pickup->InitializeFromItem(Instance.ItemData, DropCount);

	if (!ThrowImpulse.IsNearlyZero())
	{
		Pickup->AddThrowImpulse(ThrowImpulse);
	}

	const int32 Remaining = Instance.StackCount - DropCount;
	// 손에 들고 있던 벨트 슬롯을 통째로 드랍해 비우는 경우의 장착 해제는 SetSlot이 처리한다.
	SetSlot(Slot.Category, Slot.Index, Remaining > 0 ? FItemInstance(Instance.ItemData, Remaining) : FItemInstance());

	return true;
}

bool UInventoryComponent::ThrowItem(FInventorySlotRef Slot, int32 Count)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return false;
	}

	const FRotator ViewRotation = Character->GetControlRotation();
	const FVector Forward = ViewRotation.Vector();
	const FVector EyeLocation = Character->GetActorLocation() + FVector(0.f, 0.f, Character->BaseEyeHeight);
	const FVector SpawnLocation = EyeLocation + Forward * ThrowSpawnDistance;

	return DropItem(Slot, Count, FTransform(ViewRotation, SpawnLocation), Forward * ThrowImpulseStrength);
}

bool UInventoryComponent::Server_ThrowItem_Validate(FInventorySlotRef Slot, int32 Count)
{
	return true;	// 필요하면 인덱스 범위 등 검증 추가
}

void UInventoryComponent::Server_ThrowItem_Implementation(FInventorySlotRef Slot, int32 Count)
{
	ThrowItem(Slot, Count);
}

int32 UInventoryComponent::GetItemCount(const UItemDataBase* ItemData) const
{
	if (!ItemData)
	{
		return 0;
	}

	int32 Total = 0;

	// 메인 슬롯 검사
	for (const FItemInstance& Slot : MainSlots)
	{
		if (Slot.ItemData == ItemData)
		{
			Total += Slot.StackCount;
		}
	}

	// 벨트 슬롯 검사
	for (const FItemInstance& Slot : BeltSlots)
	{
		if (Slot.ItemData == ItemData)
		{
			Total += Slot.StackCount;
		}
	}

	// 장비슬롯은 재료가 들어갈 일 없으니 제외

	return Total;
}

bool UInventoryComponent::HasEnoughOf(const UItemDataBase* ItemData, int32 Amount) const
{
	return GetItemCount(ItemData) >= Amount;
}

bool UInventoryComponent::HasIngredients(const TArray<FCraftIngredient>& Ingredients) const
{
	for (const FCraftIngredient& Ingredient : Ingredients)
	{
		UItemDataBase* RequiredItem = Ingredient.Item.LoadSynchronous();
		if (!RequiredItem)
		{
			continue;	// 재료 칸이 비어있는 레시피 데이터 — 요구사항 없음으로 취급
		}

		if (GetItemCount(RequiredItem) < Ingredient.Amount)
		{
			return false;
		}
	}
	return true;
}

bool UInventoryComponent::CanCraftItem(const UItemDataBase* ItemToCraft) const
{
	return ItemToCraft && HasIngredients(ItemToCraft->CraftingCost);
}

bool UInventoryComponent::ConsumeIngredients(const TArray<FCraftIngredient>& Ingredients)
{
	if (!HasIngredients(Ingredients))
	{
		return false;
	}

	for (const FCraftIngredient& Ingredient : Ingredients)
	{
		if (UItemDataBase* RequiredItem = Ingredient.Item.LoadSynchronous())
		{
			ConsumeItemCount(RequiredItem, Ingredient.Amount);
		}
	}

	return true;
}

bool UInventoryComponent::ConsumeItemCount(const UItemDataBase* ItemData, int32 CountToRemove)
{
	if (!HasEnoughOf(ItemData, CountToRemove))
	{
		return false;
	}

	auto RemoveFrom = [this, ItemData, &CountToRemove](EInventorySlotCategory Category)
	{
		TArray<FItemInstance>& Array = GetSlotArray(Category);
		for (int32 Index = 0; Index < Array.Num() && CountToRemove > 0; ++Index)
		{
			const FItemInstance& Slot = Array[Index];
			if (Slot.ItemData != ItemData)
			{
				continue;
			}

			const int32 AmountToTake = FMath::Min(Slot.StackCount, CountToRemove);
			const int32 Remaining = Slot.StackCount - AmountToTake;
			SetSlot(Category, Index, Remaining > 0 ? FItemInstance(Slot.ItemData, Remaining) : FItemInstance());
			CountToRemove -= AmountToTake;
		}
	};

	RemoveFrom(EInventorySlotCategory::Main);
	RemoveFrom(EInventorySlotCategory::Belt);
	return true;
}

void UInventoryComponent::SetSlotItem(const FInventorySlotRef& SlotRef, const FItemInstance& NewValue)
{
	SetSlot(SlotRef.Category, SlotRef.Index, NewValue);
}

bool UInventoryComponent::TransferWithWarehouse(UWarehouseInventoryComponent* Warehouse, FInventorySlotRef PlayerSlot, int32 WarehouseSlotIndex, int32 Count, bool bToWarehouse)
{
	if (!Warehouse || !Warehouse->StorageSlots.IsValidIndex(WarehouseSlotIndex))
	{
		return false;
	}

	TArray<FItemInstance>& PlayerArray = GetSlotArray(PlayerSlot.Category);
	if (!PlayerArray.IsValidIndex(PlayerSlot.Index))
	{
		return false;
	}

	const FItemInstance& PlayerInstance = PlayerArray[PlayerSlot.Index];
	const FItemInstance& WarehouseInstance = Warehouse->StorageSlots[WarehouseSlotIndex];

	// bToWarehouse 방향에 따라 Source/Target을 정하면, 아래 이동/병합/교환 로직은
	// TransferItem과 동일한 모양으로 방향에 무관하게 처리할 수 있다.
	const FItemInstance SourceInstance = bToWarehouse ? PlayerInstance : WarehouseInstance;
	const FItemInstance TargetInstance = bToWarehouse ? WarehouseInstance : PlayerInstance;

	if (!SourceInstance.IsValid())
	{
		return false;
	}

	const int32 MoveCount = (Count > 0) ? FMath::Min(Count, SourceInstance.StackCount) : SourceInstance.StackCount;

	FItemInstance NewSource;
	FItemInstance NewTarget;

	if (!TargetInstance.IsValid())
	{
		// 대상이 비어있음 → 그냥 이동.
		NewTarget = SourceInstance;
		NewTarget.StackCount = MoveCount;

		const int32 Remaining = SourceInstance.StackCount - MoveCount;
		NewSource = Remaining > 0 ? FItemInstance(SourceInstance.ItemData, Remaining) : FItemInstance();
	}
	else if (TargetInstance.ItemData == SourceInstance.ItemData && TargetInstance.StackCount < TargetInstance.ItemData->MaxStackSize)
	{
		// 같은 아이템 + 여유 있음 → 병합.
		const int32 SpaceInTarget = TargetInstance.ItemData->MaxStackSize - TargetInstance.StackCount;
		const int32 AmountToMerge = FMath::Min(SpaceInTarget, MoveCount);

		NewTarget = TargetInstance;
		NewTarget.StackCount += AmountToMerge;

		const int32 Remaining = SourceInstance.StackCount - AmountToMerge;
		NewSource = Remaining > 0 ? FItemInstance(SourceInstance.ItemData, Remaining) : FItemInstance();
	}
	else
	{
		// 다른 아이템 → 자리 교환.
		NewTarget = SourceInstance;
		NewSource = TargetInstance;
	}

	if (bToWarehouse)
	{
		SetSlotItem(PlayerSlot, NewSource);
		Warehouse->SetSlotItem(WarehouseSlotIndex, NewTarget);
	}
	else
	{
		Warehouse->SetSlotItem(WarehouseSlotIndex, NewSource);
		SetSlotItem(PlayerSlot, NewTarget);
	}

	return true;
}

bool UInventoryComponent::Server_TransferWithWarehouse_Validate(UWarehouseInventoryComponent* Warehouse, FInventorySlotRef PlayerSlot, int32 WarehouseSlotIndex, int32 Count, bool bToWarehouse)
{
	if (!Warehouse)
	{
		return false;
	}

	// 남의(창고) 컴포넌트를 건드리는 함수라 거리 검증을 반드시 둔다 — 기존 Server_TransferItem류의
	// _Validate는 자기 자신의 슬롯만 다루므로 비어있어도 큰 문제가 없었지만, 이 함수는 원거리에서
	// 악의적으로 창고 슬롯을 조작하는 걸 막아야 한다.
	const AActor* PlayerOwner = GetOwner();
	const AActor* WarehouseOwner = Warehouse->GetOwner();
	if (!PlayerOwner || !WarehouseOwner)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(PlayerOwner->GetActorLocation(), WarehouseOwner->GetActorLocation());
	return DistSq <= FMath::Square(Warehouse->MaxInteractDistance);
}

void UInventoryComponent::Server_TransferWithWarehouse_Implementation(UWarehouseInventoryComponent* Warehouse, FInventorySlotRef PlayerSlot, int32 WarehouseSlotIndex, int32 Count, bool bToWarehouse)
{
	TransferWithWarehouse(Warehouse, PlayerSlot, WarehouseSlotIndex, Count, bToWarehouse);
}

bool UInventoryComponent::TransferWithinWarehouse(UWarehouseInventoryComponent* Warehouse, int32 FromIndex, int32 ToIndex, int32 Count, bool bAutoHalfSplitIfTargetEmpty)
{
	if (!Warehouse || FromIndex == ToIndex)
	{
		return false;
	}

	if (!Warehouse->StorageSlots.IsValidIndex(FromIndex) || !Warehouse->StorageSlots.IsValidIndex(ToIndex))
	{
		return false;
	}

	const FItemInstance SourceInstance = Warehouse->StorageSlots[FromIndex];
	const FItemInstance TargetInstance = Warehouse->StorageSlots[ToIndex];
	if (!SourceInstance.IsValid())
	{
		return false;
	}

	const int32 MoveCount = (Count > 0) ? FMath::Min(Count, SourceInstance.StackCount) : SourceInstance.StackCount;

	// 대상이 비어있음 → 그냥 이동(휠클릭 드래그로 빈 슬롯에 놓았고 Count 미지정이면 절반만).
	if (!TargetInstance.IsValid())
	{
		const int32 ActualMoveCount = (bAutoHalfSplitIfTargetEmpty && Count <= 0 && SourceInstance.StackCount >= 2)
			? (SourceInstance.StackCount / 2)
			: MoveCount;

		FItemInstance Moved = SourceInstance;
		Moved.StackCount = ActualMoveCount;
		Warehouse->SetSlotItem(ToIndex, Moved);

		const int32 Remaining = SourceInstance.StackCount - ActualMoveCount;
		Warehouse->SetSlotItem(FromIndex, Remaining > 0 ? FItemInstance(SourceInstance.ItemData, Remaining) : FItemInstance());

		return true;
	}

	// 대상에 같은 아이템이 있고 여유가 있으면 병합.
	if (TargetInstance.ItemData == SourceInstance.ItemData && TargetInstance.StackCount < TargetInstance.ItemData->MaxStackSize)
	{
		const int32 SpaceInTarget = TargetInstance.ItemData->MaxStackSize - TargetInstance.StackCount;
		const int32 AmountToMerge = FMath::Min(SpaceInTarget, MoveCount);

		FItemInstance MergedTarget = TargetInstance;
		MergedTarget.StackCount += AmountToMerge;
		Warehouse->SetSlotItem(ToIndex, MergedTarget);

		const int32 Remaining = SourceInstance.StackCount - AmountToMerge;
		Warehouse->SetSlotItem(FromIndex, Remaining > 0 ? FItemInstance(SourceInstance.ItemData, Remaining) : FItemInstance());

		return true;
	}

	// 대상에 다른 아이템 → 자리 교환.
	Warehouse->SetSlotItem(ToIndex, SourceInstance);
	Warehouse->SetSlotItem(FromIndex, TargetInstance);

	return true;
}

bool UInventoryComponent::Server_TransferWithinWarehouse_Validate(UWarehouseInventoryComponent* Warehouse, int32 FromIndex, int32 ToIndex, int32 Count, bool bAutoHalfSplitIfTargetEmpty)
{
	if (!Warehouse)
	{
		return false;
	}

	// Server_TransferWithWarehouse_Validate와 동일한 이유로 거리 검증을 둔다 — 남의(창고)
	// 컴포넌트를 건드리는 함수라 원거리에서 악의적으로 조작하는 걸 막아야 한다.
	const AActor* PlayerOwner = GetOwner();
	const AActor* WarehouseOwner = Warehouse->GetOwner();
	if (!PlayerOwner || !WarehouseOwner)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(PlayerOwner->GetActorLocation(), WarehouseOwner->GetActorLocation());
	return DistSq <= FMath::Square(Warehouse->MaxInteractDistance);
}

void UInventoryComponent::Server_TransferWithinWarehouse_Implementation(UWarehouseInventoryComponent* Warehouse, int32 FromIndex, int32 ToIndex, int32 Count, bool bAutoHalfSplitIfTargetEmpty)
{
	TransferWithinWarehouse(Warehouse, FromIndex, ToIndex, Count, bAutoHalfSplitIfTargetEmpty);
}

bool UInventoryComponent::QuickMoveToWarehouse(UWarehouseInventoryComponent* Warehouse, FInventorySlotRef PlayerSlot)
{
	if (!Warehouse)
	{
		return false;
	}

	const TArray<FItemInstance>& PlayerArray = GetSlotArray(PlayerSlot.Category);
	if (!PlayerArray.IsValidIndex(PlayerSlot.Index) || !PlayerArray[PlayerSlot.Index].IsValid())
	{
		return false;
	}

	const int32 EmptyWarehouseIndex = Warehouse->StorageSlots.IndexOfByPredicate([](const FItemInstance& Slot) { return !Slot.IsValid(); });
	if (EmptyWarehouseIndex == INDEX_NONE)
	{
		return false;
	}

	return TransferWithWarehouse(Warehouse, PlayerSlot, EmptyWarehouseIndex, 0, true);
}

bool UInventoryComponent::Server_QuickMoveToWarehouse_Validate(UWarehouseInventoryComponent* Warehouse, FInventorySlotRef PlayerSlot)
{
	if (!Warehouse)
	{
		return false;
	}

	// Server_TransferWithWarehouse_Validate와 동일한 이유로 거리 검증을 둔다.
	const AActor* PlayerOwner = GetOwner();
	const AActor* WarehouseOwner = Warehouse->GetOwner();
	if (!PlayerOwner || !WarehouseOwner)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(PlayerOwner->GetActorLocation(), WarehouseOwner->GetActorLocation());
	return DistSq <= FMath::Square(Warehouse->MaxInteractDistance);
}

void UInventoryComponent::Server_QuickMoveToWarehouse_Implementation(UWarehouseInventoryComponent* Warehouse, FInventorySlotRef PlayerSlot)
{
	QuickMoveToWarehouse(Warehouse, PlayerSlot);
}

bool UInventoryComponent::QuickMoveFromWarehouse(UWarehouseInventoryComponent* Warehouse, int32 WarehouseSlotIndex)
{
	if (!Warehouse || !Warehouse->StorageSlots.IsValidIndex(WarehouseSlotIndex))
	{
		return false;
	}

	const FItemInstance& SourceInstance = Warehouse->StorageSlots[WarehouseSlotIndex];
	if (!SourceInstance.IsValid())
	{
		return false;
	}

	// HeldItem/Consumable/Placeable은 벨트를 우선 시도(벨트에서 바로 손에 들거나/사용하거나/배치해야
	// 하므로), Equipment는 장비 슬롯 자동 장착이 QuickMoveItem 쪽 규칙이라 벨트에 놓일 이유가 없어
	// Misc와 동일하게 메인을 우선한다.
	const EItemCategory Category = SourceInstance.ItemData ? SourceInstance.ItemData->Category : EItemCategory::Misc;
	const bool bPreferBelt = Category == EItemCategory::HeldItem
		|| Category == EItemCategory::Consumable
		|| Category == EItemCategory::Placeable;

	EInventorySlotCategory TargetCategory = EInventorySlotCategory::Main;
	int32 TargetIndex = INDEX_NONE;

	if (bPreferBelt)
	{
		TargetIndex = BeltSlots.IndexOfByPredicate([](const FItemInstance& Slot) { return !Slot.IsValid(); });
		TargetCategory = EInventorySlotCategory::Belt;
	}

	// 벨트를 안 우선하거나(Misc), 벨트가 꽉 차서 못 찾았으면 메인 빈 칸으로 대체.
	if (TargetIndex == INDEX_NONE)
	{
		TargetIndex = MainSlots.IndexOfByPredicate([](const FItemInstance& Slot) { return !Slot.IsValid(); });
		TargetCategory = EInventorySlotCategory::Main;
	}

	if (TargetIndex == INDEX_NONE)
	{
		return false;
	}

	return TransferWithWarehouse(Warehouse, FInventorySlotRef{ TargetCategory, TargetIndex }, WarehouseSlotIndex, 0, false);
}

bool UInventoryComponent::Server_QuickMoveFromWarehouse_Validate(UWarehouseInventoryComponent* Warehouse, int32 WarehouseSlotIndex)
{
	if (!Warehouse)
	{
		return false;
	}

	const AActor* PlayerOwner = GetOwner();
	const AActor* WarehouseOwner = Warehouse->GetOwner();
	if (!PlayerOwner || !WarehouseOwner)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(PlayerOwner->GetActorLocation(), WarehouseOwner->GetActorLocation());
	return DistSq <= FMath::Square(Warehouse->MaxInteractDistance);
}

void UInventoryComponent::Server_QuickMoveFromWarehouse_Implementation(UWarehouseInventoryComponent* Warehouse, int32 WarehouseSlotIndex)
{
	QuickMoveFromWarehouse(Warehouse, WarehouseSlotIndex);
}

bool UInventoryComponent::ConsumeItemInstance(const FInventorySlotRef& SlotRef, const FGuid& ExpectedInstanceID, const UItemDataBase* ExpectedItemData)
{
	if (false == ExpectedInstanceID.IsValid() || false == IsValid(ExpectedItemData)) return false;

	// 유효한지 확인
	TArray<FItemInstance>& SlotArray = GetSlotArray(SlotRef.Category);
	if (false == SlotArray.IsValidIndex(SlotRef.Index)) return false;

	const FItemInstance& Instance = SlotArray[SlotRef.Index];
	if (false == Instance.IsValid()) return false;

	// 인스턴스 ID, 아이템 데이터 형식이 맞지 않으면 소비 안 함
	if (ExpectedInstanceID != Instance.InstanceID) return false;
	if (ExpectedItemData != Instance.ItemData) return false;

	FItemInstance UpdatedInstance = Instance;
	--UpdatedInstance.StackCount; // 수량 하나 차감

	SetSlot(SlotRef.Category, SlotRef.Index,
		UpdatedInstance.StackCount > 0 ? UpdatedInstance : FItemInstance());

	return true;
}

void UInventoryComponent::PrintInventoryInfo()
{
	auto PrintArray = [this](const TCHAR* Label, const TArray<FItemInstance>& Array)
	{
		for (int32 i = 0; i < Array.Num(); ++i)
		{
			if (Array[i].IsValid())
			{
				UE_LOG(LogTemp, Log, TEXT("[PIE %d][%s] - [%s %d] %s x%d"),
					UE::GetPlayInEditorID(),
					GetOwner() && GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
					Label,
					i,
					*(Array[i].ItemData->DisplayName.ToString()),
					Array[i].StackCount
				);
			}
		}
	};

	PrintArray(TEXT("Main"), MainSlots);
	PrintArray(TEXT("Belt"), BeltSlots);
	PrintArray(TEXT("Equip"), EquipmentSlots);
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 빈 FItemInstance(ItemData = nullptr)로 채워진 고정 슬롯을 만든다.
	// FItemInstance::IsValid()가 ItemData == nullptr일 때 false를 반환하므로
	// "빈 슬롯" 판정에 별도 플래그가 필요 없다.
	MainSlots.SetNum(MainSlotCount);
	BeltSlots.SetNum(BeltSlotCount);
	EquipmentSlots.SetNum(EquipmentSlotCount);

	// 슬롯 배열이 0칸에서 실제 칸 수로 바뀌는 것도 상태 변화이므로 알려준다 — 이게 없으면
	// BeginPlay보다 먼저 초기화되는 UI(레벨 블루프린트 등에서 만든 위젯)가 빈 배열을 스냅샷한
	// 채로 이후 갱신을 못 받는 문제가 생긴다.
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);   // 부모(UActorComponent)의 리플리케이션 설정을 먼저 유지

	// 메인, 벨트 슬롯은 소유자만 볼 수 있으므로 COND_OwnerOnly를 쓴다.(대역폭 최소화)
	DOREPLIFETIME_CONDITION(UInventoryComponent, MainSlots, COND_OwnerOnly);	
	DOREPLIFETIME_CONDITION(UInventoryComponent, BeltSlots, COND_OwnerOnly);

	// 장비 슬롯은 다른 플레이어도 볼 수 있으므로 COND_OwnerOnly를 쓰지 않는다.
	DOREPLIFETIME(UInventoryComponent, EquipmentSlots);

	// 일단 장비 슬롯과 함께 다른 플레이어도 볼 수 있게 리플레이트. 필요하면 COND_OwnerOnly로 바꿀 수도 있다.
	// AHeldItemBase, HeldComponent 동작 참고해서 판단
	DOREPLIFETIME(UInventoryComponent, HeldBeltIndex);	
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

