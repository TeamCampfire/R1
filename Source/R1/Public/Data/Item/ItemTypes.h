/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 아이템 타입 정의

#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

/**
 * 아이템 대분류.
 * - Equipment: 착용 가능(모자/상의/하의 등), 장비슬롯을 점유하며 외형/스탯 변화
 * - Weapon: 총/칼 등. 장비슬롯 없음 — 벨트(퀵슬롯)에서 선택하면 손에 들고, 같은 칸을
 *   다시 선택하면 손에서 내린다(UInventoryComponent::UseBeltSlot 참고).
 * - Tool: 곡괭이 등. 슬롯 동작은 Weapon과 동일하고, 채집량 같은 상세 스탯만 나중에 별도로 가짐.
 * - Consumable: 장비/보유 슬롯 불가, 사용 시 효과 적용 후 소모.
 * - Misc: 데이터(이름/설명)만 존재, 제작/건설 재료용 — 별도 서브클래스 없이
 *   UItemDataBase를 그대로 사용하고 이 값으로만 구분한다. Weapon/Tool도 상세 스탯이
 *   생기기 전까지는 같은 방식(서브클래스 없이 이 값으로만 구분)으로 취급한다.
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Equipment	UMETA(DisplayName = "Equipment"),
	Weapon		UMETA(DisplayName = "Weapon"),
	Tool		UMETA(DisplayName = "Tool"),
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
 * 장비 착용 시 부여되는 스탯 종류.
 * 지금은 방어력/이동속도만 확정됐지만("등"으로 확장 여지 있음) 필드를 늘리는 대신
 * enum 값 + 배열(FEquipmentStatModifier) 조합으로 설계해서, 새 스탯이 추가돼도
 * 여기 값만 늘리면 되고 UEquipmentItemData 구조 자체는 안 건드려도 되게 했다.
 */
UENUM(BlueprintType)
enum class EEquipmentStatType : uint8
{
	Defense				UMETA(DisplayName = "Defense"),
	MovementSpeedMult	UMETA(DisplayName = "Movement Speed Multiplier")
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
