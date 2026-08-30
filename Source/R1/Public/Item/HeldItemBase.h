/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HeldItemBase.generated.h"

class AActionCharacter;

/**
 * 손에 들 수 있는 모든 도구, 근접 무기, 특수 장비의 공통 부모 액터 추상 클래스
 */
UCLASS(Abstract)
class R1_API AHeldItemBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AHeldItemBase();

	virtual void BeginPlay() override;

	// 장착 및 해제 라이프사이클 이벤트
	virtual void OnEquipped(AActionCharacter* InCharacter);
	virtual void OnUnequipped();

	// 좌클릭 액션 (주 액션: 공격, 휘두르기, 캐스팅 등)
	virtual void OnPrimaryActionStarted() {}
	virtual void OnPrimaryActionCompleted() {}

	// 우클릭 액션 (보조 액션: 조준, 가드, 투척 준비 등)
	virtual void OnSecondaryActionStarted() {}
	virtual void OnSecondaryActionCompleted() {}

	// 액션 취소 (ESC, 점프 등)
	virtual void OnCancelAction() {}

	// 캐릭터 이동 입력 중계 (이동이 차단된 상태에서 도구가 이동 입력을 활용할 때: 예 - 낚시 A/D 저항, S 릴링)
	virtual void OnMoveInput(const FVector2D& MoveValue) {}

	// 캐릭터 입력 컴포넌트 바인딩
	virtual void SetupInputComponent(class UEnhancedInputComponent* PlayerEIC) {}

	// 캐릭터 동작 제어 질의
	virtual bool BlocksCharacterMovement() const { return false; }
	virtual bool BlocksDefaultAttack() const { return true; }

	// 소유자 캐릭터 반환
	UFUNCTION(BlueprintPure, Category = "HeldItem")
	AActionCharacter* GetOwnerCharacter() const { return OwnerCharacter; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "HeldItem")
	TObjectPtr<AActionCharacter> OwnerCharacter;
};
