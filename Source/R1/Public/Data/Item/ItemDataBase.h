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
 * 서브클래스: UEquipmentItemData(장비), UHeldItemData(무기/도구), UConsumableItemData(소비),
 * UPlaceableItemData(설치물). 기타(Misc) 아이템은 추가 필드가 필요 없으므로 별도
 * 서브클래스를 만들지 않고 이 클래스를 그대로 사용한다 (Category = Misc) — 그래서
 * 이 클래스는 Abstract로 설정 X
 *
 */
UCLASS()
class R1_API UItemDataBase : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	//~ Begin UObject Interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

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

	// 1이면 스택 불가. Equipment/HeldItem/Placeable은 슬롯당 1개만 보관 가능해야 하므로
	// PostLoad/PostEditChangeProperty에서 Category를 보고 강제로 1로 고정한다(EnforceStackRulesForCategory).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;


	// 월드에 드랍되거나 레벨에 배치될 때(AItemPickup) 보여줄 메시.
	// UEquipmentItemData::EquippedMesh(착용 시 스켈레탈 메시 파츠)와는 별개 개념 —
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visual")
	TSoftObjectPtr<UStaticMesh> PickupMesh;

	// 월드 픽업(AItemPickup)의 획득 방식. 무기/방어구/물약처럼 낱개로 다루는 아이템은
	// LookAndPress(조준+단축키), 무더기로 흩어지는 광석·제작 재료 등은 AutoOnOverlap(근접 시 자동).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EPickupMode DefaultPickupMode = EPickupMode::LookAndPress;

	// 제작에 필요한 재료 목록(재료 아이템 + 수량). 장비/도구뿐 아니라 소비/기타 아이템도
	// 제작 가능할 수 있어(붕대, 침낭 등) 서브클래스가 아니라 공통 베이스에 둔다.
	// 비어있으면 제작 불가(월드/상자 획득 전용) 아이템으로 취급.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Crafting")
	TArray<FCraftIngredient> CraftingCost;

private:
	// Category가 Equipment/HeldItem/Placeable이면 MaxStackSize를 1로 강제한다 — 디자이너가
	// 실수로 다른 값을 넣거나 에디터에서 Category만 바꿔도 즉시(에디터)/로드 시(런타임) 정정된다.
	void EnforceStackRulesForCategory();
};
