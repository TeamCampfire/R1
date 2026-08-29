// 작업 시작일 : 8/28
// 작업자 : 우진

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildingPartDefinition.generated.h"

USTRUCT(BlueprintType)
struct FGroundPlacementRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bEnabled = false;  // 이 파츠가 지면 경사 검사를 사용하는가?

    UPROPERTY(EditAnywhere, BlueprintReadOnly, 
		meta = (EditCondition = "bEnabled", EditConditionHides, ClampMin = "0.0", ClampMax = "90.0"))
    float MaxSlopeAngle = 35.f; // 설치 가능한 최대 지면 경사 (bEnabled 변수 값에 따라 해당 변수 활/비활 가능!!!)
};

/**
 건축 파츠 하나의 데이터 애셋
 */
UCLASS()
class R1_API UBuildingPartDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building|Visual")
	TObjectPtr<UStaticMesh> PartMesh; // 파츠를 표현할 메시
	//TODO 추후 SoftObjectPtr로 변경

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building|State", meta = (ClampMin = 0))
	int32 MaxDurability; // 파츠의 최대 내구도

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	FGroundPlacementRule GroundPlacementRule; // 지면에 배치될 때의 조건? 규칙?
};
