

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "HarvestSpawner.generated.h"

UCLASS()
class R1_API AHarvestSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHarvestSpawner();
	void InitializeSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 풀의 최초 인스턴스를 생성한다. 이후에는 파괴/재생성하지 않고 재사용한다.
	AActor* SpawnHarvestableObject(TSubclassOf<AActor> TargetClass);

	bool FindSpawnTransform(FTransform& OutTransform) const;
	void ActivatePooledActor(TWeakObjectPtr<AActor> PooledActor);
	void SchedulePoolActivation(AActor* PooledActor, float InDelay);

	UFUNCTION()
	void OnActorDepleted(AActor* DepletedActor);

	UFUNCTION()
	void OnManagedActorDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnLegacyActorDestroyed(AActor* DestroyedActor);

	void OnTargetClassesLoaded();

	void ProcessPendingSpawns();

public:

protected:
	//TODO 데이터 에셋으로 넘어가기
	UPROPERTY(EditDefaultsOnly, Category = "Spawn|Target")
	TArray<TSoftClassPtr<AActor>> SpawnTargetArray;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn|Target")
	TArray<int32> MaxCntArray;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float Radius = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float Delay = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float SpawnInterval = 0.1f;

	/*UPROPERTY(EditDefaultsOnly, Category = "Spawn|Target")
	TArray<TObjectPtr<TSubclassOf<AActor>>> TreeArray;*/

private:
	TSharedPtr<FStreamableHandle> AsyncHandle;

	TArray<TSubclassOf<AActor>> PendingList;
	FTimerHandle SpawnTimerHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> ManagedActors;

	TSet<TWeakObjectPtr<AActor>> InactiveActors;
	TMap<TWeakObjectPtr<AActor>, FTimerHandle> RespawnTimerHandles;
};
