// 08/28 주형진

#include "Component/HarvestableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Character/ActionCharacter.h"
#include "Components/DecalComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "NiagaraFunctionLibrary.h"

// Sets default values for this component's properties
UHarvestableComponent::UHarvestableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

FHarvestRes UHarvestableComponent::OnHitted_Implementation(AActionCharacter* InCharacter, const FVector& HitLocation)
{
	FHarvestRes Res;
	if (CurrentHp <= 0) return Res;
	if (!InCharacter) return Res;

	//TODO 세부 로직 구현
	// 0. 캐릭터의 현재 무기가 이 액터를 공격할 수 있는 타입인지 확인
	
	// 1. 무기의 데이터를 기반으로 체력 감소 (임시 50)
	CurrentHp -= 50.f;

	// 2. 스위트 스팟 적중 여부 판정
	bool bHitSweetSpot = false;
	float YieldMultiplier = 1.0f;

	if (bUseSweetSpot && CurrentSweetSpotDecal)
	{
		float DistSqr = FVector::DistSquared(HitLocation, CurrentSweetSpotDecal->GetComponentLocation());
		// 스위트 스팟 반경 내 타격 확인
		if (DistSqr <= FMath::Square(SweetSpotHitRadius))
		{
			bHitSweetSpot = true;
			YieldMultiplier = BounusRate;
			GenerateSweetSpot();
		}
	}

	Res.bHitSweetSpot = bHitSweetSpot;

	// 3. FX 및 사운드 재생, 임팩트 데칼 소환
	FRotator DecalRot = (InCharacter->GetActorLocation() - HitLocation).Rotation();
	SpawnImpactDecal_Implementation(HitLocation, DecalRot);

	if (bHitSweetSpot)
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

	// 4. 아이템 드랍 계산 (다중 아이템 및 확률 지원)
	for (const FHarvestItemYield& Yield : HarvestYields)
	{
		if (!Yield.ItemData) continue;

		// 드랍 확률 검사
		if (Yield.DropChance < 1.0f && FMath::FRand() > Yield.DropChance)
		{
			continue;
		}

		int32 YieldCount = FMath::CeilToInt32(Yield.BaseCount * YieldMultiplier);
		if (YieldCount > 0)
		{
			FHarvestItemResult ItemRes;
			ItemRes.ItemData = Yield.ItemData;
			ItemRes.Count = YieldCount;
			Res.HarvestedItems.Add(ItemRes);
		}
	}

	// 5. 자원 고갈(체력 0 이하) 및 마무리 보너스 지급
	if (CurrentHp <= 0)
	{
		CurrentHp = 0.f;
		Res.bIsDepleted = true;

		// 고갈 보너스 지급
		if (bGiveFinalBonus)
		{
			if (FinalBonusYields.Num() > 0)
			{
				for (const FHarvestItemYield& BonusYield : FinalBonusYields)
				{
					if (!BonusYield.ItemData) continue;
					if (BonusYield.DropChance < 1.0f && FMath::FRand() > BonusYield.DropChance)
					{
						continue;
					}

					int32 BonusCount = BonusYield.BaseCount;
					if (BonusCount > 0)
					{
						FHarvestItemResult* ExistingItem = Res.HarvestedItems.FindByPredicate([&BonusYield](const FHarvestItemResult& Item) {
							return Item.ItemData == BonusYield.ItemData;
						});

						if (ExistingItem)
						{
							ExistingItem->Count += BonusCount;
						}
						else
						{
							FHarvestItemResult BonusItemRes;
							BonusItemRes.ItemData = BonusYield.ItemData;
							BonusItemRes.Count = BonusCount;
							Res.HarvestedItems.Add(BonusItemRes);
						}
					}
				}
			}
			else if (FinalBonusMultiplier > 1.0f)
			{
				// 별도 보너스 목록이 없으면 기본 획득 아이템 목록에 배율 적용
				for (FHarvestItemResult& Item : Res.HarvestedItems)
				{
					Item.Count = FMath::CeilToInt32(Item.Count * FinalBonusMultiplier);
				}
			}
		}

		IHarvestable::Execute_OnHarvestEnd(this);
	}

	Res.HarvesResult = (Res.HarvestedItems.Num() > 0 || Res.bIsDepleted);
	return Res;
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

