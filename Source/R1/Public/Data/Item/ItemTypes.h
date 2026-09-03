/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 아이템 타입 정의

#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

class UItemDataBase;

/**
 * 아이템 대분류.
 * - Equipment: 착용 가능(모자/상의/하의 등), 장비슬롯을 점유하며 외형/스탯 변화.
 *   전용 데이터 클래스: UEquipmentItemData.
 * - HeldItem: 손에 드는 무기/도구 전체(칼, 총, 도끼, 곡괭이, 낚싯대 등)를 통합한 값.
 *   장비슬롯 없음 — 벨트(퀵슬롯)에서 선택하면 손에 들고, 같은 칸을 다시 선택하면
 *   손에서 내린다(UInventoryComponent::UseBeltSlot 참고). 무기와 도구를 굳이 값으로
 *   나누지 않은 이유: 슬롯 동작이 완전히 동일하고, 팀 내부적으로 전투용/채집용 구분을
 *   UI 카테고리가 아니라 개별 아이템 데이터(전용 데이터 클래스: UHeldItemData)의
 *   필드(피해량/사거리/채집량 등)만으로 표현하기로 했기 때문.
 * - Consumable: 장비/보유 슬롯 불가, 사용 시 효과 적용 후 소모.
 * - Placeable: 모닥불/제작대처럼 벨트에서 선택해 월드에 배치하는 설치물(러스트 기준
 *   EBuildingPartType::DEPLOYABLE에 해당 — 벽/바닥/기초 같은 "구조물"은 망치+건축
 *   메뉴로 직접 짓는 별개 흐름이라 인벤토리 아이템이 아니고 여기 포함되지 않는다).
 *   전용 데이터 클래스: UPlaceableItemData. 실제 배치 프리뷰/설치 로직은 이미 구현된
 *   건축 시스템(UBuildingPlacementComponent 등, Source/R1/BuildingSystem)을 그대로
 *   쓰고, 이 카테고리는 "그 시스템에 넘길 UBuildingPartDefinition을 인벤토리 슬롯에서
 *   어떻게 다룰지"만 연결하는 얇은 다리 역할이다.
 * - Misc: 데이터(이름/설명)만 존재, 제작/건설 재료용 — 별도 서브클래스 없이
 *   UItemDataBase를 그대로 사용하고 이 값으로만 구분한다.
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Equipment	UMETA(DisplayName = "Equipment"),
	HeldItem	UMETA(DisplayName = "Held Item (Weapon/Tool)"),
	Consumable	UMETA(DisplayName = "Consumable"),
	Placeable	UMETA(DisplayName = "Placeable (모닥불/제작대 등 설치물)"),
	Misc		UMETA(DisplayName = "Misc")
};

/**
 * 월드 픽업(AItemPickup)의 획득 방식. 아이템 종류에 따라 갈리므로 아이템
 * 정의(UItemDataBase) 쪽 속성으로 둔다 — 예: 무기/방어구/물약은 조준+단축키
 * (LookAndPress), 무더기로 흩어지는 광석·제작 재료는 근접 시 자동(AutoOnOverlap).
 */
UENUM(BlueprintType)
enum class EPickupMode : uint8
{
	LookAndPress	UMETA(DisplayName = "Look + Press to Pick Up"),
	AutoOnOverlap	UMETA(DisplayName = "Auto Pick Up on Overlap")
};

/**
 * 장비(착용) 슬롯 종류 — 순수 착용형 부위만 표현한다.
 * 목록은 확정이 아니라 초안이므로 실제 방어구 부위(장갑 등)가 늘어나면 추가하면 된다.
 *
 * 실제 Rust 화면 확인 결과, 이 값은 "고정 배열 인덱스"가 아니라 "같은 부위 유일성 제약"으로만
 * 쓰인다 — UInventoryComponent::EquipmentSlots는 부위별로 고정 배정된 칸이 없는 자유 배열이고,
 * 같은 EquipSlot 값을 가진 아이템은 동시에 하나만 착용 가능하다는 제약만 이 값으로 검사한다.
 */
UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
	None	UMETA(DisplayName = "None"),
	Head	UMETA(DisplayName = "Head"),
	Chest	UMETA(DisplayName = "Chest"),
	Legs	UMETA(DisplayName = "Legs"),
	Feet	UMETA(DisplayName = "Feet")
};

/**
 * 장비(방어구)가 부여하는 스탯 종류. UEquipmentItemData가 FEquipmentStatModifier 배열을
 * 통해 쓴다 — 필드를 늘리는 대신 enum 값 + 배열 조합으로 설계해서, 새 스탯이 추가돼도
 * 여기 값만 늘리면 되고 클래스 구조 자체는 안 건드려도 되게 했다.
 *
 * 자원 타격 피해량/채집 관련 값은 예전엔 여기(HarvestDamage/*GatheringMult)에 있었으나,
 * 채집량을 배율이 아니라 절대 수집량으로 쓰기로 확정하면서 이름에 남아있던 "Mult"가
 * 더 이상 실제 의미와 맞지 않아 UHeldItemData의 전용 필드(Damage/EffectiveRange/
 * OreGathering/WoodGathering/FleshGathering)로 옮겼다. 표현 방법이 두 개로 나뉘는 걸
 * 막기 위해 여기서는 제거함.
 */
UENUM(BlueprintType)
enum class EEquipmentStatType : uint8
{
	Defense				UMETA(DisplayName = "Defense"),
	MovementSpeedMult	UMETA(DisplayName = "Movement Speed Multiplier"),
	DurabilityLossMult	UMETA(DisplayName = "Durability Loss Mult (내구도 소모율 배율)")
};

USTRUCT(BlueprintType)
struct FEquipmentStatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	EEquipmentStatType StatType = EEquipmentStatType::Defense;

	// Defense는 가산값, MovementSpeedMult는 배율(1.0 = 변화 없음)로 해석한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	float Value = 0.f;
};

/**
 * 소비 아이템 효과 종류. 새 효과가 필요해지면 여기 값 추가
 * 실제 적용은 EffectType을 스위치하는 공용 함수 하나(예: 캐릭터의 ApplyItemEffect)에서 처리
 */
UENUM(BlueprintType)
enum class EItemEffectType : uint8
{
	Heal			UMETA(DisplayName = "Heal"),
	RestoreHunger	UMETA(DisplayName = "Restore Hunger"),
	RestoreThirst	UMETA(DisplayName = "Restore Thirst"),
	Poison			UMETA(DisplayName = "Poison (중독, 예: 생고기)"),
	BleedReduction	UMETA(DisplayName = "Bleed Reduction (지혈, 예: 붕대)"),
	HorseSprintSpeedBoost	UMETA(DisplayName = "Horse Sprint Speed Boost (말 질주 속도, 예: 사과)")
};

USTRUCT(BlueprintType)
struct FItemEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	EItemEffectType EffectType = EItemEffectType::Heal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	float Magnitude = 0.f;

	// 즉발 효과면 0, 도트/버프처럼 시간에 걸쳐 적용되는 효과면 초 단위 지속시간.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	float Duration = 0.f;
};

// 제작 재료 하나(재료 아이템 + 수량). UItemDataBase::CraftingCost 배열의 원소로 쓴다.
USTRUCT(BlueprintType)
struct FCraftIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	TSoftObjectPtr<UItemDataBase> Item;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting", meta = (ClampMin = "1"))
	int32 Amount = 1;
};
