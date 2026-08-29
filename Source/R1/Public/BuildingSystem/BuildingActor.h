// 작업 시작일 : 8/28
// 작업자 : 우진
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingActor.generated.h"

// 건축 조각들이 모여 만들어진 건축물 하나 단위의 액터 클래스
UCLASS()
class R1_API ABuildingActor : public AActor
{
	GENERATED_BODY()
public:
	ABuildingActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// 건축물에(this) 건축 파츠를 추가하는 함수
	UFUNCTION(BlueprintCallable, Category = "Building")
	UStaticMeshComponent* AddPart(class UBuildingPartDefinition* Definition, const FTransform& InRelativeTransform);

	//  ===================================================================================

protected:
	UPROPERTY(VisibleAnywhere, Category = "Building")
	TObjectPtr<USceneComponent> SceneRoot; // Root Component

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> PartComponents; // 건축 파츠들의 메시 컴포넌트를 관리하는 배열
};
