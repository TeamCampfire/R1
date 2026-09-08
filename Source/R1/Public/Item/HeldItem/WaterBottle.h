/// 최초작성 : 2026.09.07
/// 작 성 자 : 최 요 환
/// 간단설명 : 빈병/채워진병 두 상태만 오가는 물병 HeldItem

#pragma once

#include "CoreMinimal.h"
#include "Item/HeldItemBase.h"
#include "WaterBottle.generated.h"

class UHeldItemData;

/**
 * 물병 도구 액터.
 *
 * 물통에 물을 "얼마나" 채웠는지가 아니라 빈 상태/채워진 상태 딱 2가지만 다룬다 — 그래서
 * FItemInstance에 별도 수량 필드를 추가하지 않고, EmptyBottleData/FilledBottleData 두
 * UHeldItemData 애셋 사이를 오가는 방식으로 표현한다(같은 슬롯의 ItemData를 서로 바꿔치기).
 * 실제 슬롯 데이터 교체는 UInventoryComponent::SetSlotItem → SetSlot이 처리하며, SetSlot이
 * "새 내용물도 여전히 HeldItem이면 해제하지 않고 제자리에서 재장착"하도록 되어 있어서(2026-09-07
 * 확장) 이 액터는 손에서 내려갔다 다시 들리는 과정 없이 데이터/메시만 바뀐다.
 *
 * - 좌클릭(주 액션) = 마시기: 채워진 상태에서만 동작. StatComponent에 RestoreThirst 효과를 적용한
 *   뒤 EmptyBottleData로 전환.
 * - 우클릭(보조 액션) = 채우기: 빈 상태에서만 동작. 전방에 AWaterBody가 있는지 검사(FishingRod::
 *   UpdateCastingTrajectory와 동일한 WaterBodyCollision 프로파일 오버랩 방식 재사용)해서 있으면
 *   FilledBottleData로 전환.
 *
 * 상태 전환은 항상 서버 권위 Server RPC(Server_DrinkWater/Server_FillWater)를 거친다 — 인벤토리
 * 슬롯을 바꾸는 작업이라 클라이언트가 직접 실행하면 안 된다.
 */
UCLASS()
class R1_API AWaterBottle : public AHeldItemBase
{
	GENERATED_BODY()

public:
	AWaterBottle();

	//~ Begin AHeldItemBase Interface
	virtual void OnPrimaryActionStarted() override;
	virtual void OnSecondaryActionStarted() override;
	//~ End AHeldItemBase Interface

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DrinkWater();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FillWater();

	// OwnerCharacter 전방 FillTraceDistance 지점에 AWaterBody가 있는지 검사한다.
	bool TraceForWaterBody() const;

	// 지금 손에 든 벨트 슬롯이 확실히 "이 병"의 슬롯인지 확인한 뒤, 그 슬롯의 ItemData만 NewData로
	// 바꿔 끼운다(수량/InstanceID는 그대로 유지 — 같은 물리적 병의 상태만 바뀐 것이므로).
	void SwapBottleState(UHeldItemData* NewData);

	// 지금 들고 있는 게 채워진 상태인지.
	bool IsFilled() const;

protected:
	// 빈 상태의 아이템 정의 — 마신 직후 이걸로 전환한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaterBottle")
	TObjectPtr<UHeldItemData> EmptyBottleData;

	// 채워진 상태의 아이템 정의 — 채운 직후 이걸로 전환한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaterBottle")
	TObjectPtr<UHeldItemData> FilledBottleData;

	// 한 번 마실 때 회복되는 수분(Hydration) 양 — UStatComponent::ApplyItemEffect(RestoreThirst)로 전달.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaterBottle")
	float ThirstRestoreAmount = 40.f;

	// 채우기 판정 사거리(cm, 캐릭터 위치 기준 전방 거리).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaterBottle")
	float FillTraceDistance = 150.f;

	// 채우기 판정에 쓰는 구체 반경(cm).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaterBottle")
	float FillTraceRadius = 60.f;

	// 판정 지점의 높이 보정(cm) — OwnerCharacter->GetActorLocation()이 캡슐 중심(가슴 근처) 높이라
	// 그대로 쓰면 수면보다 높게 뜬다. 음수를 주면 그만큼 아래로 내려가서 판정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaterBottle")
	float FillTraceHeightOffset = -60.f;
};
