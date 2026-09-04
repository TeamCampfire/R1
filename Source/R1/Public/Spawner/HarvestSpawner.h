

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

	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	//TODO 데이터 에셋으로 넘어가기
	// 수집 액터를 소환하는 함수
	AActor* SpawnHarvestableObject(TSubclassOf<AActor> TargetClass);
	// 수집 액터의 파괴시에 호출될 콜백
	UFUNCTION(BlueprintCallable)
	void OnActorDepleted(AActor* DestroyedActor);

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
	int32 Radius = 10000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float Delay = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float SpawnInterval = 0.2f;

	/*UPROPERTY(EditDefaultsOnly, Category = "Spawn|Target")
	TArray<TObjectPtr<TSubclassOf<AActor>>> TreeArray;*/

private:
	TSharedPtr<FStreamableHandle> AsyncHandle;

	TArray<TSubclassOf<AActor>> PendingList;
	FTimerHandle SpawnTimerHandle;
};
