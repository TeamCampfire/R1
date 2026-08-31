// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/InventoryComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "Data/Item/EquipmentItemData.h"
#include "Item/ItemPickup.h"
#include "GameFramework/Character.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
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
		for (FItemInstance& Slot : MainSlots)
		{
			if (OutRemainder <= 0)
			{
				break;
			}

			if (Slot.ItemData == ItemData && Slot.StackCount < ItemData->MaxStackSize)
			{
				const int32 SpaceInSlot = ItemData->MaxStackSize - Slot.StackCount;	// 여유 공간(스택) 계산
				const int32 AmountToAdd = FMath::Min(SpaceInSlot, OutRemainder);	// 추가할 스택 수 계산
				Slot.StackCount += AmountToAdd;
				OutRemainder -= AmountToAdd;
			}
		}
	}

	// 2) 남은 수량은 빈 슬롯에 새로 채운다. 장비처럼 스택 불가(MaxStackSize == 1)면
	// 슬롯 하나에 항상 1개씩만 들어가므로, 여러 개면 자연스럽게 슬롯 여러 개를 쓴다.
	for (FItemInstance& Slot : MainSlots)
	{
		if (OutRemainder <= 0)
		{
			break;
		}

		// 빈슬롯 체크
		if (!Slot.IsValid())
		{
			const int32 AmountToAdd = FMath::Min(ItemData->MaxStackSize, OutRemainder);
			Slot = FItemInstance(ItemData, AmountToAdd);
			OutRemainder -= AmountToAdd;
		}
	}

	// 1개라도 아이템이 인벤토리에 들어간 경우
	const bool bAddedAnything = OutRemainder < Count;
	if (bAddedAnything)
	{
		OnInventoryChanged.Broadcast();
	}

	/// Test Call
	PrintInventoryInfo();

	// 모든 수량이 완전히 들어갔으면 true 리턴
	// 1개도 추가되지 않았거나 일부만 추가됬으면 false 리턴
	return OutRemainder == 0;
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
	TArray<FItemInstance>& Array = GetSlotArray(Category);
	if (!Array.IsValidIndex(Index))
	{
		return;
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

		case EItemCategory::Weapon:
		case EItemCategory::Tool:
			/// 벨트 UI 갱신
			// 선택 = 손에 듦, 이미 든 같은 칸을 다시 선택 = 손을 내림. 다른 벨트 슬롯은 건드리지 않는다.
			HeldBeltIndex = (HeldBeltIndex == BeltIndex) ? INDEX_NONE : BeltIndex;
			OnInventoryChanged.Broadcast();

			/// 헬드 컴포넌트에 아이템 장착

			break;

		case EItemCategory::Consumable:
		{
			// TODO(UseItem 세부 구현): 실제 효과(Heal/RestoreHunger/RestoreThirst 등) 적용은
			// StatComponent 연동 작업에서 처리. 지금은 수량 차감만 담당한다.
			const int32 Remaining = Instance.StackCount - 1;
			SetSlot(EInventorySlotCategory::Belt, BeltIndex, Remaining > 0 ? FItemInstance(Instance.ItemData, Remaining) : FItemInstance());
			break;
		}

		default:
			// Misc 등 — 벨트에 있을 수는 있지만 사용 액션은 무동작.
			break;
	}
}

bool UInventoryComponent::UseSelectedItem()
{
	const FItemInstance Instance = GetSelectedItemInstance();
	if (!Instance.IsValid() || Instance.ItemData->Category != EItemCategory::Consumable)
	{
		return false;
	}

	// TODO(효과 적용): 실제 효과(Heal/RestoreHunger/RestoreThirst 등) 적용은 StatComponent 연동 후 처리.
	// 지금은 UseBeltSlot의 Consumable 분기와 동일하게 수량 차감만 담당한다.
	const int32 Remaining = Instance.StackCount - 1;
	SetSlot(SelectedSlotRef.Category, SelectedSlotRef.Index, Remaining > 0 ? FItemInstance(Instance.ItemData, Remaining) : FItemInstance());
	return true;
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
	SetSlot(Slot.Category, Slot.Index, Remaining > 0 ? FItemInstance(Instance.ItemData, Remaining) : FItemInstance());

	// 손에 들고 있던 벨트 슬롯을 통째로 드랍한 경우 손을 비운다.
	if (Remaining <= 0 && Slot.Category == EInventorySlotCategory::Belt && HeldBeltIndex == Slot.Index)
	{
		HeldBeltIndex = INDEX_NONE;
	}

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

void UInventoryComponent::PrintInventoryInfo()
{
	auto PrintArray = [](const TCHAR* Label, const TArray<FItemInstance>& Array)
	{
		for (int32 i = 0; i < Array.Num(); ++i)
		{
			if (Array[i].IsValid())
			{
				UE_LOG(LogTemp, Log, TEXT("[%s %d] %s x%d"),
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


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

