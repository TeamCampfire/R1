/// 최초작성 : 2026.09.09
/// 작 성 자 : 최 요 환

#include "Item/HeldItem/Bandage.h"
#include "Character/ActionCharacter.h"
#include "Component/InventoryComponent.h"
#include "Component/StatComponent.h"
#include "Data/Item/HeldItemData.h"
#include "Data/Item/ItemTypes.h"
#include "DrawDebugHelpers.h"

ABandage::ABandage()
{
}

void ABandage::OnPrimaryActionStarted()
{
	if (!GetItemData() || !OwnerCharacter)
	{
		return;
	}

	// 몽타주 재생(로컬 선행 + 서버/멀티캐스트 동기화)은 베이스가 처리한다.
	Super::OnPrimaryActionStarted();

	Server_UseBandage();
}

bool ABandage::Server_UseBandage_Validate()
{
	return true;
}

void ABandage::Server_UseBandage_Implementation()
{
	if (!GetItemData() || !OwnerCharacter)
	{
		return;
	}

	if (UStatComponent* StatComp = OwnerCharacter->GetStatComponent())
	{
		for (const FItemEffect& Effect : GetItemData()->Effects)
		{
			StatComp->ApplyItemEffect(Effect);
		}
	}

	ConsumeOneCharge();
}

void ABandage::OnSecondaryActionStarted()
{
	if (!GetItemData() || !OwnerCharacter)
	{
		return;
	}

	// SecondaryMontage가 지정돼 있으면 베이스가 재생해준다(현재는 미지정 — 빈칸으로 둠).
	Super::OnSecondaryActionStarted();

	Server_UseBandageOnTarget();
}

bool ABandage::Server_UseBandageOnTarget_Validate()
{
	return true;
}

void ABandage::Server_UseBandageOnTarget_Implementation()
{
	if (!GetItemData() || !OwnerCharacter)
	{
		return;
	}

	// 클라이언트가 보낸 대상을 신뢰하지 않고 서버에서 직접 조준 판정을 다시 수행한다
	// (AWaterBottle::Server_FillWater_Implementation과 동일한 이유).
	AActionCharacter* Target = TraceForTargetCharacter();
	if (!Target)
	{
		return;
	}

	if (UStatComponent* StatComp = Target->GetStatComponent())
	{
		for (const FItemEffect& Effect : GetItemData()->Effects)
		{
			StatComp->ApplyItemEffect(Effect);
		}
	}

	ConsumeOneCharge();
}

AActionCharacter* ABandage::TraceForTargetCharacter() const
{
	if (!OwnerCharacter || !GetWorld())
	{
		return nullptr;
	}

	const FVector Start = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	const FVector End = Start + Forward * TargetTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.bTraceComplex = false;

	// 정밀한 라인트레이스 대신 스피어 스윕을 써서 조준이 살짝 빗나가도 맞은 것으로 인정한다
	// (조준 편의성 — 이번 설계에서 요청된 사항).
	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(TargetTraceRadius);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity, ObjectQueryParams, SphereShape, QueryParams);
	AActionCharacter* TargetCharacter = bHit ? Cast<AActionCharacter>(Hit.GetActor()) : nullptr;

	// 디버그용 — AWaterBottle::TraceForWaterBody와 동일한 방식. 스윕 경로(Start~End)를 선으로,
	// 시작/끝 지점을 판정 반경 그대로의 구체로 그린다. 초록 = 대상(AActionCharacter) 발견,
	// 빨강 = 못 찾음(다른 걸 맞았거나 아예 안 맞음). Server_UseBandageOnTarget이 서버에서만
	// 이 함수를 호출하므로 서버(리슨 서버 포함) 화면에서만 보인다. IsPlayInEditor()는 에디터
	// PIE 세션에서만 true라 패키징된 빌드(Standalone/Shipping)에서는 이 블록이 실행되지 않는다.
	if (GetWorld()->IsPlayInEditor())
	{
		const FColor DebugColor = TargetCharacter ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), Start, End, DebugColor, false, 2.0f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), Start, TargetTraceRadius, 16, DebugColor, false, 2.0f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), End, TargetTraceRadius, 16, DebugColor, false, 2.0f, 0, 1.5f);
	}

	return TargetCharacter;
}

void ABandage::ConsumeOneCharge()
{
	UInventoryComponent* Inventory = OwnerCharacter ? OwnerCharacter->GetInventoryComponent() : nullptr;
	if (!Inventory || Inventory->HeldBeltIndex == INDEX_NONE || !Inventory->BeltSlots.IsValidIndex(Inventory->HeldBeltIndex))
	{
		return;
	}

	const int32 BeltIndex = Inventory->HeldBeltIndex;
	const FItemInstance& HeldInstance = Inventory->BeltSlots[BeltIndex];

	// 지금 손에 든 벨트 슬롯이 확실히 "이 붕대"인지 확인 — 다른 벨트 슬롯을 잘못 건드리지 않게
	// (AWaterBottle::SwapBottleState와 동일한 방어 로직).
	if (HeldInstance.ItemData != GetItemData())
	{
		return;
	}

	const int32 Remaining = HeldInstance.StackCount - 1;
	Inventory->SetSlotItem(FInventorySlotRef{ EInventorySlotCategory::Belt, BeltIndex },
		Remaining > 0 ? FItemInstance(HeldInstance.ItemData, Remaining) : FItemInstance());
}
