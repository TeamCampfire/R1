/// 최초작성 : 2026.08.31
/// 작 성 자 : 최 요 환
/// 간단설명 : 벨트(퀵슬롯) 바 위젯 — 인벤토리 패널을 열고 닫는 것과 무관하게 항상 HUD에 떠있다.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/InventoryComponent.h"
#include "BeltBarWidget.generated.h"

class UInventoryComponent;
class UPanelWidget;
class UInventorySlotWidget;

/**
 * 벨트(퀵슬롯) 바의 C++ 베이스. AMainHUD가 BeginPlay에서 생성해 상시 뷰포트에 띄운다 —
 * 장비/메인을 보여주는 UInventoryWidget(토글 패널)과는 별개의 위젯 인스턴스다.
 * 슬롯 그리드 생성 로직은 UInventorySlotWidget::EnsureGridSlots를 그대로 재사용한다.
 *
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다:
 * - BeltSlotContainer : 벨트 슬롯이 배치될 패널(UniformGridPanel 권장).
 * 클래스 디폴트에서 SlotWidgetClass를 WBP_InventorySlot(UInventorySlotWidget 부모)으로 지정해야 한다.
 */
UCLASS()
class R1_API UBeltBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> BeltSlotContainer;

	// 슬롯 하나를 표현할 위젯 클래스. WBP 디폴트에서 UInventorySlotWidget 부모 WBP로 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 GridColumns = 6;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleSlotDropped(FInventorySlotRef FromSlot, FInventorySlotRef ToSlot);

	UFUNCTION()
	void HandleSlotClicked(FInventorySlotRef SlotRef);

	UFUNCTION()
	void HandleSlotRightClicked(FInventorySlotRef SlotRef);

	UFUNCTION()
	void HandleSlotDragCancelled(FInventorySlotRef SlotRef);

	// 언바인딩용으로 보관. 소유 폰이 사라지는 경우도 있어 약한 참조로 들고 있는다.
	TWeakObjectPtr<UInventoryComponent> BoundInventory;

	TArray<TObjectPtr<UInventorySlotWidget>> BeltSlotWidgets;
};
