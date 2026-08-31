/// 최초작성 : 2026.08.30
/// 작 성 자 : 최 요 환
/// 간단설명 : 인벤토리 패널(장비+메인) 위젯 — 토글로 열고 닫는 화면. 벨트는 별개(UBeltBarWidget, 항상 HUD에 떠있음).

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/InventoryComponent.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;
class UTextBlock;
class UPanelWidget;
class UInventorySlotWidget;

/**
 * 인벤토리 패널(장비+메인) 위젯의 C++ 베이스.
 *
 * 벨트는 여기 없다 — 벨트는 인벤토리를 열고 닫는 것과 무관하게 항상 화면에 떠있어야 해서
 * 별도 위젯(UBeltBarWidget)이 담당하고, AMainHUD가 상시 띄운다. 여기와 UBeltBarWidget
 * 둘 다 슬롯 그리드를 채우는 로직은 UInventorySlotWidget::EnsureGridSlots를 공유한다.
 *
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다(전부 BindWidgetOptional):
 * - InventoryStateText     : 슬롯 상태 전체를 찍는 TextBlock(디버그용, 그대로 유지).
 * - EquipmentSlotContainer : 장비 슬롯이 배치될 패널(UniformGridPanel 권장).
 * - MainSlotContainer      : 메인 슬롯이 배치될 패널.
 * 그리고 클래스 디폴트에서 SlotWidgetClass를 WBP_InventorySlot(UInventorySlotWidget 부모)으로 지정해야
 * 슬롯 패널이 채워진다.
 */
UCLASS()
class R1_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 레벨 블루프린트 등에서 CreateWidget 특수 노드 없이도 안전하게(Initialize까지 거쳐서)
	// 이 위젯을 만들어 화면에 띄우기 위한 테스트용 헬퍼.
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (WorldContext = "WorldContextObject"))
	static UInventoryWidget* ShowInventoryTestWidget(UObject* WorldContextObject, TSubclassOf<UInventoryWidget> WidgetClass);

	// 선택 상태(파란 테두리)를 초기화한다 — 패널이 닫힐 때 선택이 남아있지 않도록
	// MainHUDWidget::ToggleInventoryPanel이 닫는 시점에 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearSelection();

protected:
	//~ Begin UUserWidget Interface
	// WBP 디자이너 프리뷰 전용 — PIE 밖에서는 BoundInventory가 없어 그리드가 비어 보이므로,
	// IsDesignTime()일 때만 UInventoryComponent CDO의 슬롯 개수만큼 더미로 미리 채운다.
	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InventoryStateText;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EquipmentSlotContainer;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> MainSlotContainer;

	// 슬롯 하나를 표현할 위젯 클래스. WBP 디폴트에서 UInventorySlotWidget 부모 WBP로 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

	// 컨테이너가 UniformGridPanel이면 이 값 기준으로 Row/Column을 계산해서 줄맞춤한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 GridColumns = 6;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshStateText();
	void RebuildSlots();

	UFUNCTION()
	void HandleSlotDropped(FInventorySlotRef FromSlot, FInventorySlotRef ToSlot, int32 Count, bool bAutoHalfSplitOnEmptyTarget);

	// 슬롯 클릭 → 선택 상태로 만들기만 한다(실제 파란 테두리 갱신은 다음 HandleInventoryChanged에서).
	UFUNCTION()
	void HandleSlotClicked(FInventorySlotRef SlotRef);

	// 슬롯 우클릭 → 빠른 이동/장착(QuickMoveItem)을 시도한다.
	UFUNCTION()
	void HandleSlotRightClicked(FInventorySlotRef SlotRef);

	// 유효한 드롭 대상 없이(슬롯 밖) 드래그가 끝났을 때 → 월드에 버린다(ThrowItem).
	UFUNCTION()
	void HandleSlotDragCancelled(FInventorySlotRef SlotRef);

	// 언바인딩용으로 보관. 소유 폰이 사라지는 경우도 있어 약한 참조로 들고 있는다.
	TWeakObjectPtr<UInventoryComponent> BoundInventory;

	TArray<TObjectPtr<UInventorySlotWidget>> EquipmentSlotWidgets;
	TArray<TObjectPtr<UInventorySlotWidget>> MainSlotWidgets;
};
