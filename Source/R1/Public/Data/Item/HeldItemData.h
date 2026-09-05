/// 최초작성 : 2026.09.03
/// 작 성 자 : 최 요 환
/// 간단설명 : 손에 드는 무기/도구(HeldItem) 아이템 정의 클래스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Item/ItemDataBase.h"
#include "HeldItemData.generated.h"

/**
 * 손에 드는 무기/도구 아이템 정의(칼, 총, 도끼, 곡괭이, 낚싯대 등).
 * 장비 슬롯을 점유하지 않고, 벨트(퀵슬롯)에서 선택하면 손에 들고 같은 칸을 다시
 * 선택하면 손에서 내린다(UInventoryComponent::UseBeltSlot 참고).
 *
 * 무기(전투용)와 도구(채집용)를 별도 클래스로 나누지 않은 이유: 슬롯 동작이 완전히
 * 동일하고, 전투용/채집용 구분은 클래스가 아니라 이 클래스의 필드 값(피해량/사거리는
 * 전투용, 채집량은 채집용에서만 의미를 가짐 — 쓰지 않는 필드는 0으로 비워두면 됨)만으로
 * 표현하기로 했기 때문(EItemCategory::HeldItem 주석 참고).
 *
 * 붕대 등 손에 들고 쓰는 소모품은 시간 관계상 이번 프로젝트에서는 제외하고 즉시소비형
 * Consumable로만 구현한다 — 그래서 이 클래스엔 Effects 같은 소비 효과 필드가 없다.
 */
UCLASS()
class R1_API UHeldItemData : public UItemDataBase
{
	GENERATED_BODY()

public:
	UHeldItemData();

	// 벨트 슬롯에서 선택(장착)했을 때 손에 스폰할 액터 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem")
	TSubclassOf<class AHeldItemBase> HeldItemClass;

	// 부여되는 스탯 변화 (방어력 등 범용 modifier — Defense/MovementSpeedMult 계열).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem")
	TArray<FEquipmentStatModifier> StatModifiers;

	// 피해량(대인/자원 타격 공용, 추후 분리 필요해지면 나눔).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Combat")
	float Damage = 0.f;

	// 유효 사거리(cm 등 단위는 추후 확정).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Combat")
	float EffectiveRange = 0.f;

	// 아래 3개는 "배율"이 아니라 "고정 수집량"으로 쓰기로 확정했다 — 이 도구로 한 번
	// 타격/채집할 때마다 얻는 절대 수량. 채집용이 아닌 아이템(칼 등)은 0으로 둔다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Gathering")
	float OreGathering = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Gathering")
	float WoodGathering = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Gathering")
	float FleshGathering = 0.f;

	// 내구도 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Durability")
	bool bHasDurability = true;

	// 최대 내구도 (정적 불변값)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Durability", meta = (EditCondition = "bHasDurability", ClampMin = "1.0"))
	float MaxDurability = 100.0f;

	// 좌클릭 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Animation")
	TObjectPtr<UAnimMontage> PrimaryMontage;

	// 우클릭 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Animation")
	TObjectPtr<UAnimMontage> SecondaryMontage;

	// 해당 아이템에 사용할 애니메이션 레이어
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Animation")
	TSubclassOf<UAnimInstance> AnimLayer;

	// 손에 장착될 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Mesh")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

};
