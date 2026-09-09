#include "Item/PlaceableItem/Campfire/CampfireComponent.h"

#include "Component/InventoryComponent.h"
#include "Data/Campfire/CampfireConfigDataAsset.h"
#include "Data/Item/ItemDataBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UCampfireComponent::UCampfireComponent()
{
	SetIsReplicatedByDefault(true);	// Replicate 기본 설정
	PrimaryComponentTick.bCanEverTick = false;	// Tick 대신 타이머 사용
}

void UCampfireComponent::BeginPlay()
{
	Super::BeginPlay();

	OutputSlots.SetNum(2);	// 산출 슬롯은 2개로 지정

	// 서버에서 모닥불 전용 Tick 타이머 시작
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(CampfireTimer, this, &UCampfireComponent::TickCampfire, UpdateInterval, true);
	}
}

void UCampfireComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 각 슬롯들과 모닥불 상태는 리플리케이트되어야 함
	DOREPLIFETIME(UCampfireComponent, InputSlot);
	DOREPLIFETIME(UCampfireComponent, FuelSlot);
	DOREPLIFETIME(UCampfireComponent, OutputSlots);
	DOREPLIFETIME(UCampfireComponent, bIsLit);
	DOREPLIFETIME(UCampfireComponent, RemainingFuelTime);
	DOREPLIFETIME(UCampfireComponent, CurrentFuelDuration);
	DOREPLIFETIME(UCampfireComponent, CurrentCookingTime);
}

// 슬롯의 읽기 전용 포인터 반환
const FItemInstance* UCampfireComponent::FindSlot(const FCampfireSlotRef& Slot) const
{
	// 슬롯 종류별로 유효한 인덱스를 가졌으면 슬롯 아이템 읽기 전용 포인터 반환
	switch (Slot.Type)
	{
	case ECampfireSlotType::Input:
		return Slot.Index == 0 ? &InputSlot : nullptr;

	case ECampfireSlotType::Fuel:
		return Slot.Index == 0 ? &FuelSlot : nullptr;

	case ECampfireSlotType::Output:
		return OutputSlots.IsValidIndex(Slot.Index) ? &OutputSlots[Slot.Index] : nullptr;

	default: return nullptr;
	}
}

// 슬롯의 수정 가능한 포인터 반환
FItemInstance* UCampfireComponent::FindMutableSlot(const FCampfireSlotRef& Slot)
{
	return const_cast<FItemInstance*>(
		const_cast<const UCampfireComponent*>(this)->FindSlot(Slot)
	);
}

// 슬롯 찾으면 찾은 거 주고, 못 찾았으면 빈 아이템 반환
FItemInstance UCampfireComponent::GetSlot(const FCampfireSlotRef& Slot) const
{
	const FItemInstance* Found = FindSlot(Slot);
	return Found ? *Found : FItemInstance();
}

// 아이템을 올릴 수 있는 슬롯인지 확인
bool UCampfireComponent::CanAcceptItem(const FCampfireSlotRef& Slot, const UItemDataBase* Item) const
{
	if (!Config || !Item || Slot.Index != 0)
	{
		return false;
	}

	// 요리, 연료 레시피 기준으로 확인
	return (Slot.Type == ECampfireSlotType::Input && Config->FindCookingRecipe(Item))
		|| (Slot.Type == ECampfireSlotType::Fuel && Config->FindFuelRecipe(Item));
}

// 재료 굽기 진행도
float UCampfireComponent::GetCookingProgress() const
{
	return Config && Config->CookingTime > 0.f
		? FMath::Clamp(CurrentCookingTime / Config->CookingTime, 0.f, 1.f) : 0.f;
}

// 현재 연료의 남은 연소 시간 비율
float UCampfireComponent::GetFuelProgress() const
{
	return CurrentFuelDuration > 0.f
		? FMath::Clamp(RemainingFuelTime / CurrentFuelDuration, 0.f, 1.f) : 0.f;
}

// 사용 가능한 산출 슬롯 찾기
// 연료 부산물과 요리 결과가 출력 슬롯 두 칸을 공유하며, 같은 아이템의 기존 스택을 우선으로 채움
int32 UCampfireComponent::FindOutputSlot(UItemDataBase* Item) const
{
	if (!Item || Item->MaxStackSize <= 0)
		return INDEX_NONE;

	int32 EmptyIndex = INDEX_NONE;
	for (int32 i = 0; i < OutputSlots.Num(); ++i)
	{
		const FItemInstance& Output = OutputSlots[i];
		if (!Output.IsValid())
		{
			// 빈 슬롯 발견
			if (EmptyIndex == INDEX_NONE)
				EmptyIndex = i;
		}
		else if (Output.ItemData == Item && Output.StackCount < Item->MaxStackSize)
		{
			// 최대 스택에 도달하지 않은 같은 종류의 아이템이 있는 슬롯 발견
			return i;
		}
	}
	return EmptyIndex;
}

// 산출 슬롯에 연료 부산물, 굽기 결과물 넣기
bool UCampfireComponent::AddOutput(UItemDataBase* Item)
{
	const int32 OutputIndex = FindOutputSlot(Item);	// 사용 가능한 산출 슬롯 찾기

	// 사용 가능한 산출 슬롯 못 찾았으면 실패 처리
	if (OutputIndex == INDEX_NONE)
		return false;

	FItemInstance& Output = OutputSlots[OutputIndex];
	if (Output.IsValid())
	{
		// 이미 쌓인 스택이 있으면 스택++
		++Output.StackCount;
	}
	else
	{
		// 완전히 빈 슬롯이었으면 아이템 1개 생성, 저장
		Output = FItemInstance(Item, 1);
	}

	return true;
}

// 조리 진행도와 연결된 아이템이 조리 입력 슬롯에 있는 아이템이랑 같은지 확인
void UCampfireComponent::SyncProgressItems()
{
	// 입력 슬롯이 비었거나, 진행도와 연결 아이템과 조리 입력 슬롯의 아이템이 다르면 굽기 진행 초기화
	if (!InputSlot.IsValid() || ProgressCookingItem != InputSlot.ItemData)
	{
		ProgressCookingItem = InputSlot.IsValid() ? InputSlot.ItemData : nullptr;
		CurrentCookingTime = 0.f;
	}
}

// 연료 소모
// 연료는 연소 시작 시 한 개 차감하고, 남은 연소 시간과 부산물을 슬롯 재고와 별도로 보관
bool UCampfireComponent::PrepareFuel()
{
	// 진행도-아이템 관계 확인
	SyncProgressItems();

	// 시작 시 이미 차감한 연료는 슬롯의 남은 연료와 독립적으로 연소
	// 0초까지 탄 채 Output 공간을 기다리는 경우에도 추가 연료를 소비하지 않음
	if (CurrentFuelDuration > 0.f)
		return true;

	// 설정이나 대기 중인 연료가 없으면 새 연료 준비 실패
	if (!Config || !FuelSlot.IsValid())
		return false;

	// 사용 가능한 연료가 아니면 연료 준비 실패
	const FCampfireFuelRecipe* FuelRecipe = Config->FindFuelRecipe(FuelSlot.ItemData);
	if (!FuelRecipe)
		return false;

	PendingFuelOutputItem = FuelRecipe->AfterItem;	// 연료 부산물 기억

	CurrentFuelDuration = FMath::Max(0.1f, Config->BurningTime);	// 연료 연소 시간 설정
	RemainingFuelTime = CurrentFuelDuration;	// 연소 시 차감할 변수에 연소 시간 전달

	--FuelSlot.StackCount;	// 연료 슬롯에서 연료 개수 하나 차감
	// 연료 슬롯에 있는 거 다 썼으면 빈 슬롯으로 설정
	if (FuelSlot.StackCount <= 0)
		FuelSlot = FItemInstance();

	return true;
}

// 연소 중인 연료 연소 완료 처리
bool UCampfireComponent::CompleteCurrentFuel()
{
	// 연료를 전부 소진했는지 검사
	if (CurrentFuelDuration <= 0.f || RemainingFuelTime > 0.f)
		return false;

	// 산출 슬롯에 연료 부산물을 넣을 수 있는지 검사
	if (PendingFuelOutputItem && !AddOutput(PendingFuelOutputItem))
		return false;

	// 연소 완료 처리
	PendingFuelOutputItem = nullptr;
	CurrentFuelDuration = 0.f;
	RemainingFuelTime = 0.f;
	return true;
}

// 서버에서 실제 연소한 시간만큼 요리를 진행
// 결과물 추가에 성공해야 입력 재료를 차감
void UCampfireComponent::TickCampfire()
{
	// 서버인지, 모닥불이 켜져 있는지, 레시피 데이터가 있는지 확인
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bIsLit || !Config)
		return;

	// 연료 소비 시도
	// -> 실패: 모닥불 끄고 return;
	if (!PrepareFuel())
	{
		bIsLit = false;
		NotifyStateChanged();
		return;
	}

	// -> 성공: 연소

	// 연료 연소 시간 관리
	const float BurningDelta = FMath::Min(RemainingFuelTime, UpdateInterval);
	RemainingFuelTime = FMath::Max(0.f, RemainingFuelTime - BurningDelta);

	// 입력 슬롯에 들어온 아이템의 레시피 확인
	const FCampfireCookingRecipe* CookingRecipe = InputSlot.IsValid()
		? Config->FindCookingRecipe(InputSlot.ItemData) : nullptr;

	// 유효하지 않은 레시피거나 산출 슬롯이 포화 상태면 조리 불가 처리
	const bool bCanCook = CookingRecipe && CookingRecipe->AfterItem && FindOutputSlot(CookingRecipe->AfterItem) != INDEX_NONE;

	// 조리
	if (bCanCook)
	{
		CurrentCookingTime += BurningDelta;	// 조리 시간 증가

		// 정해진 조리 시간 도달하면 결과물 뱉고 조리 시간 초기화
		if (CurrentCookingTime >= FMath::Max(0.1f, Config->CookingTime))
		{
			if (AddOutput(CookingRecipe->AfterItem))
			{
				--InputSlot.StackCount;
				if (InputSlot.StackCount <= 0) InputSlot = FItemInstance();
				CurrentCookingTime = 0.f;
			}
		}
	}

	// 연료 연소 완료
	// 산출 슬롯 포화 시 결과물 보류
	if (RemainingFuelTime <= 0.f)
	{
		// 부산물 출력 공간이 없거나 다음 연료가 없으면 모닥불 소화
		if (!CompleteCurrentFuel() || !FuelSlot.IsValid())
			bIsLit = false;
	}

	NotifyStateChanged();
}

// 모닥불 점화 상태 설정
void UCampfireComponent::SetLit(bool bNewLit)
{
	// 모닥불 점화 상태는 서버가 관리
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	// 모닥불 켜려는데 연료 없으면 점화 실패 처리
	if (bNewLit && !PrepareFuel())
	{
		bIsLit = false;
		NotifyStateChanged();
		return;
	}

	bIsLit = bNewLit;
	NotifyStateChanged();
}

void UCampfireComponent::NotifyStateChanged()
{
	SyncProgressItems();	// 진행도와 아이템 일치 여부 처리

	OnCampfireStateChanged.Broadcast();	// 모닥불 상태 변경 델리게이트 발동

	// 모닥불 상태 변경이 클라이언트에 빠르게 전달되도록 네트워크 업데이트 요청
	if (AActor* Owner = GetOwner())
		Owner->ForceNetUpdate();
}

void UCampfireComponent::OnRep_State()
{
	OnCampfireStateChanged.Broadcast();
}

/* 아이템 이동: 인벤토리 -> 모닥불 */
// 서버에서 처리
// 허용 레시피와 스택 여유를 검사
bool UCampfireComponent::MoveFromInventory(UInventoryComponent* Inventory, const FInventorySlotRef& From, const FCampfireSlotRef& To, int32 Count, bool bHalfSplit)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Inventory)
		return false;

	// 인벤토리에 있는 아이템 배열 읽기
	TArray<FItemInstance>& SourceArray = Inventory->GetSlotArray(From.Category);
	if (!SourceArray.IsValidIndex(From.Index) || !SourceArray[From.Index].IsValid())
		return false;

	// 모닥불에 올리려는 아이템 기억 및  허용되는 아이템인지 검사
	const FItemInstance Source = SourceArray[From.Index];
	if (!CanAcceptItem(To, Source.ItemData))
		return false;

	// 사용할 모닥불 슬롯 기억
	FItemInstance* Target = FindMutableSlot(To);
	if (!Target)
		return false;

	int32 MoveCount = Count > 0 ? FMath::Min(Count, Source.StackCount) : Source.StackCount;	// 옮길 아이템 스택 수 기억

	// 절반 나눠서 옮기는 거면 절반 수량으로 수정
	if (bHalfSplit && Count <= 0 && !Target->IsValid() && Source.StackCount >= 2)
		MoveCount = Source.StackCount / 2;

	// MoveCount가 너무 많으면 최대로 쌓을 수 있는 만큼만 기억
	if (!Target->IsValid())
		MoveCount = FMath::Min(MoveCount, Source.ItemData->MaxStackSize);

	// 목표 슬롯에 아이템이 있는지 확인
	if (Target->IsValid())
	{
		// 목표 슬롯에 이미 다른 종류의 아이템이 들어 있으면 실패 처리
		if (Target->ItemData != Source.ItemData)
			return false;

		// 목표 슬롯에 이미 있는 아이템 수까지 고려해서 실제로 넣을 수 있는 만큼만 넣기
		MoveCount = FMath::Min(MoveCount, Source.ItemData->MaxStackSize - Target->StackCount);
	}

	// 목표 슬롯에 넣을 수 있는 자리가 없으면 실패 처리
	if (MoveCount <= 0)
		return false;

	// 목표 슬롯에 아이템 있으면 스택 수만 증가
	if (Target->IsValid())
	{
		Target->StackCount += MoveCount;
	}
	// 목표 슬롯이 비었으면 옮길 만큼 아이템 등록
	else
	{
		*Target = FItemInstance(Source.ItemData, MoveCount);
	}

	// 원래 슬롯에서 아이템 개수 차감
	FItemInstance Remaining = Source;
	Remaining.StackCount -= MoveCount;
	Inventory->SetSlot(From.Category, From.Index, Remaining.StackCount > 0 ? Remaining : FItemInstance());

	NotifyStateChanged();
	return true;
}

// 아이템 이동: 모닥불 -> 인벤토리
// 서버에서 실행
bool UCampfireComponent::MoveToInventory(UInventoryComponent* Inventory, const FCampfireSlotRef& From, const FInventorySlotRef& To, int32 Count, bool bHalfSplit)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Inventory)
		return false;

	FItemInstance* SourcePtr = FindMutableSlot(From);

	// 장비 슬롯으로 이동은 실패 처리
	if (!SourcePtr || !SourcePtr->IsValid() || To.Category == EInventorySlotCategory::Equipment)
		return false;

	TArray<FItemInstance>& TargetArray = Inventory->GetSlotArray(To.Category);
	if (!TargetArray.IsValidIndex(To.Index))
		return false;

	const FItemInstance Source = *SourcePtr;
	const FItemInstance Target = TargetArray[To.Index];

	int32 MoveCount = Count > 0 ? FMath::Min(Count, Source.StackCount) : Source.StackCount;

	// 절반 옵션 처리
	if (bHalfSplit && Count <= 0 && !Target.IsValid() && Source.StackCount >= 2)
		MoveCount = Source.StackCount / 2;

	// 아이템 최대 스택 수 안 넘게
	if (!Target.IsValid())
		MoveCount = FMath::Min(MoveCount, Source.ItemData->MaxStackSize);

	// 다른 종류 아이템 있으면 실패 처리
	// 같은 종류 아이템이면 여유 스택만큼만 이동
	if (Target.IsValid())
	{
		if (Target.ItemData != Source.ItemData)
			return false;

		MoveCount = FMath::Min(MoveCount, Source.ItemData->MaxStackSize - Target.StackCount);
	}

	// 목표 슬롯에 여유 공간이 없으면 실패 처리
	if (MoveCount <= 0)
		return false;

	// 아이템 이동
	FItemInstance NewTarget = Target.IsValid() ? Target : FItemInstance(Source.ItemData, 0);
	NewTarget.StackCount += MoveCount;
	Inventory->SetSlot(To.Category, To.Index, NewTarget);

	// 아이템 차감, 스택 다 차감했으면 슬롯 비우기
	SourcePtr->StackCount -= MoveCount;
	if (SourcePtr->StackCount <= 0)
		*SourcePtr = FItemInstance();

	NotifyStateChanged();
	return true;
}

// 아이템 우클릭 이동: 인벤토리 -> 모닥불
// 우클릭한 아이템의 레시피를 기준으로 요리 입력 또는 연료 슬롯을 자동 선택
bool UCampfireComponent::QuickMoveFromInventory(UInventoryComponent* Inventory, const FInventorySlotRef& From)
{
	if (!Inventory) return
		false;

	const TArray<FItemInstance>& Slots = Inventory->GetSlotArray(From.Category);
	if (!Slots.IsValidIndex(From.Index) || !Slots[From.Index].IsValid())
		return false;

	// 요리 입력인지 연료 입력인지 판별
	const UItemDataBase* Item = Slots[From.Index].ItemData;
	const FCampfireSlotRef Target =
		Config && Config->FindCookingRecipe(Item)
		? FCampfireSlotRef{ ECampfireSlotType::Input, 0 }
		: FCampfireSlotRef{ ECampfireSlotType::Fuel, 0 }
	;

	return CanAcceptItem(Target, Item) && MoveFromInventory(Inventory, From, Target, 0, false);
}

// 아이템 우클릭 이동: 모닥불 -> 인벤토리
// 우클릭 회수는 메인 인벤토리의 같은 아이템 스택을 먼저 채운 뒤 빈 슬롯을 찾음
bool UCampfireComponent::QuickMoveToInventory(UInventoryComponent* Inventory, const FCampfireSlotRef& From)
{
	if (!Inventory || !GetSlot(From).IsValid())
		return false;

	const FItemInstance Source = GetSlot(From);
	for (int32 i = 0; i < Inventory->MainSlots.Num(); ++i)
	{
		const FItemInstance& Slot = Inventory->MainSlots[i];

		// 같은 아이템 스택의 여유 공간에 이동시킬 때
		if (Slot.IsValid() && Slot.ItemData == Source.ItemData && Slot.StackCount < Source.ItemData->MaxStackSize)
		{
			// 아이템 이동 성공하면 이동 성공 처리
			if (MoveToInventory(Inventory, From, { EInventorySlotCategory::Main, i }, 0, false) && !GetSlot(From).IsValid())
				return true;
		}
	}

	// 빈 슬롯에 이동시킬 때
	for (int32 i = 0; i < Inventory->MainSlots.Num(); ++i)
	{
		if (!Inventory->MainSlots[i].IsValid())
			return MoveToInventory(Inventory, From, { EInventorySlotCategory::Main, i }, 0, false);
	}

	return false;
}
