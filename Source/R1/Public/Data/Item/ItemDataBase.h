/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 모든 아이템의 베이스 클래스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Item/ItemTypes.h"
#include "ItemDataBase.generated.h"

/**
 * 모든 아이템의 공통 "정적 정의" 데이터.
 *
 * 인스턴스별로 바뀌는 값(현재 스택 수량, 향후 내구도 등)은 여기 두지 않는다.
 * 이 DataAsset은 같은 종류의 아이템이면 절대 안 바뀌는 값만 갖고,
 * 실제 인벤토리 슬롯에는 이 애셋을 참조하는 FItemInstance(ItemInstance.h)가 들어간다.
 *
 * 서브클래스: UEquipmentItemData(장비), UConsumableItemData(소비).
 * 기타(Misc) 아이템은 추가 필드가 필요 없으므로 별도 서브클래스를 만들지 않고
 * 이 클래스를 그대로 사용한다 (Category = Misc) — 그래서 이 클래스는 Abstract로 설정 X
 *
 */
UCLASS()
class R1_API UItemDataBase : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	//~ Begin UPrimaryDataAsset Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UPrimaryDataAsset Interface

public:
	// 게임 로직/세이브에서 사용하는 안정적 식별자.
	// Asset Manager가 로드에 쓰는 PrimaryAssetId(GetPrimaryAssetId 참고)와는 별개 —
	// 이쪽은 에셋 리네임/경로 이동에 영향받지 않는 디자이너용 ID다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	// 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	// 설명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
	FText Description;

	// 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	// 카테고리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemCategory Category = EItemCategory::Misc;

	// 1이면 스택 불가. 장비 아이템은 UEquipmentItemData 생성자에서 1로 고정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;


	// 월드에 드랍되거나 레벨에 배치될 때(AItemPickup) 보여줄 메시.
	// UEquipmentItemData::EquippedMesh(착용 시 스켈레탈 메시 파츠)와는 별개 개념 —
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visual")
	TSoftObjectPtr<UStaticMesh> PickupMesh;

};
