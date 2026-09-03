/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/HeldItemBase.h"
#include "HeldItemComponent.generated.h"

class AActionCharacter;
class UItemDataBase;
class UEquipmentItemData;

/**
 * 손에 쥐는 도구, 근접 무기, 특수 장비의 장착/해제 및 액션 입력 중계를 전담하는 컴포넌트
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class R1_API UHeldItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHeldItemComponent();

	virtual void BeginPlay() override;

	// 아이템 데이터 기반 장착
	UFUNCTION(BlueprintCallable, Category = "HeldItem")
	AHeldItemBase* EquipItem(UItemDataBase* ItemData);

	UFUNCTION(BlueprintCallable, Category = "HeldItem")
	AHeldItemBase* EquipHeldItemByData(UEquipmentItemData* EquipItemData);

	// 클래스 직접 장착
	UFUNCTION(BlueprintCallable, Category = "HeldItem")
	AHeldItemBase* EquipHeldItemByClass(TSubclassOf<AHeldItemBase> ItemClass);

	// 손에 든 아이템 해제
	UFUNCTION(BlueprintCallable, Category = "HeldItem")
	void UnequipHeldItem();

	// 좌클릭 액션 (주 사용)
	UFUNCTION(BlueprintCallable, Category = "HeldItem|Input")
	void UsePrimaryAction(bool bStarted);

	// 우클릭 액션 (보조 기능)
	UFUNCTION(BlueprintCallable, Category = "HeldItem|Input")
	void UseSecondaryAction(bool bStarted);

	// 액션 취소
	UFUNCTION(BlueprintCallable, Category = "HeldItem|Input")
	void CancelAction();

	// 이동 입력 중계
	void OnMoveInput(const FVector2D& MoveValue);

	// 캐릭터 상태 제어 질의
	UFUNCTION(BlueprintPure, Category = "HeldItem|State")
	bool BlocksCharacterMovement() const;

	UFUNCTION(BlueprintPure, Category = "HeldItem|State")
	bool BlocksDefaultAttack() const;

	// 현재 손에 든 아이템 액터 반환
	UFUNCTION(BlueprintPure, Category = "HeldItem")
	FORCEINLINE AHeldItemBase* GetCurrentHeldItem() const { return CurrentHeldItem; }

	// 현재 장착된 아이템 데이터 반환
	UFUNCTION(BlueprintPure, Category = "HeldItem")
	FORCEINLINE UEquipmentItemData* GetCurrentEquippedItemData() const { return CurrentEquippedItemData; }

	// 특정 도구/무기 클래스로 안전하게 캐스팅하여 반환
	template<typename T>
	T* GetCurrentHeldItemOf() const
	{
		return Cast<T>(CurrentHeldItem);
	}

protected:
	// 게임 시작 시 컴포넌트에서 자동 장착할 기본 아이템 데이터 (에디터 디테일 패널에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeldItem|Default")
	TObjectPtr<UEquipmentItemData> DefaultItemData;

	// 아이템 데이터 대신 액터 클래스로 직접 지정하고 싶을 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeldItem|Default")
	TSubclassOf<AHeldItemBase> DefaultHeldItemClass;

	UPROPERTY(BlueprintReadOnly, Category = "HeldItem|Runtime")
	TObjectPtr<AHeldItemBase> CurrentHeldItem;

	UPROPERTY(BlueprintReadOnly, Category = "HeldItem|Runtime")
	TObjectPtr<UEquipmentItemData> CurrentEquippedItemData;

	UPROPERTY()
	TObjectPtr<AActionCharacter> OwnerCharacter;
};
