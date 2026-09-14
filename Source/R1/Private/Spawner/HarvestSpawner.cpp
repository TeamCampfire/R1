#include "Spawner/HarvestSpawner.h"

#include "Component/HarvestableComponent.h"
#include "Engine/AssetManager.h"
#include "Interface/HarvestPoolable.h"
#include "R1/R1.h"

AHarvestSpawner::AHarvestSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHarvestSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;

	InitializeSpawner();
}

void AHarvestSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	for (TPair<TWeakObjectPtr<AActor>, FTimerHandle>& Pair : RespawnTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Pair.Value);
	}
	RespawnTimerHandles.Empty();

	for (AActor* ManagedActor : ManagedActors)
	{
		if (!IsValid(ManagedActor)) continue;

		if (UHarvestableComponent* Harvestable = ManagedActor->FindComponentByClass<UHarvestableComponent>())
		{
			Harvestable->OnHarvestableDepleted.RemoveDynamic(this, &AHarvestSpawner::OnActorDepleted);
		}
		ManagedActor->OnDestroyed.RemoveDynamic(this, &AHarvestSpawner::OnManagedActorDestroyed);
	}
	ManagedActors.Empty();
	InactiveActors.Empty();

	Super::EndPlay(EndPlayReason);
}

AActor* AHarvestSpawner::SpawnHarvestableObject(TSubclassOf<AActor> TargetClass)
{
	if (!HasAuthority() || !TargetClass || !GetWorld()) return nullptr;

	FTransform SpawnTransform;
	if (!FindSpawnTransform(SpawnTransform))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[AHarvestSpawner] Could not find a valid ground position for %s"),
			*TargetClass->GetName());
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(TargetClass, SpawnTransform, SpawnParams);
	if (!SpawnedActor) return nullptr;

	if (UHarvestableComponent* Harvestable = SpawnedActor->FindComponentByClass<UHarvestableComponent>())
	{
		SpawnedActor->SetReplicateMovement(true);
		ManagedActors.Add(SpawnedActor);
		Harvestable->OnHarvestableDepleted.AddDynamic(this, &AHarvestSpawner::OnActorDepleted);
		SpawnedActor->OnDestroyed.AddDynamic(this, &AHarvestSpawner::OnManagedActorDestroyed);
		Harvestable->SetPoolActive(true);

		if (SpawnedActor->GetClass()->ImplementsInterface(UHarvestPoolable::StaticClass()))
		{
			IHarvestPoolable::Execute_OnTakenFromHarvestPool(SpawnedActor);
		}
	}
	else
	{
		// HarvestableComponent가 없는 기존 대상은 기존 파괴 기반 리스폰을 유지한다.
		UE_LOG(LogTemp, Warning,
			TEXT("[AHarvestSpawner] %s has no HarvestableComponent; using legacy destroy/respawn."),
			*GetNameSafe(SpawnedActor));
		SpawnedActor->OnDestroyed.AddDynamic(this, &AHarvestSpawner::OnLegacyActorDestroyed);
	}

	return SpawnedActor;
}

bool AHarvestSpawner::FindSpawnTransform(FTransform& OutTransform) const
{
	if (!GetWorld()) return false;

	constexpr int32 MaxTry = 100;
	constexpr float ZOffset = 5000.0f;
	constexpr float TraceLength = 10000.0f;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_BUILDABLEGROUND);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;

	for (int32 CurrentTry = 0; CurrentTry < MaxTry; ++CurrentTry)
	{
		const FVector2D RandomPos2D = FMath::RandPointInCircle(Radius);
		const FVector StartPos = GetActorLocation()
			+ FVector(RandomPos2D.X, RandomPos2D.Y, ZOffset);
		const FVector EndPos = StartPos + FVector::DownVector * TraceLength;

		FHitResult WaterHit;
		const bool bInWater = GetWorld()->LineTraceSingleByChannel(
			WaterHit, StartPos, EndPos, ECC_Water, QueryParams);

		FHitResult GroundHit;
		const bool bHitGround = GetWorld()->LineTraceSingleByObjectType(
			GroundHit, StartPos, EndPos, ObjectQueryParams, QueryParams);

		if (bHitGround && !bInWater)
		{
			OutTransform = FTransform(
				FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f),
				GroundHit.ImpactPoint);
			return true;
		}
	}

	return false;
}

void AHarvestSpawner::OnActorDepleted(AActor* DepletedActor)
{
	if (!HasAuthority() || !IsValid(DepletedActor) || InactiveActors.Contains(DepletedActor)) return;

	InactiveActors.Add(DepletedActor);

	if (DepletedActor->GetClass()->ImplementsInterface(UHarvestPoolable::StaticClass()))
	{
		IHarvestPoolable::Execute_OnReturnedToHarvestPool(DepletedActor);
	}

	if (UHarvestableComponent* Harvestable = DepletedActor->FindComponentByClass<UHarvestableComponent>())
	{
		Harvestable->SetPoolActive(false);
	}

	SchedulePoolActivation(DepletedActor, Delay);
}

void AHarvestSpawner::SchedulePoolActivation(AActor* PooledActor, float InDelay)
{
	if (!IsValid(PooledActor) || !GetWorld()) return;

	const TWeakObjectPtr<AActor> WeakActor(PooledActor);
	if (FTimerHandle* ExistingHandle = RespawnTimerHandles.Find(WeakActor))
	{
		GetWorldTimerManager().ClearTimer(*ExistingHandle);
	}

	FTimerHandle& TimerHandle = RespawnTimerHandles.FindOrAdd(WeakActor);
	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(
		this, &AHarvestSpawner::ActivatePooledActor, WeakActor);
	GetWorldTimerManager().SetTimer(
		TimerHandle, RespawnDelegate, FMath::Max(InDelay, KINDA_SMALL_NUMBER), false);
}

void AHarvestSpawner::ActivatePooledActor(TWeakObjectPtr<AActor> PooledActor)
{
	RespawnTimerHandles.Remove(PooledActor);

	AActor* Actor = PooledActor.Get();
	if (!HasAuthority() || !IsValid(Actor) || !InactiveActors.Contains(PooledActor)) return;

	FTransform SpawnTransform;
	if (!FindSpawnTransform(SpawnTransform))
	{
		// 일시적으로 지면을 찾지 못했으면 새로 생성하지 않고 같은 인스턴스로 재시도한다.
		SchedulePoolActivation(Actor, FMath::Max(SpawnInterval, 1.0f));
		return;
	}

	Actor->SetActorLocationAndRotation(
		SpawnTransform.GetLocation(),
		SpawnTransform.Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	if (UHarvestableComponent* Harvestable = Actor->FindComponentByClass<UHarvestableComponent>())
	{
		Harvestable->SetPoolActive(true);
	}

	if (Actor->GetClass()->ImplementsInterface(UHarvestPoolable::StaticClass()))
	{
		IHarvestPoolable::Execute_OnTakenFromHarvestPool(Actor);
	}

	InactiveActors.Remove(PooledActor);
}

void AHarvestSpawner::OnManagedActorDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority() || !DestroyedActor) return;

	const TSubclassOf<AActor> OriginalClass = DestroyedActor->GetClass();
	const TWeakObjectPtr<AActor> WeakActor(DestroyedActor);
	if (FTimerHandle* Handle = RespawnTimerHandles.Find(WeakActor))
	{
		GetWorldTimerManager().ClearTimer(*Handle);
	}
	RespawnTimerHandles.Remove(WeakActor);
	InactiveActors.Remove(WeakActor);
	ManagedActors.Remove(DestroyedActor);

	FTimerHandle ReplacementHandle;
	FTimerDelegate ReplacementDelegate = FTimerDelegate::CreateWeakLambda(this, [this, OriginalClass]()
	{
		SpawnHarvestableObject(OriginalClass);
	});
	GetWorldTimerManager().SetTimer(
		ReplacementHandle,
		ReplacementDelegate,
		FMath::Max(Delay, KINDA_SMALL_NUMBER),
		false);
}

void AHarvestSpawner::OnLegacyActorDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority() || !DestroyedActor) return;

	const TSubclassOf<AActor> OriginalClass = DestroyedActor->GetClass();
	FTimerHandle RespawnHandle;
	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateWeakLambda(this, [this, OriginalClass]()
	{
		SpawnHarvestableObject(OriginalClass);
	});
	GetWorldTimerManager().SetTimer(
		RespawnHandle,
		RespawnDelegate,
		FMath::Max(Delay, KINDA_SMALL_NUMBER),
		false);
}

void AHarvestSpawner::OnTargetClassesLoaded()
{
	PendingList.Empty();

	// SpawnTargetArray[i] 타겟을 MaxCntArray[i]개만큼 최초 한 번 생성한다.
	for (int32 Index = 0; Index < SpawnTargetArray.Num(); ++Index)
	{
		TSubclassOf<AActor> LoadedClass = SpawnTargetArray[Index].Get();
		if (!LoadedClass) continue;

		for (int32 Count = 0; Count < MaxCntArray[Index]; ++Count)
		{
			PendingList.Add(LoadedClass);
		}
	}

	if (PendingList.IsEmpty()) return;

	// 풀 워밍업도 프레임에 나누어 처리해 시작 히치를 줄인다.
	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AHarvestSpawner::ProcessPendingSpawns,
		FMath::Max(SpawnInterval, KINDA_SMALL_NUMBER),
		true);
}

void AHarvestSpawner::ProcessPendingSpawns()
{
	if (PendingList.IsEmpty())
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	const TSubclassOf<AActor> ClassToSpawn = PendingList.Pop();
	SpawnHarvestableObject(ClassToSpawn);
}

void AHarvestSpawner::InitializeSpawner()
{
	if (SpawnTargetArray.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AHarvestSpawner::InitializeSpawner] Please assign spawn targets."));
		return;
	}

	if (MaxCntArray.Num() != SpawnTargetArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AHarvestSpawner::InitializeSpawner] Set a max count for every target."));
		return;
	}

	TArray<FSoftObjectPath> TargetsToLoad;
	for (const TSoftClassPtr<AActor>& SpawnTarget : SpawnTargetArray)
	{
		if (!SpawnTarget.IsNull())
		{
			TargetsToLoad.AddUnique(SpawnTarget.ToSoftObjectPath());
		}
	}

	if (TargetsToLoad.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AHarvestSpawner::InitializeSpawner] Every spawn target is null."));
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	AsyncHandle = StreamableManager.RequestAsyncLoad(
		TargetsToLoad,
		FStreamableDelegate::CreateUObject(this, &AHarvestSpawner::OnTargetClassesLoaded));
}
