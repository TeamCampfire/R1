// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/InventoryComponent.h"
#include "Data/Item/ItemDataBase.h"

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
		for (FItemInstance& Slot : Slots)
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
	for (FItemInstance& Slot : Slots)
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

void UInventoryComponent::PrintInventoryInfo()
{
	for (int i= 0; i < Slots.Max(); i++)
	{
		if (Slots[i].IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("[%d] 슬롯 : [%s] %d개"), 
				i, 
				*(Slots[i].ItemData->DisplayName.ToString()),
				Slots[i].StackCount
			);
		}
	}
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 빈 FItemInstance(ItemData = nullptr)로 채워진 고정 슬롯을 만든다.
	// FItemInstance::IsValid()가 ItemData == nullptr일 때 false를 반환하므로
	// "빈 슬롯" 판정에 별도 플래그가 필요 없다.
	Slots.SetNum(SlotCount);
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

