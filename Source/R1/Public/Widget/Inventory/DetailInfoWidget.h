/// 최초작성 : 2026.08.31
/// 작 성 자 : 최 요 환
/// 간단설명 : 인벤토리에서 선택된 슬롯의 아이템 상세 정보(이름/설명/아이콘/스탯/액션/분할)를 보여주는 위젯

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/InventoryComponent.h"
#include "DetailInfoWidget.generated.h"

class UInventoryComponent;
class UParameterBarWidget;
class UTextBlock;
class UImage;
class UButton;
class USlider;
class UPanelWidget;
class UWidget;
class UDragDropOperation;
struct FEquipmentStatModifier;
struct FItemEffect;
enum class EEquipmentStatType : uint8;
enum class EItemEffectType : uint8;

/**
 * 인벤토리(UInventoryComponent)에서 선택된 슬롯(SelectSlot으로 지정된 슬롯)의 아이템 정보를
 * 보여주는 상세 정보 패널. OnInventoryChanged/OnSelectionChanged 둘 다 구독해서, 선택이
 * 바뀌거나(클릭) 선택된 슬롯의 내용물이 바뀌면(수량 변화 등) 매번 다시 그린다.
 *
 * WBP(WBP_DetailInfo)에서 아래 이름 + 타입으로 배치하면 자동 바인딩된다(전부 BindWidgetOptional):
 * - RootPanel             : 패널 전체를 감싸는 컨테이너. 선택된 슬롯이 없으면 Collapsed.
 * - TitleText             : 아이템 이름.
 * - DescriptionText       : 아이템 설명.
 * - IconImage             : 아이템 아이콘. 아이콘이 없는 아이템은 자동으로 숨겨진다.
 * - InfoRowsContainer     : "정보" 섹션의 스탯 행이 채워질 패널(VerticalBox 권장) — 장비/도구는
 *                           라벨+StatBarWidgetClass 바, 소비는 색상 있는 텍스트 한 줄씩 채워진다.
 * - ActionButtonsContainer: 카테고리별 조건부 액션 버튼들을 담는 레이아웃용 컨테이너(선택 사항 —
 *                           버튼 자체는 런타임 생성이 아니라 아래처럼 WBP에 미리 배치해둔다).
 * - UseButton             : "사용" 버튼 — Consumable일 때만 보이고 그 외엔 Collapsed
 *                           (UInventoryComponent::UseSelectedItem 호출). 버튼이 늘어나면 같은
 *                           패턴(BindWidgetOptional + RebuildActionButtons에서 SetVisibility)으로 추가.
 * - DiscardButton         : "버리기" 버튼 — ThrowItem 호출.
 * - SplitPanel            : 분할 섹션 전체를 감싸는 컨테이너. 스택 불가/StackCount<=1이면 Collapsed.
 * - SplitQuantitySlider   : "나눌 수량" 슬라이더(1 ~ StackCount-1, 최소 1개는 항상 원본에 남긴다).
 * - SplitQuantityText     : 슬라이더 값을 숫자로 보여주는 텍스트.
 * - SplitDragIcon         : 분할된 스택을 드래그해서 슬롯 위젯에 놓는 아이콘 — 이 위젯 위에서
 *                           마우스를 누른 채 드래그를 시작하면 UInventoryDragDropOperation(Count =
 *                           CurrentSplitCount)을 만들어 보낸다. 실제 이동은 드롭받은 슬롯 위젯이
 *                           속한 UInventoryWidget/UBeltBarWidget이 TransferItem으로 처리한다.
 */
UCLASS()
class R1_API UDetailInfoWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	//~ End UUserWidget Interface

private:
	// 인벤토리 컴포넌트 바인딩(최초 1회 + 부활 등으로 폰이 바뀔 때마다) — 옛 컴포넌트 델리게이트
	// 해제 후 새 폰의 컴포넌트를 다시 찾아 구독한다. AActionPlayerController::OnPossessedCharChange에
	// 구독해서 부활 시에도 다시 호출되게 한다.
	UFUNCTION()
	void RebindInventory();

	// RebindInventory/NativeDestruct 양쪽에서 공유하는 델리게이트 해제 로직.
	void UnbindInventoryDelegates();

	UFUNCTION()
	void HandleInventoryChanged();

	// Title/Description/Icon/Split 슬라이더 범위 등 전체 갱신. 선택 없음이면 RootPanel만 Collapsed.
	void RefreshDisplay();

	// "정보" 섹션 스탯 행 재생성. 장비/도구(UEquipmentItemData::StatModifiers)는 Bar 타입
	// (StatBarWidgetClass 재사용), 소비(UConsumableItemData::Effects)는 색상 있는 텍스트 행.
	void RebuildInfoRows(const FItemInstance& Selected);

	// 스탯 하나를 라벨 + StatBarWidgetClass 바 한 행으로 만들어 InfoRowsContainer에 추가.
	void AddStatBarRow(EEquipmentStatType StatType, float Value);

	// 소비 효과 하나를 색상 있는 텍스트 한 줄로 만들어 InfoRowsContainer에 추가.
	void AddEffectTextRow(const FItemEffect& Effect);

	// 바 표시용 "기준 최대값" — 실제 스탯의 진짜 최대치가 아니라, 바를 얼마나 채울지 정하기
	// 위한 순전히 시각적인 기준값이다. 스탯 종류별로 의미(가산 vs 배율)가 달라서 값도 다르게
	// 잡아야 하므로 여기 한 곳에 모아두고 밸런스가 잡히면 이 값만 조정한다.
	static float GetStatBarReferenceMax(EEquipmentStatType StatType);

	// 소비 효과 종류별 텍스트 색상 — Heal은 붉은 계열, RestoreHunger/RestoreThirst는 서로
	// 구분되게 각각 다른 색을 쓴다.
	static FLinearColor GetEffectColor(EItemEffectType EffectType);

	// 조건부 액션 버튼("사용" 등) 표시/숨김 갱신 — 버튼은 WBP에 미리 배치돼 있고 여기서는
	// 카테고리에 맞는 것만 Visibility를 켜고 나머지는 Collapsed로 둔다.
	void RebuildActionButtons(const FItemInstance& Selected);

	UFUNCTION()
	void HandleDiscardClicked();

	UFUNCTION()
	void HandleUseClicked();

	UFUNCTION()
	void HandleSplitQuantityChanged(float Value);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RootPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> InfoRowsContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ActionButtonsContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> DiscardButton;

	// Consumable일 때만 보이는 "사용" 버튼 — WBP에 미리 배치, 카테고리에 따라 Visibility만 토글한다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> UseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SplitPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USlider> SplitQuantitySlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SplitQuantityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> SplitDragIcon;

	// "정보" 섹션의 Bar 타입 스탯 행에 쓸 위젯 클래스. UParameterBarWidget(라벨+바+값, 강진구
	// 작성분)을 그대로 재사용 — WBP 디폴트에서 실제 WBP_ParameterBar로 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DetailInfo")
	TSubclassOf<UParameterBarWidget> StatBarWidgetClass;

private:
	TWeakObjectPtr<UInventoryComponent> BoundInventory;

	// 분할 슬라이더가 현재 가리키는 수량 — 선택이 바뀌어도(슬라이더가 새 범위로 재설정돼도)
	// 이전에 고르던 값을 최대한 유지하기 위해 기억해둔다.
	int32 CurrentSplitCount = 1;
};
