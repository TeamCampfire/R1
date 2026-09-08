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
#include "DrawDebugHelpers.h"

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
	FVector TraceCenter = OwnerCharacter->GetActorLocation() + Forward * FillTraceDistance;
	TraceCenter.Z += FillTraceHeightOffset;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.bTraceComplex = false;

	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(FillTraceRadius);

	// AFishingRod::UpdateCastingTrajectory와 동일한 방식 — WaterBodyCollision 프로파일로 먼저
	// 검사하고, 못 찾으면 ECC_WorldDynamic 채널로 한 번 더(월드 세팅에 따라 프로파일이 없을 수 있어
	// 백업 경로로 둔다). "물"로 인정하는 기준도 FishingRod와 동일하게 실제 AWaterBody 액터이거나,
	// 이름에 "Water"가 들어간 액터(테스트 맵에서 흔한, Water 플러그인 없이 콜리전 프리셋만
	// WaterBodyCollision으로 맞춘 순수 StaticMeshActor — 예: Lv_R1Alpha의 SM_WaterMesh)까지 허용한다.
	// GetName()(내부 오브젝트 이름)뿐 아니라 에디터 아웃라이너 라벨(GetActorLabel(), 나중에 F2로
	// 리네임한 경우 내부 이름과 달라질 수 있음)도 같이 검사해서 리네임된 액터도 잡히게 한다.
	auto IsWaterActor = [](const AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}
#if WITH_EDITOR
		return Actor->IsA(AWaterBody::StaticClass())
			|| Actor->GetName().Contains(TEXT("Water"))
			|| Actor->GetActorLabel().Contains(TEXT("Water"));
#else
		return Actor->IsA(AWaterBody::StaticClass()) || Actor->GetName().Contains(TEXT("Water"));
#endif
	};

	// 디버그용 — 판정 범위 안에서 실제로 뭐가 걸리는지 화면에 그대로 찍는다(원인 파악용).
	// 아무것도 안 찍히면 위치/콜리전 문제, 뭔가 찍히는데 물로 인식이 안 되면 이름/타입 문제.
	auto LogOverlap = [](const TCHAR* Pass, const FOverlapResult& Overlap, bool bMatched)
	{
		if (!GEngine)
		{
			return;
		}
		GEngine->AddOnScreenDebugMessage(-1, 3.f, bMatched ? FColor::Green : FColor::Silver,
			FString::Printf(TEXT("[WaterCheck][%s] %s (matched=%d)"), Pass, *GetNameSafe(Overlap.GetActor()), bMatched));
	};

	bool bFoundWater = false;

	TArray<FOverlapResult> ProfileOverlaps;
	GetWorld()->OverlapMultiByProfile(ProfileOverlaps, TraceCenter, FQuat::Identity, FName(TEXT("WaterBodyCollision")), SphereShape, QueryParams);

	for (const FOverlapResult& Overlap : ProfileOverlaps)
	{
		const bool bMatched = IsWaterActor(Overlap.GetActor());
		LogOverlap(TEXT("Profile"), Overlap, bMatched);
		if (bMatched)
		{
			bFoundWater = true;
		}
	}

	if (!bFoundWater)
	{
		TArray<FOverlapResult> DynamicOverlaps;
		GetWorld()->OverlapMultiByChannel(DynamicOverlaps, TraceCenter, FQuat::Identity, ECC_WorldDynamic, SphereShape, QueryParams);

		for (const FOverlapResult& Overlap : DynamicOverlaps)
		{
			const bool bMatched = IsWaterActor(Overlap.GetActor());
			LogOverlap(TEXT("Dynamic"), Overlap, bMatched);
			if (bMatched)
			{
				bFoundWater = true;
			}
		}

		if (ProfileOverlaps.Num() == 0 && DynamicOverlaps.Num() == 0 && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("[WaterCheck] 두 패스 모두 아무 것도 안 겹침 — 위치/사거리 문제"));
		}
	}

	// 디버그용 — 채우기 판정 범위(전방 FillTraceDistance 지점, 반경 FillTraceRadius)를 시각화한다.
	// 초록 = 물 감지됨, 빨강 = 못 찾음. Server_FillWater_Implementation이 서버에서만 실행되므로
	// 서버(리슨 서버 포함) 화면에서만 보인다. 테스트 끝나면 이 블록은 지워도 된다.
	DrawDebugSphere(GetWorld(), TraceCenter, FillTraceRadius, 16, bFoundWater ? FColor::Green : FColor::Red, false, 2.0f, 0, 1.5f);

	return bFoundWater;
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
