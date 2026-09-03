#include "Spawner/HarvestSpawner.h"

// Sets default values
AHarvestSpawner::AHarvestSpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AHarvestSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;
	InitializeSpawner();
}

// Called every frame
void AHarvestSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

AActor* AHarvestSpawner::SpawnHarvestableObject(TSubclassOf<AActor> TargetClass)
{
	if (!HasAuthority()) return nullptr;
	AActor* SpawnedActor = nullptr;

	int32 MaxTry = 100;
	int32 Current = 0;
	// 최대 반복횟수 제한
	while (Current < MaxTry)
	{
		//0. 원 안에서 임의의 점을 선택
		//0-1. 원의 중심 선정
		FVector CentorLoc = GetActorLocation();
		float ZOffset = 5000.f;
		CentorLoc.Z += ZOffset;

		//0-2. 원점을 통해서 반지름 Radius인 원을 만들고 반지름 안의 임의의 점 선정
		FVector2D RandonPos2D = FMath::RandPointInCircle(Radius);
		FVector RandPos3D(CentorLoc.X + RandonPos2D.X, CentorLoc.Y + RandonPos2D.Y, CentorLoc.Z);

		//1. 해당 점에서 아래로 라인 트레이스
		FHitResult HitRes;
		//1-1. 라인 트레이스 길이는 10미터 까지로 제한.
		FVector StartPos = RandPos3D;
		FVector EndPos = RandPos3D + FVector::DownVector * 10000.f;

		//1-2. ECC_WorldStatic만 감지
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		//1-3. 추가 파라미터 (자신 제외, 복잡한 충돌 여부 등)
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this); // 자기 자신은 무시
		QueryParams.bTraceComplex = false; // 단순 콜리전 사용 (필요시 true)

		//1-3. 라인 트레이스 WordStatic을 대상으로만 진행
		bool bHit = GetWorld()->LineTraceSingleByObjectType(
			HitRes,
			StartPos,
			EndPos,
			ObjectQueryParams,
			QueryParams
		);
		//2. 충돌시 해당 지점에 소환
		if (bHit)
		{
			SpawnedActor = GetWorld()->SpawnActor<AActor>(TargetClass, HitRes.ImpactPoint, FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f));
			//3. 소환한 액터의 OnDestroy에 SpawnHarvestableObject 달아놓기
			SpawnedActor->OnDestroyed.AddDynamic(this, &AHarvestSpawner::OnActorDepleted);
			break;
		}
		//4. 충돌하지 않은 경우 0.으로 복귀
		Current++;
	}

	return SpawnedActor;
}


void AHarvestSpawner::OnActorDepleted(AActor* DestroyedActor)
{
	if (!HasAuthority()) return;

	// 해당 엑터가 파괴되었을 때 Delay만큼 기다렸다가 다시 소환
	TSubclassOf<AActor> OriginalClass = DestroyedActor->GetClass();

	FTimerHandle RespawnHandle;
	GetWorld()->GetTimerManager().SetTimer(
		RespawnHandle,
		[this, OriginalClass]()
		{
			SpawnHarvestableObject(OriginalClass);
		},
		Delay,
		false
	);
}

void AHarvestSpawner::InitializeSpawner()
{
	if (SpawnTargetArray.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AHarvestSpawner::InitializeSpawner()] : Please Assign Spawn Target"));
		return;
	}

	if (MaxCntArray.Num() != SpawnTargetArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AHarvestSpawner::InitializeSpawner()] : Please set All Max Cnt"));
		return;
	}

	// SpawnTargetArray[i] 타겟을  MaxCntArray[i]개 만큼 소환 
	for (int i = 0; i < SpawnTargetArray.Num(); i++)
	{
		for (int j = 0; j < MaxCntArray[i]; j++)
		{
			if (SpawnHarvestableObject(SpawnTargetArray[i]) == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("[AHarvestSpawner::InitializeSpawner()] : Something Wrong With Spawn"));
				return;
			}
		}
	}
}
