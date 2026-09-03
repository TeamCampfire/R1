/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 장비아이템 정의 클래스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Item/ItemDataBase.h"
#include "EquipmentItemData.generated.h"

/**
 * 장비(착용) 아이템 정의. 머리/상의/하의/다리/발 등 몸에 착용하며 외형/스탯 변화를 준다.
 * 손에 드는 무기/도구는 이 클래스가 아니라 UHeldItemData(HeldItemData.h)를 쓴다 —
 * 예전에는 무기/도구도 이 클래스를 그대로 썼으나, "몸에 착용" vs "손에 들기"는
 * 슬롯 시스템(장비 슬롯 vs 벨트+HeldItemComponent)부터 갈리는 별개 개념이라 분리했다.
 *
 * - 방어구(Head/Chest/Legs/Feet)는 좌클릭 액션이 없는 것으로 확정되어 액션 관련 필드는 넣지 않음.
 * - 내구도 감소(피격 시)/파괴 시스템은 "필요할지 더 확인이 필요"한 상태라 이번 패스에서는 제외.
 *   확정되면: 이 클래스에는 MaxDurability(불변값)만 추가하고, 실제 현재 내구도는
 *   정적 정의가 아니라 인스턴스 상태이므로 FItemInstance(ItemInstance.h) 쪽에
 *   CurrentDurability를 추가하는 식으로 확장.
 */
UCLASS()
class R1_API UEquipmentItemData : public UItemDataBase
{
	GENERATED_BODY()

public:
	UEquipmentItemData();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	EEquipmentSlotType EquipSlot = EEquipmentSlotType::Chest;

	// 착용 시 갈아 끼울 스켈레탈 메시 파츠.
	// (Body/Head 모듈형 구조 + SetLeaderPoseComponent로 본을 공유하는 방식과 함께 사용 —
	// 캐릭터 이동/애니메이션 로드맵 문서의 Body+Head Leader Pose 패턴 참고)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Visual")
	TSoftObjectPtr<USkeletalMesh> EquippedMesh;

	// 착용 시 적용되는 스탯 변화 (방어력, 이동속도 배율 등).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquipmentStatModifier> StatModifiers;

	// 내구도 사용 여부 (방어구, 도구, 무기 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Durability")
	bool bHasDurability = true;

	// 장비의 최대 내구도 (정적 불변값)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Durability", meta = (EditCondition = "bHasDurability", ClampMin = "1.0"))
	float MaxDurability = 100.0f;
};
