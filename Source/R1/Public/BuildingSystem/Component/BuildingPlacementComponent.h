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

	UFUNCTION(BlueprintCallable)
	class ABuildingPreviewActor* GetPreviewActor(); // Getter함수_PreviewActor

	// 현재 프리뷰 위치에 건축물 설치를 요청
	void ConfirmPlacement();

protected:
	// 클라이언트가 서버에 새로운 건물 생성을 요청
	UFUNCTION(Server, Reliable)
	void ServerPlaceNewBuilding(UBuildingPartDefinition* Definition, const FTransform& InPlacementTransform);

private:
	// 현재 프리뷰 액터 영역이 다른 오브젝트들과 겹치는지 검사, true : 겹침(설치 불가)
	// SupportingComponent : 프리뷰를 받치고 있는 지면 컴포넌트
	bool HasPlacementOverlap(const UPrimitiveComponent* SupportingComponent) const;
	
	// 프리뷰 위치를 세팅하고, 겹침 결과에 따라 머티리얼 변경
	void ShowPreviewAtLocation(const FVector& InPreviewLocation, const FVector& InSurfaceNormal, UPrimitiveComponent* SupportingComponent);

	// 현재 맞힌 표면이 선택한 파츠를 배치할 수 있는 표면인지 검사
	bool IsBuildableSurface(const UPrimitiveComponent* SupportingComponent) const;

	// 현재 표면의 경사가 선택한 파츠의 허용 범위인지 검사
	bool IsGroundSlopeValid(const FVector& InSurfaceNormal);

	// 본인이 로컬 플레이어인지 검사. 서버일 때는 본인의 프리뷰 액터만 보여야 하기 때문
	bool IsLocalPlacementController() const;

	//  ===================================================================================
private:
	// 최대 건축 지점 거리
	float MaxPlacementDistance = 800.f;

	// 최대 건축 지점을 기준으로 위,아래 각각 탐색할 세로 라인트레이스 길이
	float GroundSearchDistance = 1000.f;

	// Foundation 전용 : 현재 Foundation의 피벗부터 가장 낮은 지점까지의 길이
	float CurFoundationLegLength = 0.f;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Building|Placement")
	TSubclassOf<class ABuildingPreviewActor> PreviewActorClass; // 생성할 프리뷰 액터 종류 (BP_PreviewBuildingActor를 알기 위함)

	UPROPERTY(Transient)
	TObjectPtr<ABuildingPreviewActor> PreviewActor; // PreviewActorClass로 생성된, 현재 재사용하고 있는 프리뷰 액터

	UPROPERTY(Transient)
	TObjectPtr<UBuildingPartDefinition>	SelectedDefinition; // 현재 선택한 건축 파츠 데이터

	UPROPERTY(EditDefaultsOnly, Category = "Building|Placement")
	TSubclassOf<class ABuildingActor> BuildingActorClass; // 실제 설치 시 생성할 건물 껍데기 액터 클래스

	UPROPERTY(Transient)
	bool bCanPlace = false; // 현재 프리뷰 위치에 실제 파츠를 설치할 수 있는지요?

private:
	bool bIsPlacing = false; // 현재 건축물 배치중인가요?
};