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

	// 새 Foundation과 이미 설치된 Foundation 사이에서 서로 맞닿은 모든 연결면을 찾아 양쪽 소켓을 점유 처리하는 함수
	void ResolveAdjacentFoundationConnections(FGuid NewPartID, float AnchorTolerance);

	// 건물 전체의 현재 내구도 반환
	UFUNCTION(BlueprintPure, Category = "Building") float GetCurrentDurability() const { return CurrentDurability; }

	// 건물을 구성하는 모든 파츠의 내구도 합계를 반환 = 그게 곧 이 건물의 내구도
	UFUNCTION(BlueprintPure, Category = "Building") float GetMaxDurability() const { return MaxDurability; }

	// 이 BuildingActor를 구성하는 모든 파츠의 설치 비용을 자원으로 드롭한 뒤 건물 전체를 제거하는 함수
	UFUNCTION(BlueprintCallable, Category = "Building")
	bool DemolishAndDropResources();

	// 건물 전체에 피해를 적용하는 함수, 반환값은 성공 여부
	UFUNCTION(BlueprintCallable, Category = "Building")
	bool ApplyBuildingDamage(float DamageAmount);

private:
	// PlacedParts 전체를 순회해 파츠별 ResourceCosts를 합산 계산하는 함수
	// 예 : Foundation 목재 25 + Wall 목재 50  → 목재 75개로 하나의 환급 항목 생성
	// 비용 데이터가 잘못된 파츠가 하나라도 있으면 false를 반환하여 건물을 삭제하지 못함 !
	bool BuildDemolitionRefunds(TMap<class UItemDataBase*, int32>& OutRefunds) const;
	//  ===================================================================================

protected:
	UPROPERTY(VisibleAnywhere, Category = "Building")
	TObjectPtr<USceneComponent> SceneRoot; // Root Component

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Building")
	TArray<FPlacedBuildingPart> PlacedParts; // 이 건물을 구성하는 모든 설치 완료 파츠 (클라에 동기화할)

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Building|Durability")
	float CurrentDurability = 0.f; // 현재 건물에 남아 있는 전체 내구도

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Building|Durability")
	float MaxDurability = 0.f; // 건물을 구성하는 모든 파츠의 최대 내구도 합계
};
