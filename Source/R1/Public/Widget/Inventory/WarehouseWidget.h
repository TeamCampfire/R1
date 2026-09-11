/// 최초작성 : 2026.09.06
/// 작 성 자 : 최 요 환
/// 간단설명 : 창고 상호작용 시 여는, 플레이어 인벤토리(메인 슬롯)와 창고 저장공간을 나란히 보여주는 위젯

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/InventoryComponent.h"
#include "WarehouseWidget.generated.h"

class UInventorySlotWidget;
class UPanelWidget;
class UWarehouseInventoryComponent;
class UTextBlock;
class UInventoryWidget;
class UBeltBarWidget;

/**
 * 창고 상호작용 시 여는 위젯. 창고 저장 슬롯 그리드 하나만 그린다 — 플레이어 쪽(메인/벨트)은
 * 이 위젯이 따로 그리지 않고, 이미 HUD에 떠 있는 UInventoryWidget/UBeltBarWidget을 그대로
 * 재사용한다(Rust처럼 창고를 열면 내 인벤토리 패널이 옆에 같이 열리는 방식).
 *
 * UInventoryWidget과 다른 점: UInventoryWidget은 "로컬 플레이어 폰의 인벤토리 하나"만 한 번
 * 바인딩하면 끝이지만, 이 위젯은 상호작용마다 다른 창고를 열 수 있으므로 열 때마다
 * (OpenWarehouse) 바인딩 대상을 새로 지정한다.
 *
 * ContainerId(플레이어 쪽 위젯들은 전부 기본값 0, 창고 그리드만 1)로 컨테이너를 구분해서
 * EnsureGridSlots에 넘긴다 — 같은 컨테이너 내부 드래그는 OnSlotDropped(TransferItem)로,
 * 컨테이너를 넘나드는 드래그는 OnSlotDroppedCross(TransferWithWarehouse)로 각각 처리한다.
 * 자세한 설명은 InventorySlotWidget.h의 ContainerId/OnSlotDroppedCross 주석 참고.
 *
 * 크로스 드래그를 받으려면 UInventoryWidget의 메인 슬롯 위젯들과 UBeltBarWidget의 벨트 슬롯
 * 위젯들에도 OnSlotDroppedCross를 추가로 바인딩해야 한다 — 그 위젯들 자신은 항상 같은
 * 컨테이너(자기 자신)로만 드롭될 거라 가정하고 OnSlotDropped만 구독하기 때문에, 창고가 열려있는
 * 동안만 이 위젯이 직접 덧붙여 구독한다(OpenWarehouse에서 추가, CloseWarehouse에서 해제).
 *
 * 범위: 지금은 플레이어 쪽 메인/벨트 슬롯만 창고와 주고받을 수 있다(장비 제외 — 장비를 벗어
 * 창고에 넣는 흐름은 필요해지면 추가). 창고 내부 슬롯끼리 재정렬하는 드래그(같은 컨테이너 내부)는
 * HandleSlotDropped가 UInventoryComponent::TransferWithinWarehouse를 통해 처리한다.
 *
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다:
 * - WarehouseSlotContainer : 창고 저장 슬롯 그리드(UniformGridPanel 권장).
 */
UCLASS()
class R1_API UWarehouseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 창고와의 상호작용으로 이 위젯을 연다. PlayerInventoryWidget/PlayerBeltBarWidget은
	// MainHUDWidget이 이미 갖고 있는 인스턴스를 그대로 넘겨받아 크로스 드래그 바인딩에 쓴다
	// (UMainHUDWidget::OpenWarehousePanel이 InventoryWidget을 강제로 보이게 만든 뒤 호출).
	void OpenWarehouse(UWarehouseInventoryComponent* Warehouse, UInventoryWidget* PlayerInventoryWidget, UBeltBarWidget* PlayerBeltBarWidget);

	// 창고 이용을 그만할 때 호출 — 바인딩 해제 + 패널 숨김 + 게임 입력 복구까지 전부 처리한다.
	// 상호작용 버튼 재입력(AActionPlayerController::Client_OpenWarehouse_Implementation의 토글)이나
	// 인벤토리 토글 버튼(UMainHUDWidget::ToggleInventoryPanel) 양쪽에서 호출될 수 있다.
	UFUNCTION(BlueprintCallable, Category = "Warehouse")
	void CloseWarehouse();

	// AActionPlayerController::Client_OpenWarehouse_Implementation이 "이미 이 창고가 열려있는지"
	// 판단해 재상호작용 시 닫을지 결정하는 데 쓴다.
	UFUNCTION(BlueprintPure, Category = "Warehouse")
	UWarehouseInventoryComponent* GetBoundWarehouse() const { return BoundWarehouse.Get(); }

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

	UFUNCTION()
	void HandleInventoryChanged();

	// 창고 그리드 내부 재정렬(같은 컨테이너 내부 드래그) — Category는 EnsureGridSlots가 채운
	// 형식상의 값(Main)이라 쓰지 않고 Index만 창고 슬롯 번호로 읽는다.
	UFUNCTION()
	void HandleSlotDropped(FInventorySlotRef FromSlot, FInventorySlotRef ToSlot, int32 Count, bool bAutoHalfSplitOnEmptyTarget);

	UFUNCTION()
	void HandleSlotDroppedCross(int32 FromContainerId, FInventorySlotRef FromSlot, int32 ToContainerId, FInventorySlotRef ToSlot, int32 Count);

	// 창고 슬롯 클릭 → 창고 쪽 선택으로 표시(DetailInfoWidget이 이 선택을 보고 상세 정보를 그린다).
	UFUNCTION()
	void HandleSlotClicked(FInventorySlotRef SlotRef);

	// 플레이어 쪽(메인/벨트) 슬롯 클릭 → 창고 선택을 해제한다 — 두 선택이 동시에 파란 테두리로
	// 표시되면 DetailInfoWidget이 뭘 보여줘야 할지 애매해지므로, 플레이어 쪽을 새로 선택하면
	// 창고 쪽 선택은 자동으로 풀어서 "항상 최대 하나만 선택됨"을 보장한다.
	UFUNCTION()
	void HandlePlayerSlotClicked(FInventorySlotRef SlotRef);

	// 창고 슬롯 우클릭 → 창고에서 플레이어 인벤토리로 빠른 이동(카테고리별 벨트/메인 우선순위는
	// UInventoryComponent::QuickMoveFromWarehouse가 처리).
	UFUNCTION()
	void HandleSlotRightClicked(FInventorySlotRef SlotRef);

	void UnbindDelegates();
	void RebuildSlots();
	void BindCrossDropOnPlayerWidgets();
	void UnbindCrossDropOnPlayerWidgets();

	// 창고 쪽 그리드.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> WarehouseSlotContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WarehouseLabel;

	UPROPERTY(EditDefaultsOnly, Category = "Warehouse")
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Warehouse")
	int32 GridColumns = 6;

private:

	FText DefaultWarehouseLabel;	// Looting: 시체 창고는 UI 라벨 내용 바꿔야 돼서 원래 내용 기억

	static constexpr int32 PlayerContainerId = 0;
	static constexpr int32 WarehouseContainerId = 1;

	// 화면에 그리지는 않지만(그리기는 UInventoryWidget/UBeltBarWidget이 담당) HandleSlotDroppedCross가
	// 서버로 전송할 때(Server_TransferWithWarehouse) 필요해서 갖고 있는다.
	TWeakObjectPtr<UInventoryComponent> BoundPlayerInventory;
	TWeakObjectPtr<UWarehouseInventoryComponent> BoundWarehouse;

	// 크로스 드래그 바인딩 해제용으로 보관.
	TWeakObjectPtr<UInventoryWidget> BoundInventoryWidget;
	TWeakObjectPtr<UBeltBarWidget> BoundBeltBarWidget;

	TArray<TObjectPtr<UInventorySlotWidget>> WarehouseSlotWidgets;
};
