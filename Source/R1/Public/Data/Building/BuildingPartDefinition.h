// 작업 시작일 : 8/28
// 작업자 : 우진

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildingPartDefinition.generated.h"

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
};
