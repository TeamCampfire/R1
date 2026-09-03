#include "Component/HarvestableComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Character/ActionCharacter.h"
#include "Components/DecalComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "Item/ItemPickup.h"
#include "NiagaraFunctionLibrary.h"

// Sets default values for this component's properties
UHarvestableComponent::UHarvestableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ItemPickupClass = AItemPickup::StaticClass();
	SetIsReplicatedByDefault(true);
	SweetSpotTransform = FTransform::Identity;
}

void UHarvestableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHarvestableComponent, SweetSpotTransform);
}

void UHarvestableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* MyOwner = GetOwner())
	{
		// 서버일 때만 복제 켜기를 실행하도록 가드!
		if (MyOwner->HasAuthority())
		{
			MyOwner->SetReplicates(true);
		}
	}
}

FHarvestRes UHarvestableComponent::OnHitted_Implementation(AActionCharacter* InCharacter, const FVector& HitLocation)
{
	// 서버에서만 채집 로직 및 자원 감소/지급을 수행
	if (!GetOwner()->HasAuthority()) return FHarvestRes();

	FHarvestRes Res;
	if (CurrentHp <= 0 || !InCharacter) return Res;

	// 1. 체력 감소 (임시 50)
	CurrentHp -= 50.f;

	// 2. 피격 피드백(사운드/FX/데칼) 및 스위트스팟 배율 산출
	const float YieldMultiplier = ProcessHitFeedback(InCharacter, HitLocation, Res.bHitSweetSpot);

	// 3. 고갈(파괴) 조건 판정 (Rust 방식)
	// - 스위트스팟을 쓰는 오브젝트: 피가 다 닳아도 '스위트스팟(X자)을 쳐야만' 최종 파괴 & 보너스 수확!
	// - 스위트스팟을 안 쓰는 오브젝트: 피가 다 닳으면 즉시 파괴 & 보너스 수확!
	const bool bCanDeplete = (CurrentHp <= 0) && (!bUseSweetSpot || Res.bHitSweetSpot);

	if (bCanDeplete)
	{
		CurrentHp = 0.f;
		Res.bIsDepleted = true;

		// 고갈 보너스 수확 (드럼통은 바닥 드랍, 나무/돌은 인벤토리)
		ProcessDepletion(YieldMultiplier, Res.HarvestedItems);

		IHarvestable::Execute_OnHarvestEnd(this);
	}
	else
	{
		// 피가 다 닳았는데 스위트스팟을 안 쳐서 안 부서진 경우: 피를 최소치(1.0f)로 유지하여 쓰러지지 않게 함
		if (CurrentHp <= 0)
		{
			CurrentHp = 1.0f;
		}

		// 일반 타격 아이템 수확
		CollectYieldItems(HarvestYields, YieldMultiplier, Res.HarvestedItems);
	}

	Res.HarvesResult = (Res.HarvestedItems.Num() > 0 || Res.bIsDepleted);
	return Res;
}

float UHarvestableComponent::ProcessHitFeedback(AActionCharacter* InCharacter, const FVector& HitLocation, bool& bOutHitSweetSpot)
{
	bOutHitSweetSpot = false;
	float Multiplier = 1.0f;

	// 스위트스팟 적중 판정 (서버의 SweetSpotTransform 위치 기준)
	if (bUseSweetSpot)
	{
		// 아직 스위트스팟이 생성되지 않은 첫 타격인 경우
		if (SweetSpotTransform.GetLocation().IsNearlyZero())
		{
			GenerateSweetSpot_Server();
		}
		else
		{
			// 이미 스위트스팟이 존재하는 경우 거리 비교
			float DistSqr = FVector::DistSquared(HitLocation, SweetSpotTransform.GetLocation());
			if (DistSqr <= FMath::Square(SweetSpotHitRadius))
			{
				bOutHitSweetSpot = true;
				Multiplier = BounusRate;
				GenerateSweetSpot_Server(); // 적중 성공 시 다음 스위트스팟 위치로 이동
			}
		}
	}

	// 모든 클라이언트(및 호스트)에 타격 FX/사운드/임팩트데칼 브로드캐스트
	FRotator DecalRot = (InCharacter->GetActorLocation() - HitLocation).Rotation();
	Multicast_PlayHitEffects(HitLocation, bOutHitSweetSpot, DecalRot);

	return Multiplier;
}

void UHarvestableComponent::Multicast_PlayHitEffects_Implementation(const FVector& HitLocation, bool bIsSweetSpot, const FRotator& DecalRot)
{
	// 1. 임팩트 데칼 소환
	SpawnImpactDecal_Implementation(HitLocation, DecalRot);

	// 2. FX 및 사운드 재생
	if (bIsSweetSpot)
	{
		if (SweetSpotHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), SweetSpotHitSound, HitLocation);
		}
		if (SweetSpotNiagaraFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SweetSpotNiagaraFX, HitLocation);
		}
	}
	else
	{
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, HitLocation);
		}
		if (HitNiagaraFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitNiagaraFX, HitLocation);
		}
	}
}

void UHarvestableComponent::GenerateSweetSpot_Server()
{
	if (!GetOwner()->HasAuthority()) return;

	FVector SpawnPos = FVector::ZeroVector;
	FRotator SpawnRot = FRotator::ZeroRotator;
	FVector ActorCenter = GetOwner()->GetActorLocation();

	// 초기 생성
	if (SweetSpotTransform.GetLocation().IsNearlyZero())
	{
		// 0. 액터의 원점에서 랜덤한 높이 선정
		ActorCenter.Z += FMath::FRandRange(SweetSpotMinHeight, SweetSpotMaxHeight);

		// 1. 랜덤한 방향 선정 (yaw)
		float RandAngle = FMath::FRandRange(0.f, 360.f);
		FVector Dir = FRotator(0.0, RandAngle, 0.0).Vector();

		// 2. 라인트레이스를 통해 표면 충돌 지점 검출
		FVector TraceStart = ActorCenter + Dir * SweetSpotTraceDistance;
		FVector TraceEnd = ActorCenter;

		FHitResult HitRes;
		FCollisionQueryParams Param;
		if (GetOwner()->ActorLineTraceSingle(HitRes, TraceStart, TraceEnd, ECC_Visibility, Param))
		{
			SpawnPos = HitRes.ImpactPoint;
			SpawnRot = (-HitRes.ImpactNormal).Rotation();
		}
	}
	else
	{
		// 이미 존재하는 경우: 기존 위치 주변에서 상하/좌우 변위 이동
		ActorCenter.Z = FMath::Clamp(
			SweetSpotTransform.GetLocation().Z + FMath::FRandRange(-SweetSpotHeightDeltaRange, SweetSpotHeightDeltaRange),
			GetOwner()->GetActorLocation().Z + SweetSpotMinHeight,
			GetOwner()->GetActorLocation().Z + SweetSpotMaxHeight
		);

		float RandAngle = FMath::FRandRange(-SweetSpotAngleDeltaRange, SweetSpotAngleDeltaRange) + SweetSpotTransform.Rotator().Yaw;
		FVector Dir = FRotator(0.0, RandAngle, 0.0).Vector();

		FVector TraceStart = ActorCenter + Dir * SweetSpotTraceDistance;
		FVector TraceEnd = ActorCenter;

		FHitResult HitRes;
		FCollisionQueryParams Param;
		if (GetOwner()->ActorLineTraceSingle(HitRes, TraceStart, TraceEnd, ECC_Visibility, Param))
		{
			SpawnPos = HitRes.ImpactPoint;
			SpawnRot = (-HitRes.ImpactNormal).Rotation();
		}
	}

	if (!SpawnPos.IsNearlyZero())
	{
		// 서버의 Replicated 변수에 저장 -> 모든 클라이언트에 OnRep으로 복제
		SweetSpotTransform = FTransform(SpawnRot, SpawnPos);

		// 리슨 서버(Host) 플레이어의 로컬 화면에서도 데칼 갱신
		if (GetNetMode() != NM_DedicatedServer)
		{
			OnRep_SweetSpotTransform();
		}
	}
}

void UHarvestableComponent::OnRep_SweetSpotTransform()
{
	// 위치가 0,0,0 등으로 리셋된 경우 데칼 제거
	if (SweetSpotTransform.GetLocation().IsNearlyZero())
	{
		if (CurrentSweetSpotDecal)
		{
			CurrentSweetSpotDecal->DestroyComponent();
			CurrentSweetSpotDecal = nullptr;
		}
		return;
	}

	// 클라이언트 로컬 화면에 데칼 스폰 또는 위치 갱신
	if (!CurrentSweetSpotDecal)
	{
		CurrentSweetSpotDecal = UGameplayStatics::SpawnDecalAtLocation(
			GetWorld(),
			SweetSpotDecal,
			SweetSpotDecalSize,
			SweetSpotTransform.GetLocation(),
			SweetSpotTransform.Rotator(),
			SweetSpotDecalLifeSpan
		);
	}
	else
	{
		CurrentSweetSpotDecal->SetWorldLocationAndRotation(
			SweetSpotTransform.GetLocation(),
			SweetSpotTransform.Rotator()
		);
	}
}

void UHarvestableComponent::CollectYieldItems(const TArray<FHarvestItemYield>& InYields, float Multiplier, TArray<FHarvestItemResult>& OutResults)
{
	for (const FHarvestItemYield& Yield : InYields)
	{
		if (!Yield.ItemData) continue;

		// 드랍 확률 검사
		if (Yield.DropChance < 1.0f && FMath::FRand() > Yield.DropChance)
		{
			continue;
		}

		int32 YieldCount = FMath::CeilToInt32(Yield.BaseCount * Multiplier);
		if (YieldCount <= 0) continue;

		// 기존 목록에 동일 아이템이 있으면 수량 합산, 없으면 추가
		FHarvestItemResult* Existing = OutResults.FindByPredicate([&Yield](const FHarvestItemResult& Item) {
			return Item.ItemData == Yield.ItemData;
		});

		if (Existing)
		{
			Existing->Count += YieldCount;
		}
		else
		{
			FHarvestItemResult NewItem;
			NewItem.ItemData = Yield.ItemData;
			NewItem.Count = YieldCount;
			OutResults.Add(NewItem);
		}
	}
}

void UHarvestableComponent::ProcessDepletion(float Multiplier, TArray<FHarvestItemResult>& InOutResults)
{
	CurrentHp = 0.f;
	if (!bGiveFinalBonus) return;

	// 1. 별도의 고갈 보너스 아이템 목록(FinalBonusYields)이 명시된 경우 (드럼통, 보물상자 등)
	if (FinalBonusYields.Num() > 0)
	{
		TArray<FHarvestItemResult> BonusItems;
		CollectYieldItems(FinalBonusYields, Multiplier, BonusItems);

		if (bDropItemsInWorldOnDepleted)
		{
			SpawnWorldPickups(BonusItems);
		}
		else
		{
			for (const FHarvestItemResult& BonusItem : BonusItems)
			{
				FHarvestItemResult* Existing = InOutResults.FindByPredicate([&BonusItem](const FHarvestItemResult& Item) {
					return Item.ItemData == BonusItem.ItemData;
				});

				if (Existing)
				{
					Existing->Count += BonusItem.Count;
				}
				else
				{
					InOutResults.Add(BonusItem);
				}
			}
		}
	}
	// 2. 아이템 누락시 기본 아이템에 배율만 적용해서 지급(사고 방지)
	else
	{
		const float EffectiveBonus = (FinalBonusMultiplier > 0.0f) ? FinalBonusMultiplier : 1.0f;

		TArray<FHarvestItemResult> BaseItems;
		CollectYieldItems(HarvestYields, EffectiveBonus * Multiplier, BaseItems);

		if (bDropItemsInWorldOnDepleted)
		{
			SpawnWorldPickups(BaseItems);
		}
		else
		{
			InOutResults.Append(BaseItems);
		}
	}
}

void UHarvestableComponent::SpawnWorldPickups(const TArray<FHarvestItemResult>& ItemsToSpawn)
{
	if (!ItemPickupClass || !GetWorld()) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FHarvestItemResult& Item : ItemsToSpawn)
	{
		if (!Item.ItemData || Item.Count <= 0) continue;

		FVector SpawnLoc = GetOwner()->GetActorLocation() + FVector(
			FMath::FRandRange(-DropImpulseRadius, DropImpulseRadius),
			FMath::FRandRange(-DropImpulseRadius, DropImpulseRadius),
			30.0f
		);
		FRotator SpawnRot = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f);

		if (AItemPickup* Pickup = GetWorld()->SpawnActor<AItemPickup>(ItemPickupClass, SpawnLoc, SpawnRot, SpawnParams))
		{
			Pickup->InitializeFromItem(Item.ItemData, Item.Count);
		}
	}
}

void UHarvestableComponent::OnHarvestEnd_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("자원 액터의 자원 고갈 및 소멸"));
	GetOwner()->Destroy();
}

void UHarvestableComponent::SpawnImpactDecal_Implementation(const FVector SpawnPoint, const FRotator SpawnRotator)
{
	if (ImpactDecals.Num() == 0) return;
	int32 RandInt = FMath::RandRange(0, FMath::Max(ImpactDecals.Num() - 1, 0));
	TObjectPtr<UMaterial> CurrDecal = ImpactDecals[RandInt];
	UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CurrDecal, ImpactDecalSize, SpawnPoint, SpawnRotator, ImpactDecalLifeSpan);
}
