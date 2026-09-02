/// 최초작성 : 2026.08.31
/// 작 성 자 : 강 진 구
/// 침낭 테스트

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Interface/RespawnPointInterface.h"
#include "TestSleepingBag.generated.h"

UCLASS()
class R1_API ATestSleepingBag : public AActor, public IRespawnPointInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestSleepingBag();

	virtual FTransform GetRespawnTransform_Implementation() const override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
