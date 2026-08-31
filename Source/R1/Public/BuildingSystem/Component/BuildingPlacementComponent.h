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
	// 클라이언트가 서버에 새로운 건축물 생성을 요청
	UFUNCTION(Server, Reliable)
	void ServerPlaceNewBuilding(UBuildingPartDefinition* Definition, const FTransform& InPlacementTransform);

	// 클라이언트가 서버에 스냅 건물 파츠 생성을 요청
	UFUNCTION(Server, Reliable)
	void ServerPlaceSnappedPart(UBuildingPartDefinition* Definition, 
		class ABuildingActor* TargetBuilding, FGuid TargetPartID, FName SocketName);

	// Foundation Type의 지면 배치 프리뷰를 갱신
	void UpdateFoundationPreview(APlayerController* PlayerController);

	// Structure_Snap Type의 지면 배치 프리뷰를 갱신
	void UpdateStructureSnapPreview(APlayerController* PlayerController);

	// Structure_Snap 대상 초기화 함수
	void ClearCurrentSnapTarget();

	// 프리뷰를 설치 불가 상태로 만들고 숨김
	void HidePlacementPreview();

private:
	// 현재 프리뷰 액터 영역이 다른 오브젝트들과 겹치는지 검사, true : 겹침(설치 불가)
	// SupportingComponent : 프리뷰를 받치고 있는 지면 컴포넌트
	bool HasPlacementOverlap(const UPrimitiveComponent* SupportingComponent, const ABuildingActor* IgnoredBuilding = nullptr) const;
	
	// 프리뷰 위치를 세팅하고, 겹침 결과에 따라 머티리얼 변경
	void ShowPreviewAtLocation(const FVector& InPreviewLocation, const FVector& InSurfaceNormal, UPrimitiveComponent* SupportingComponent);

	// 현재 맞힌 표면이 선택한 파츠를 배치할 수 있는 표면인지 검사
	bool IsBuildableSurface(const UBuildingPartDefinition* Definition, const UPrimitiveComponent* SupportingComponent) const;

	// 현재 표면의 경사가 선택한 파츠의 허용 범위인지 검사
	bool IsGroundSlopeValid(const UBuildingPartDefinition* Definition, const FVector& InSurfaceNormal);

	// 본인이 로컬 플레이어인지 검사. 서버일 때는 본인의 프리뷰 액터만 보여야 하기 때문
	bool IsLocalPlacementController() const;

	// 요청한 설치 위치가 서버에서 허용하는 거리 안인지 검사
	bool IsWithinServerPlacementDistance(const FVector& InPlacementLocation) const;

	// 설치 요청 위치 아래에서 Foundation을 지지할 BuildableGround 탐색 (서버용)
	bool FindSupportingGround(const UBuildingPartDefinition* Definition, const FVector& InPlacementLocation, struct FHitResult& OutGroundHit) const;

	// 서버 겹침 검사용 테스트 메시 컴포넌트를 최초 한 번 생성한 뒤 재사용할 예정
	UStaticMeshComponent* GetOrCreateServerValidationMesh();

	// 요청받은 Transform에서 실제 메시 콜리전이 장애물과 겹치는지 검사
	bool HasServerPlacementOverlap(const UBuildingPartDefinition* Definition, const FTransform& InPlacementTransform, 
		const UPrimitiveComponent* IgnoredSupportingComponent = nullptr, const ABuildingActor* IgnoredBuilding = nullptr);

	// 파츠의 PlacementType에 맞는 서버 배치 검사
	bool IsServerPlacementRuleValid(const UBuildingPartDefinition* Definition, const FTransform& InPlacementTransform);

	// Foundation의 지면·경사·장애물 조건을 검사 (기존)
	bool IsServerFoundationPlacementValid(const UBuildingPartDefinition* Definition, const FTransform& InPlacementTransform);
	
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

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ServerValidationMeshComponent; // 서버에서 실제 메시 콜리전 검사에 사용하는 숨겨진 컴포넌트

	UPROPERTY(EditDefaultsOnly, Category = "Building|Snapping", meta = (ClampMin = "0.0"))
	float StructureSnapTraceDistance = 1000.f; // 카메라에서 기존 건축물을 찾는 라인트레이스 거리

	UPROPERTY(EditDefaultsOnly, Category = "Building|Snapping", meta = (ClampMin = "0.0"))
	float SnapPointSearchRadius = 200.f; // 조준 위치에서 이 거리 안에 있는 소켓만 후보로 사용

	UPROPERTY(Transient)
	TWeakObjectPtr<class ABuildingActor> CurrentSnapBuilding; // 현재 프리뷰가 붙어 있는 BuildingActor

	FGuid CurrentSnapTargetPartID; // 현재 프리뷰가 붙어 있는 대상 파츠 ID

	FName CurrentSnapSocketName = NAME_None; // 현재 선택된 소켓 이름

	UPROPERTY(EditDefaultsOnly, Category = "Building|Server")
	float ServerPlacementDistanceTolerance = 50.f; // 서버 허용 오차 (네트워크 지연이나 부동소수점 계산 차이)

private:
	bool bIsPlacing = false; // 현재 건축물 배치중인가요?
};
