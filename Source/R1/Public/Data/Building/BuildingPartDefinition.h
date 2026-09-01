// 작업 시작일 : 8/28
// 작업자 : 우진

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildingPartDefinition.generated.h"

// 배치되는 방식 Enum
UENUM(BlueprintType)
enum class EBuildingPlacementType : uint8
{
	FOUNDATION,		// 지면 자유 배치 또는 다른 Foundation에 스냅
	STRUCTURE_SNAP, // 기존 구조물의 지정된 스냅 포인트에 설치
	SURFACE,		// Foundation이나 Floor의 표면 위에 자유 배치
	ATTACHMENT		// 문틀처럼 특정 파츠의 전용 슬롯에 부착
};

// 건축 파츠 타입 Enum
UENUM(BlueprintType)
enum class EBuildingPartType : uint8
{
	NONE,
	FOUNDATION,
	FOUNDATION_TRIANGLE,
	WALL,
	WALL_DOORFRAME,
	FLOOR,
	STAIR,
	DOOR,
	DEPLOYABLE, // 모닥불, 제작대처럼 구조물이 아닌 설치물
	ROOF
};

// 스냅되어 붙을 수 있는 것들의 정보가 담긴 구조체
USTRUCT(BlueprintType)
struct FBuildingSnapPointDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SocketName = NAME_None; // Static Mesh에 만들어둔 스냅 소켓 이름이면서 설치된 파츠에서 이 자리를 식별하는 ID

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<EBuildingPartType> AllowedPartTypes; // 이 위치에 설치할 수 있는 파츠의 종류

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ConnectedPartSocketName = NAME_None; // 이 소켓에 새 파츠가 연결될 때 새로 설치된 파츠에서 함께 점유할 연결 소켓 이름

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snapping")
	FName ConnectionAnchorSocketName = NAME_None; // 이 스냅 포인트가 담당하는 실제 연결면의 중심 소켓
	// (새 파츠 설치 후 다른 Foundation의 연결면과 맞닿았는지 검사할 때 사용)
};

// 곧 사라질 수도 있음
// 지형 배치 조건이 담긴 구조체
USTRUCT(BlueprintType)
struct FGroundPlacementRule
{
    GENERATED_BODY()

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	EBuildingPlacementType PlacementType = EBuildingPlacementType::FOUNDATION; // 월드에 배치되는 방식

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	EBuildingPartType PartType = EBuildingPartType::NONE; // 건축 파츠의 종류

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Placement", meta = (TitleProperty = "SocketName"))
	TArray<FBuildingSnapPointDefinition> SnapPoints; // 이 파츠가 다른 파츠에 제공하는 스냅 위치들

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snapping")
	TArray<float> AllowedSnapYawOffsets = { 0.f }; // 소켓의 회전을 기준으로 선택할 수 있는 상대 Yaw 각도들
};
