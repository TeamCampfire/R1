/// 최초작성 : 2026.09.07
/// 작 성 자 : 최 요 환

#include "Item/HeldItem/WaterBottle.h"
#include "Character/ActionCharacter.h"
#include "Component/InventoryComponent.h"
#include "Component/StatComponent.h"
#include "Data/Item/HeldItemData.h"
#include "Data/Item/ItemTypes.h"
#include "WaterBodyActor.h"
#include "Engine/OverlapResult.h"

AWaterBottle::AWaterBottle()
{
}

bool AWaterBottle::IsFilled() const
{
	return GetItemData() && GetItemData() == FilledBottleData;
}

void AWaterBottle::OnPrimaryActionStarted()
{
	if (!IsFilled())
	{
		// 빈 병은 마실 게 없다 — 몽타주도 재생하지 않는다.
		return;
	}

	// ItemData(FilledBottleData)에 PrimaryMontage가 지정돼 있으면 베이스가 재생해준다.
	Super::OnPrimaryActionStarted();

	Server_DrinkWater();
}

void AWaterBottle::OnSecondaryActionStarted()
{
	if (IsFilled())
	{
		// 이미 찬 병은 또 채울 필요가 없다.
		return;
	}

	Super::OnSecondaryActionStarted();

	Server_FillWater();
}

bool AWaterBottle::Server_DrinkWater_Validate()
{
	return true;
}

void AWaterBottle::Server_DrinkWater_Implementation()
{
	if (!IsFilled() || !OwnerCharacter)
	{
		return;
	}

	if (UStatComponent* StatComp = OwnerCharacter->GetStatComponent())
	{
		FItemEffect ThirstEffect;
		ThirstEffect.EffectType = EItemEffectType::RestoreThirst;
		ThirstEffect.Magnitude = ThirstRestoreAmount;
		StatComp->ApplyItemEffect(ThirstEffect);
	}

	SwapBottleState(EmptyBottleData);
}

bool AWaterBottle::Server_FillWater_Validate()
{
	return true;
}

void AWaterBottle::Server_FillWater_Implementation()
{
	if (IsFilled() || !OwnerCharacter)
	{
		return;
	}

	if (!TraceForWaterBody())
	{
		return;
	}

	SwapBottleState(FilledBottleData);
}

bool AWaterBottle::TraceForWaterBody() const
{
	if (!OwnerCharacter || !GetWorld())
	{
		return false;
	}

	const FVector Forward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	const FVector TraceCenter = OwnerCharacter->GetActorLocation() + Forward * FillTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.bTraceComplex = false;

	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(FillTraceRadius);

	// AFishingRod::UpdateCastingTrajectory와 동일한 방식 — WaterBodyCollision 프로파일로 먼저
	// 검사하고, 못 찾으면 ECC_WorldDynamic 채널로 한 번 더(월드 세팅에 따라 프로파일이 없을 수 있어
	// 백업 경로로 둔다).
	TArray<FOverlapResult> ProfileOverlaps;
	GetWorld()->OverlapMultiByProfile(ProfileOverlaps, TraceCenter, FQuat::Identity, FName(TEXT("WaterBodyCollision")), SphereShape, QueryParams);

	for (const FOverlapResult& Overlap : ProfileOverlaps)
	{
		if (Overlap.GetActor() && Overlap.GetActor()->IsA(AWaterBody::StaticClass()))
		{
			return true;
		}
	}

	TArray<FOverlapResult> DynamicOverlaps;
	GetWorld()->OverlapMultiByChannel(DynamicOverlaps, TraceCenter, FQuat::Identity, ECC_WorldDynamic, SphereShape, QueryParams);

	for (const FOverlapResult& Overlap : DynamicOverlaps)
	{
		if (Overlap.GetActor() && Overlap.GetActor()->IsA(AWaterBody::StaticClass()))
		{
			return true;
		}
	}

	return false;
}

void AWaterBottle::SwapBottleState(UHeldItemData* NewData)
{
	if (!NewData || !OwnerCharacter)
	{
		return;
	}

	UInventoryComponent* Inventory = OwnerCharacter->GetInventoryComponent();
	if (!Inventory || Inventory->HeldBeltIndex == INDEX_NONE || !Inventory->BeltSlots.IsValidIndex(Inventory->HeldBeltIndex))
	{
		return;
	}

	const int32 BeltIndex = Inventory->HeldBeltIndex;
	const FItemInstance& HeldInstance = Inventory->BeltSlots[BeltIndex];

	// 지금 손에 든 벨트 슬롯이 확실히 "이 병"인지 확인 — 다른 벨트 슬롯을 잘못 건드리지 않게.
	if (HeldInstance.ItemData != GetItemData())
	{
		return;
	}

	// 같은 물리적 병의 상태만 바뀌는 것이므로 InstanceID는 유지하고 ItemData/StackCount만 바꾼다.
	FItemInstance NewInstance;
	NewInstance.ItemData = NewData;
	NewInstance.StackCount = 1;
	NewInstance.InstanceID = HeldInstance.InstanceID;

	Inventory->SetSlotItem(FInventorySlotRef{ EInventorySlotCategory::Belt, BeltIndex }, NewInstance);
}
