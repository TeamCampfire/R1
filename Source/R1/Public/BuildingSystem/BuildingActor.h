// 작업 시작일 : 8/28
// 작업자 : 우진
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingActor.generated.h"

USTRUCT(BlueprintType)
struct FPlacedBuildingPart
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid PartID; // 설치된 파츠 하나를 식별하는 고유 ID

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBuildingPartDefinition> Definition; // 이 파츠가 어떤 건축 파츠인지 나타내는 원본 데이터

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTransform RelativeTransform = FTransform::Identity; // 껍데기 BuildingActor를 기준으로 한 상대 위치

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CurDurability = 0.f; // 현재 내구도

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComponent; // 실제로 월드에 표시되는 런타임 메시 컴포넌트

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FName> OccupiedSnapPoints; 	// 이 파츠가 제공하는 스냅 포인트 중 이미 다른 파츠가 설치된(사용된?) 소켓!! 이름
};

// 건축 조각들이 모여 만들어진 건축물 하나 단위의 껍데기 액터 클래스
UCLASS()
class R1_API ABuildingActor : public AActor
{
	GENERATED_BODY()
public:
	ABuildingActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 건축물에(this) 건축 파츠를 추가하는 함수
	UFUNCTION(BlueprintCallable, Category = "Building")
	UStaticMeshComponent* AddPart(class UBuildingPartDefinition* Definition, const FTransform& InRelativeTransform);

	// 충돌한 메시 컴포넌트(건축 파츠)에 해당하는 설치 파츠 기록을 찾는 함수
	// (인자로 들어오는 게 라인트레이스로 맞춘 파츠의 메시 컴포넌트)
	const FPlacedBuildingPart* FindPlacedPartByComponent(const UPrimitiveComponent* InComponent) const;

	FPlacedBuildingPart* FindPlacedPartByComponent(UPrimitiveComponent* InComponent);

	// 해당 건축 파츠가 제공하는 소켓의 월드 Transform을 가져오는 함수 -> 가져옴 : true 반환
	// 두번째 인자 : 이번에 새로 설치될 건축 파츠 데이터
	bool TryGetSnapPointWorldTransform(const UPrimitiveComponent* TargetComponent,
		const UBuildingPartDefinition* IncomingDefinition, const FName& InSocketName,
		FTransform& OutWorldTransform) const;

	// (서버용) PartID를 이용해 설치된 파츠를 확인
	FPlacedBuildingPart* FindPlacedPartByID(const FGuid& InPartID);

	// 인자로 들어온 Transform 위치에 기존 Foundation이 이미 존재하는지 확인
	// Foundation 고리 때문에 점유 체크와는 별개로 위치 중복 검사가 필요해요
	bool HasFoundationAtTransform(const FTransform& InWorldTransform, float LocationTolerance = 2.f) const;
	//  ===================================================================================

protected:
	UPROPERTY(VisibleAnywhere, Category = "Building")
	TObjectPtr<USceneComponent> SceneRoot; // Root Component

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Building")
	TArray<FPlacedBuildingPart> PlacedParts; // 이 건물을 구성하는 모든 설치 완료 파츠 (클라에 동기화할)
};
