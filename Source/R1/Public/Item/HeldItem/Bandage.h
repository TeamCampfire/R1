/// 최초작성 : 2026.09.09
/// 작 성 자 : 최 요 환
/// 간단설명 : 좌클릭으로 낱개 소모하는 붕대 HeldItem

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/HeldItemBase.h"
#include "Bandage.generated.h"

/**
 * 붕대 도구 액터.
 *
 * 다른 HeldItem(무기/도구)과 달리 UHeldItemData::bAllowStacking = true로 설정해 벨트 슬롯에
 * 여러 개(스택)를 들고 다닐 수 있다. 좌클릭(주 액션)마다 UItemDataBase::Effects(지혈/힐 등)를
 * 자신에게, 우클릭(보조 액션)은 조준 중인 다른 플레이어 캐릭터에게 1회 적용하고 스택을 1개
 * 소모한다 — 소모/효과 적용 자체는 서버 권위 RPC(Server_UseBandage/Server_UseBandageOnTarget)를
 * 거친다(인벤토리 슬롯을 바꾸는 작업이라 클라이언트가 직접 실행하면 안 됨, AWaterBottle과 동일한 이유).
 *
 * 스택이 0이 되면 슬롯에 빈 FItemInstance를 써넣기만 하면 된다 — 손에서 내리는 처리(HeldItemComponent
 * ::UnequipHeldItem)는 UInventoryComponent::SetSlot이 "손에 든 벨트 슬롯의 ItemData가 null로
 * 바뀌면 자동 해제"하도록 이미 처리해주므로 이 액터가 직접 신경 쓸 필요가 없다.
 *
 * 우클릭 대상 판정은 정밀한 라인트레이스 대신 스피어 트레이스(Sweep)를 쓴다 — 조준이 살짝
 * 빗나가도 맞은 것으로 인정해주기 위한 편의성 조치. 현재 팀/PvP 구분 시스템이 없어 맞은
 * AActionCharacter는 자기 자신만 아니면 누구든(다른 팀 개념 없음) 허용한다.
 */
UCLASS()
class R1_API ABandage : public AHeldItemBase
{
	GENERATED_BODY()

public:
	ABandage();

	//~ Begin AHeldItemBase Interface
	virtual void OnPrimaryActionStarted() override;
	virtual void OnSecondaryActionStarted() override;
	//~ End AHeldItemBase Interface

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseBandage();

	// 조준 중인 다른 플레이어 캐릭터에게 효과를 적용한다 — 대상 판정(TraceForTargetCharacter)은
	// 클라이언트를 신뢰하지 않고 서버에서 직접 다시 수행한다(AWaterBottle::Server_FillWater와 동일한 이유).
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseBandageOnTarget();

	// 지금 손에 든 벨트 슬롯이 확실히 "이 붕대"의 슬롯인지 확인한 뒤 스택을 1개 소모한다.
	// 소모 후 남은 수량이 0이면 슬롯을 비운다(자동 해제는 SetSlot이 처리).
	void ConsumeOneCharge();

	// OwnerCharacter 정면으로 스피어 트레이스를 쏴서 맞은 AActionCharacter를 반환한다(자기 자신 제외,
	// 없으면 nullptr). 서버(Server_UseBandageOnTarget)에서만 호출해 대상을 신뢰성 있게 확정한다.
	AActionCharacter* TraceForTargetCharacter() const;

protected:
	// 우클릭 대상 판정 사거리(cm, 캐릭터 위치 기준 전방 거리) — BP 서브클래스에서 조절 가능.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bandage|Target")
	float TargetTraceDistance = 100.f;

	// 우클릭 대상 판정에 쓰는 스피어 반경(cm) — 클수록 조준이 관대해진다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bandage|Target")
	float TargetTraceRadius = 30.f;
};
