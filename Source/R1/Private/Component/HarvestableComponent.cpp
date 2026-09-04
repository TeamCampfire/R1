#include "Component/HarvestableComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Character/ActionCharacter.h"
#include "Components/DecalComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "Item/ItemPickup.h"
#include "NiagaraFunctionLibrary.h"
#include "Component/HeldItemComponent.h"
#include "Data/Item/HeldItemData.h"

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
	// TODO 기본값 삭제
	float Damage = 10.0f;
	float ItemCnt = 10;
	if (CurrentHp <= 0 || !InCharacter) return Res;


	// 0. 데이터 세팅
	if (UHeldItemComponent* Comp = InCharacter->GetHeldItemComponent())
	{
		if(UHeldItemData* ItemData = Comp->GetCurrentEquippedItemData())
			Damage = ItemData->Damage;
	}

	//1. 스위트스팟 맞았는지 확인
	bool IsHitted = SweetSpotHitted(HitLocation);
	if (IsHitted)
	{
		Damage *= 2;
		ItemCnt *= 2;
	}

	// 1. 체력 감소
	CurrentHp -= Damage;

	// 2. 모든 클라이언트(및 호스트)에 타격 FX/사운드/임팩트데칼 브로드캐스트
	FRotator DecalRot = (InCharacter->GetActorLocation() - HitLocation).Rotation();
	Multicast_PlayHitEffects(HitLocation, IsHitted, DecalRot);

	// 3. 아이템 획득

	// 기본 아이템 획득
	if (DefaultItem)
		Res.HarvestedItems.Add(FHarvestItemResult(DefaultItem, ItemCnt));

	// 보너스 아이템
	TArray<FHarvestItemResult> HarverstAdditiveItems;
	CollectYieldItems(HarvestAdditiveYields, HarverstAdditiveItems);
	for (auto& Item : HarverstAdditiveItems)
	{
		Res.HarvestedItems.Add(Item);
	}

	// 4. 부숴진 경우에 아이템을 획득하는 경우
	if (CurrentHp <= 0)
	{
		if (bGiveItemWhenDestroy)
		{
			TArray<FHarvestItemResult> DestroyAdditiveItems;
			CollectYieldItems(DestroyAdditiveYields, DestroyAdditiveItems);
			if (bDropItemsInWorldOnDepleted)
			{
				SpawnWorldPickups(DestroyAdditiveItems);
			}
			else
			{
				for (auto& Item : DestroyAdditiveItems)
				{
					Res.HarvestedItems.Add(Item);
				}
			}

		}
		IHarvestable::Execute_OnHarvestEnd(this);
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

bool UHarvestableComponent::SweetSpotHitted(const FVector& HitLocation)
{
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
				GenerateSweetSpot_Server(); // 적중 성공 시 다음 스위트스팟 위치로 이동
				return true;
			}
		}
	}
	return false;
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
			SpawnRot = (HitRes.ImpactNormal).Rotation();
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
			SpawnRot = HitRes.ImpactNormal.Rotation();
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

void UHarvestableComponent::CollectYieldItems(const TArray<FHarvestItemYield>& InYields, TArray<FHarvestItemResult>& OutResults)
{
	for (const FHarvestItemYield& Yield : InYields)
	{
		if (!Yield.ItemData) continue;

		// 드랍 확률 검사
		if (Yield.DropChance < 1.0f && FMath::FRand() > Yield.DropChance)
		{
			continue;
		}

		if (Yield.BaseCount <= 0) continue;

		// 기존 목록에 동일 아이템이 있으면 수량 합산, 없으면 추가
		FHarvestItemResult* Existing = OutResults.FindByPredicate([&Yield](const FHarvestItemResult& Item) {
			return Item.ItemData == Yield.ItemData;
		});

		if (Existing)
		{
			Existing->Count += Yield.BaseCount;
		}
		else
		{
			FHarvestItemResult NewItem;
			NewItem.ItemData = Yield.ItemData;
			NewItem.Count = Yield.BaseCount;
			OutResults.Add(NewItem);
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
	//UE_LOG(LogTemp, Display, TEXT("자원 액터의 자원 고갈 및 소멸"));
	if (CurrentSweetSpotDecal)
	{
		CurrentSweetSpotDecal->DestroyComponent();
		CurrentSweetSpotDecal = nullptr;
	}
	GetOwner()->Destroy();
}

void UHarvestableComponent::SpawnImpactDecal_Implementation(const FVector SpawnPoint, const FRotator SpawnRotator)
{
	if (ImpactDecals.Num() == 0) return;
	int32 RandInt = FMath::RandRange(0, FMath::Max(ImpactDecals.Num() - 1, 0));
	TObjectPtr<UMaterial> CurrDecal = ImpactDecals[RandInt];
	UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CurrDecal, ImpactDecalSize, SpawnPoint, SpawnRotator, ImpactDecalLifeSpan);
}
