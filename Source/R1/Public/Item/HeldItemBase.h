/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HeldItemBase.generated.h"

class AActionCharacter;
class UHeldItemData;
class UStaticMeshComponent;

/**
 * 손에 들 수 있는 모든 도구, 근접 무기, 특수 장비의 공통 부모 액터 추상 클래스
 */
UCLASS()
class R1_API AHeldItemBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AHeldItemBase();

	virtual void BeginPlay() override;

	// 장착 및 해제 라이프사이클 이벤트
	virtual void OnEquipped(AActionCharacter* InCharacter);
	virtual void OnUnequipped();

	// 데이터 에셋의 메시 세팅
	virtual void InitItemVisual(UHeldItemData* InItemData);

	// 좌클릭 액션 (주 액션: 공격, 휘두르기, 캐스팅 등)
	virtual void OnPrimaryActionStarted();
	virtual void OnPrimaryActionCompleted() {}

	// 우클릭 액션 (보조 액션: 조준, 가드, 투척 준비 등)
	virtual void OnSecondaryActionStarted();
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

	// 메시 컴포넌트 접근자
	UFUNCTION(BlueprintPure, Category = "HeldItem")
	FORCEINLINE UStaticMeshComponent* GetItemMesh1P() const { return ItemMesh1P; }

	UFUNCTION(BlueprintPure, Category = "HeldItem")
	FORCEINLINE UStaticMeshComponent* GetItemMesh3P() const { return ItemMesh3P; }

	// 아이템 데이터 접근자
	UFUNCTION(BlueprintPure, Category = "HeldItem")
	FORCEINLINE UHeldItemData* GetItemData() const { return ItemData; }

	UFUNCTION(BlueprintCallable, Category = "HeldItem|State")
	void ToggleState();

	UFUNCTION(BlueprintCallable, Category = "HeldItem|State")
	void SetActiveState(bool InValue);

	UFUNCTION(BlueprintCallable, Category = "HeldItem|State")
	FORCEINLINE bool GetActiveState() { return bIsActive; }

	// 오프셋 접근자 (ItemData 우선, 없을 시 Default 설정값 반환)
	UFUNCTION(BlueprintPure, Category = "HeldItem|Offset")
	FTransform GetItemMesh1POffset() const;

	UFUNCTION(BlueprintPure, Category = "HeldItem|Offset")
	FTransform GetItemMesh3POffset() const;

	UFUNCTION(BlueprintPure, Category = "HeldItem|Offset")
	FVector GetFirstPersonMeshLocation() const;

	UFUNCTION(BlueprintPure, Category = "HeldItem|Offset")
	FRotator GetFirstPersonMeshRotation() const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnRep_ItemData();

	UFUNCTION(Server, Reliable)
	void Server_PlayPrimaryActionMontage();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayPrimaryActionMontage();

	UFUNCTION(Server, Reliable)
	void Server_PlaySecondaryActionMontage();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySecondaryActionMontage();

	UFUNCTION()
	virtual void OnRep_IsActiveState();

	// 상태가 바뀔 때 사용하는 이벤트 함수
	UFUNCTION(BlueprintCallable, Category = "HeldItem|State")
	virtual void OnItemStateChanged(bool bNewState);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "HeldItem")
	TObjectPtr<AActionCharacter> OwnerCharacter;

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, BlueprintReadOnly, Category = "HeldItem")
	TObjectPtr<UHeldItemData> ItemData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeldItem|ItemMesh")
	TObjectPtr<UStaticMeshComponent> ItemMesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeldItem|ItemMesh")
	TObjectPtr<UStaticMeshComponent> ItemMesh3P;

	UPROPERTY(ReplicatedUsing = OnRep_IsActiveState, BlueprintReadOnly, Category = "HeldItem|State")
	bool bIsActive = false;

	// ----------------------------------------------------
	// 기본 오프셋 설정 (ItemData가 없을 때 fallback으로 사용)
	// ----------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Offset")
	FVector DefaultFirstPersonMeshLocation = FVector(0.f, 0.f, -130.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Offset")
	FRotator DefaultFirstPersonMeshRotation = FRotator(0.f, -90.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Offset")
	FTransform DefaultItemMesh1POffset = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeldItem|Offset")
	FTransform DefaultItemMesh3POffset = FTransform::Identity;
};
