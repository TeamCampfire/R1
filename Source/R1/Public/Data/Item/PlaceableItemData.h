/// 최초작성 : 2026.09.03
/// 작 성 자 : 최 요 환
/// 간단설명 : 설치물(모닥불/제작대 등) 아이템 정의 클래스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Item/ItemDataBase.h"
#include "PlaceableItemData.generated.h"

/**
 * 벨트에서 선택해 월드에 배치하는 설치물 아이템 정의(모닥불, 제작대 등).
 * 실제 배치 프리뷰/설치 로직은 새로 만들지 않고 이미 구현된 건축 시스템
 * (UBuildingPlacementComponent, ABuildingPreviewActor, ABuildingActor —
 * Source/R1/BuildingSystem)을 그대로 쓴다. 그래서 이 클래스는 그 시스템이
 * 요구하는 UBuildingPartDefinition을 가리키는 필드 하나만 갖는다 — PartMesh/PlacementType/SnapPoints/MaxDurability 등을
 * 여기서 중복으로 정의하지 않는다.
 *
 * 벽/바닥/기초 같은 "구조물" 파츠(EBuildingPartType의 FOUNDATION/WALL/...)는
 * 망치+건축 메뉴로 직접 짓는 별개 흐름이라 인벤토리 아이템이 아니다 — 이 클래스는
 * EBuildingPartType::DEPLOYABLE 파츠(모닥불/제작대류)에만 쓴다.
 */
UCLASS()
class R1_API UPlaceableItemData : public UItemDataBase
{
	GENERATED_BODY()

public:
	UPlaceableItemData();

	// 이 아이템을 벨트에서 선택해 배치를 시작할 때 UBuildingPlacementComponent::
	// StartPlacement()에 넘길 건축 파츠 데이터. EBuildingPartType::DEPLOYABLE인
	// UBuildingPartDefinition을 참조해야 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placeable")
	TSoftObjectPtr<class UBuildingPartDefinition> BuildingPart;
};
