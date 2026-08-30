#include "Component/HarvestableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Character/ActionCharacter.h"
#include "Components/DecalComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "Item/ItemPickup.h"
#include "NiagaraFunctionLibrary.h"

// Sets default values for this component's properties
UHarvestableComponent::UHarvestableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	ItemPickupClass = AItemPickup::StaticClass();
}

FHarvestRes UHarvestableComponent::OnHitted_Implementation(AActionCharacter* InCharacter, const FVector& HitLocation)
{
	FHarvestRes Res;
	if (CurrentHp <= 0 || !InCharacter) return Res;

	// 1. 체력 감소 (임시 50)
	// TODO 아이템 장착 구현되면 적용
	CurrentHp -= 50.f;

	// 2. 피격 피드백(사운드/FX/데칼) 및 스위트스팟 배율 산출
	const float YieldMultiplier = ProcessHitFeedback(InCharacter, HitLocation, Res.bHitSweetSpot);

	// 3. 고갈(파괴) 조건 판정 (Rust 방식)
	// - 스위트스팟을 쓰는 오브젝트(나무/광석): 피가 다 닳아도 '스위트스팟(X자)을 쳐야만' 최종 파괴 & 보너스 수확!
	// - 스위트스팟을 안 쓰는 오브젝트(드럼통/상자/시체): 피가 다 닳으면 즉시 파괴 & 보너스 수확!
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

	// 스위트 스팟 적중 판정
	if (bUseSweetSpot && CurrentSweetSpotDecal)
	{
		float DistSqr = FVector::DistSquared(HitLocation, CurrentSweetSpotDecal->GetComponentLocation());
		if (DistSqr <= FMath::Square(SweetSpotHitRadius))
		{
			bOutHitSweetSpot = true;
			Multiplier = BounusRate;
			GenerateSweetSpot();
		}
	}

	// 임팩트 데칼 소환
	FRotator DecalRot = (InCharacter->GetActorLocation() - HitLocation).Rotation();
	SpawnImpactDecal_Implementation(HitLocation, DecalRot);

	// FX 및 사운드 재생
	if (bOutHitSweetSpot)
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

	return Multiplier;
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
	UE_LOG(LogTemp, Display, TEXT("자원 액터의 자원 고갈"));
	// 0. 파괴 연출 재생
	UE_LOG(LogTemp, Display, TEXT("자원 액터의 파괴 애니메이션"));
	// 1. 연출 재생이 끝나면 Destroy
	UE_LOG(LogTemp, Display, TEXT("자원 액터의 소멸"));
	//DestroyComponent();
	GetOwner()->Destroy();

}

void UHarvestableComponent::SpawnImpactDecal_Implementation(const FVector SpawnPoint, const FRotator SpawnRotator)
{
	if (ImpactDecals.Num() == 0) return;
	// 데칼 중에서 랜덤으로 선택해 소환
	int32 RandInt = FMath::RandRange(0, FMath::Max(ImpactDecals.Num() - 1,0));
	TObjectPtr<UMaterial> CurrDecal = ImpactDecals[RandInt];
	UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CurrDecal, ImpactDecalSize, SpawnPoint, SpawnRotator, ImpactDecalLifeSpan);
}

// Called when the game starts
void UHarvestableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bUseSweetSpot)
	{
		GenerateSweetSpot();
	}
	
}


// Called every frame
void UHarvestableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UHarvestableComponent::GenerateSweetSpot()
{

	FVector SpawnPos;
	FRotator SpawnRot;

	// 초기 생성
	if (!CurrentSweetSpotDecal)
	{
		
		//0. 액터의 원점에서 랜덤한 높이 선정
		FVector ActorCenter  = GetOwner()->GetActorLocation();
		ActorCenter.Z += FMath::FRandRange(SweetSpotMinHeight, SweetSpotMaxHeight);

		//1. 랜덤한 방향 선정 (yaw만 설정)
		float RandAngle = FMath::FRandRange(0.f, 360.f);
		FVector Dir = FRotator(0.0, RandAngle, 0.0).Vector();

		//2. 위에서 선정한 백터 방향으로 이동 (큰 나무 생각)
		FVector TraceStart = ActorCenter + Dir * SweetSpotTraceDistance;

		FVector TraceEnd = ActorCenter;

		FHitResult HitRes;

		// 이렇게 할거면 플레이어는 감지 못하는 채널을 만들던가
		// 플레이어는 Visibillity에서 영원히 빼던가
		//if (GetWorld()->LineTraceSingleByChannel(HitRes, TraceStart, TraceEnd, ECC_Visibility))

		// 이렇게하면 이 액터의 컴포넌트들을 대상으로 라인 트레이싱 
		//2. 해당 지점에서 벡터를 액터의 원점 방향으로 쏴서 데칼 생성
		FCollisionQueryParams Param;
		if(GetOwner()->ActorLineTraceSingle(HitRes, TraceStart, TraceEnd, ECC_Visibility, Param))
		{
			SpawnPos = HitRes.ImpactPoint;
			SpawnRot = HitRes.ImpactNormal.Rotation();

			CurrentSweetSpotDecal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), SweetSpotDecal, SweetSpotDecalSize, SpawnPos, SpawnRot, SweetSpotDecalLifeSpan);
		}
	}
	else
	{
		// 이미 데칼이 존재하는 상태면
		// 
		// 0. 기존 데칼의 상하 오프셋 범위 내에서 이동
		FVector ActorCenter = GetOwner()->GetActorLocation();
		ActorCenter.Z = FMath::Clamp(CurrentSweetSpotDecal->GetComponentLocation().Z + FMath::FRandRange(-SweetSpotHeightDeltaRange, SweetSpotHeightDeltaRange), SweetSpotMinHeight, SweetSpotMaxHeight);

		// 1. 기존 데칼의 좌우 각도 편차 범위 내에서 회전
		float RandAngle = FMath::FRandRange(-SweetSpotAngleDeltaRange, SweetSpotAngleDeltaRange) + CurrentSweetSpotDecal->GetComponentRotation().Yaw;
		FVector Dir = FRotator(0.0, RandAngle, 0.0).Vector();

		// 2. 그 지점에서 다시 ray를 쏴서 맞은 지점에 데칼을 이동 시킨다.
		FVector TraceStart = ActorCenter + Dir * SweetSpotTraceDistance;
		FVector TraceEnd = ActorCenter;

		FHitResult HitRes;
		FCollisionQueryParams Param;
		if (GetOwner()->ActorLineTraceSingle(HitRes, TraceStart, TraceEnd, ECC_Visibility, Param))
		{
			SpawnPos = HitRes.ImpactPoint;
			SpawnRot = HitRes.ImpactNormal.Rotation();
			CurrentSweetSpotDecal->SetWorldLocationAndRotation(SpawnPos, SpawnRot);
		}
	}


}

