// 작업 시작일 : 8/28
// 작업자 : 우진
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingPreviewActor.generated.h"

// 건축 파츠들이 진짜 월드에 배치되기 전에 어떻게 배치될 지 눈으로 직접 확인할 수 있는 프리뷰 액터 클래스
//TODO : 플레이어 별로 하나씩 가진 채로 건축 할 때 마다 재활용 하도록 할 예정
UCLASS()
class R1_API ABuildingPreviewActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ABuildingPreviewActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	void SetDefinition(class UBuildingPartDefinition* Definition);
	//  ===================================================================================

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot; // Root Component

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent; // Mesh Component

protected:
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UBuildingPartDefinition> PreviewDefinition; // Preview로 보여줄 파츠의 데이터

};
