// 작업 시작일 : 8/28
// 작업자 : 우진
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildingPlacementComponent.generated.h"


// 건축 파츠를 선택한 순간부터 실제로 설치하기 전까지의 과정을 담당하는 컴포넌트
// Player의 Controller에 붙을 예정
UCLASS( ClassGroup=(Building), meta=(BlueprintSpawnableComponent) )
class R1_API UBuildingPlacementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuildingPlacementComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Building|Placement")
	void StartPlacement(class UBuildingPartDefinition* Definition); // 건축물 짓기 시작할 때

	UFUNCTION(BlueprintCallable, Category = "Building|Placement")
	void StopPlacement(); // 건축물 짓는 거 마무리 지을 때
	//  ===================================================================================

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Building|Placement")
	TSubclassOf<class ABuildingPreviewActor> PreviewActorClass; // 생성할 프리뷰 액터 종류 (BP_PreviewBuildingActor를 알기 위함)

	UPROPERTY(Transient)
	TObjectPtr<ABuildingPreviewActor> PreviewActor; // PreviewActorClass로 생성된, 현재 재사용하고 있는 프리뷰 액터

	UPROPERTY(Transient)
	TObjectPtr<UBuildingPartDefinition> SelectedDefinition; // 현재 선택한 건축 파츠 데이터

private:
	bool bIsPlacing = false; // 현재 건축물 배치중인가요?
};