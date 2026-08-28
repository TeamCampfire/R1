// 작업 시작일 : 8/28
// 작업자 : 우진
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingActor.generated.h"

UCLASS()
class R1_API ABuildingActor : public AActor
{
	GENERATED_BODY()
public:
	ABuildingActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	UFUNCTION(BlueprintCallable, Category = "Building")
	void AddPart(class UBuildingPartDefinition* Definition, const FTransform& InRelativeTransform);

	//  ===================================================================================

protected:
	UPROPERTY(VisibleAnywhere, Category = "Building")
	TObjectPtr<USceneComponent> SceneRoot; // Root Component

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> PartComponents;
};
