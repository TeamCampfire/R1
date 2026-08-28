/// 최초작성 : 2026.08.25
/// 작 성 자 : 최 요 환
/// 간단설명 : 플레이어 캐릭터의 애니메이션 클래스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ActionAnimInstance.generated.h"

class AActionCharacter;
class UCharacterMovementComponent;
/**
 * 
 */
UCLASS()
class R1_API UActionAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float Speed = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float Direction = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsCrouched = false;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsFalling = false;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsSprinting = false;

private:
    UPROPERTY()
    TObjectPtr<AActionCharacter> OwningCharacter;

    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> MovementComponent;

};
