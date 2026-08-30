/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 아이템 타입 정의

#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

/**
 * 아이템 대분류.
 * - Equipment: 착용 가능, 외형/스탯 변화
 * - Consumable: 장비 불가, 사용 시 효과 적용
 * - Misc: 데이터(이름/설명)만 존재, 제작/건설 재료용 — 별도 서브클래스 없이
 *   UItemDataBase를 그대로 사용하고 이 값으로만 구분한다.
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Equipment	UMETA(DisplayName = "Equipment"),
	Consumable	UMETA(DisplayName = "Consumable"),
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
 * 장비 슬롯 종류.
 * Head/Chest/Legs/Feet = 외형 전용 방어구 슬롯
 * Weapon/Tool = 손에 드는 슬롯
 * 목록은 확정이 아니라 초안이므로 실제 방어구 부위(장갑 등)가 늘어나면 추가하면 된다.
 */
UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
	None	UMETA(DisplayName = "None"),
	Head	UMETA(DisplayName = "Head"),
	Chest	UMETA(DisplayName = "Chest"),
	Legs	UMETA(DisplayName = "Legs"),
	Feet	UMETA(DisplayName = "Feet"),
	Weapon	UMETA(DisplayName = "Weapon (Hand)"),
	Tool	UMETA(DisplayName = "Tool (Hand)")
};

/**
 * 장비 착용 시 부여되는 스탯 종류.
 * 지금은 방어력/이동속도만 확정됐지만("등"으로 확장 여지 있음) 필드를 늘리는 대신
 * enum 값 + 배열(FEquipmentStatModifier) 조합으로 설계해서, 새 스탯이 추가돼도
 * 여기 값만 늘리면 되고 UEquipmentItemData 구조 자체는 안 건드려도 되게 했다.
 */
UENUM(BlueprintType)
enum class EEquipmentStatType : uint8
{
	Defense				UMETA(DisplayName = "Defense"),
	MovementSpeedMult	UMETA(DisplayName = "Movement Speed Multiplier"),
	HarvestDamage		UMETA(DisplayName = "Harvest Damage (자원 타격 피해량)"),
	WoodGatheringMult	UMETA(DisplayName = "Wood Gathering Mult (나무 벌목 배율)"),
	OreGatheringMult	UMETA(DisplayName = "Ore Gathering Mult (암석 채광 배율)"),
	FleshGatheringMult	UMETA(DisplayName = "Flesh Gathering Mult (생고기 해체 배율)"),
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
	RestoreThirst	UMETA(DisplayName = "Restore Thirst")
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
