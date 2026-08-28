// 08/28 주형진

#include "Component/HarvestableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Character/ActionCharacter.h"
#include "Components/DecalComponent.h"

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
	//TODO ItemData 기반으로 연동

	// 0. 캐릭터의 현재 무기가 이 액터를 공격할 수 있는 타입인지 확인
	
	// 1. 무기의 데이터를 기반으로 체력 감소
	CurrentHp -= 50.f;

	// 3. 자원 액터의 데이터를 통해서 결과 구조체 생성
	// Res.Count = Res.ItemData.Cnt 
	Res.Count = 1;
	Res.HarvesResult = true;
	Res.ItemData = ItemData;
	// 3-0. 스위트 스팟에 맞은 경우 개수에 배율을 곱해서 반환

	if (CurrentSweetSpotDecal)
	{
		float DistSqr = FVector::DistSquared(HitLocation, CurrentSweetSpotDecal->GetComponentLocation());
		//TODO 매직넘버 고치리
		// 반경 15cm를 때렸으면 맞은걸로 처리
		if (DistSqr <= 15 * 15)
		{
			// 소수점 발생시 올림처리
			Res.Count = FMath::CeilToInt32(Res.Count * BounusRate);
			GenerateSweetSpot();
		}
	}

	// 4. 만약 자원 액터의 체력이 0보다 작아지면 OnHarvestEnd() 호출
	if (CurrentHp <= 0)
	{
		CurrentHp = 0.f;
		IHarvestable::Execute_OnHarvestEnd(this);
	}


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
	UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CurrDecal, FVector(10, 10, 10), SpawnPoint, SpawnRotator, 60.f);
}

// Called when the game starts
void UHarvestableComponent::BeginPlay()
{
	Super::BeginPlay();

	GenerateSweetSpot();
	
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
		
		//0. 액터의 원점에서 랜덤한 벡터 선정 (높이 제한 60 ~ 100)
		//0-0 액터의 원점에서 60~100만큼 떨어진 지점에서 시작
		FVector ActorCenter  = GetOwner()->GetActorLocation();
		// TODO 매직 넘버 수정
		ActorCenter.Z += FMath::FRandRange(80.f, 120.f);

		//1. 랜덤한 방향 선정 (yaw만 설정)
		float RandAngle = FMath::FRandRange(0.f, 360.f);
		FVector Dir = FRotator(0.0, RandAngle, 0.0).Vector();

		//2. 위에서 선정한 백터 방향으로 10m이동 (큰 나무 생각)
		FVector TraceStart = ActorCenter + Dir * 1000;

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

			//TODO 매직넘버 고치기
			FVector DecalSize(10, 10, 10);
			CurrentSweetSpotDecal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), SweetSpotDecal, DecalSize, SpawnPos, SpawnRot, 60.f);
		}
	}
	else
	{
		// 이미 데칼이 존재하는 상태면
		// 
		// 0. 기존 데칼의 상하 20cm까지 움직인다.
		FVector ActorCenter = GetOwner()->GetActorLocation();
		//TODO 매직넘버 고치기
		ActorCenter.Z = FMath::Clamp(CurrentSweetSpotDecal->GetComponentLocation().Z + FMath::FRandRange(-15.f, 15.f), 80.f, 120.f);

		// 1. 기존 데칼의 좌우로 45도까지 회전한다.
		float RandAngle = FMath::FRandRange(-22.5f, 22.5f) + CurrentSweetSpotDecal->GetComponentRotation().Yaw;
		FVector Dir = FRotator(0.0, RandAngle, 0.0).Vector();

		// 2. 그 지점에서 다시 ray를 쏴서 맞은 지점에 데칼을 이동 시킨다.
		FVector TraceStart = ActorCenter + Dir * 1000;
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

