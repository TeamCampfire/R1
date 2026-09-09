#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/PlaceableItem/Campfire/CampfireTypes.h"
#include "Component/InventoryComponent.h"
#include "Item/ItemInstance.h"
#include "CampfireSlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UWidget;
class UTexture2D;
class ACampfire;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCampfireSlotRightClicked, ACampfire*, Campfire, FCampfireSlotRef, CampfireSlotRef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnInventoryDroppedOnCampfire, ACampfire*, Campfire, FInventorySlotRef, From, FCampfireSlotRef, To, int32, Count, bool, bHalfSplit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnCampfireSlotDropped, ACampfire*, Campfire, FCampfireSlotRef, From, FCampfireSlotRef, To, int32, Count, bool, bHalfSplit);

// 모닥불 슬롯의 아이템 표시
// 허용 드롭 강조와 인벤토리 간 드래그 정보를 담당
UCLASS()
class R1_API UCampfireSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 블루프린트에서 설정한 슬롯 가이드 아이콘을 슬롯에 보이게 설정
	virtual void SynchronizeProperties() override;

	// 슬롯 정보 초기화
	void InitializeSlot(ACampfire* InCampfire, const FCampfireSlotRef& InSlot);

	// 슬롯 정보 새로고침
	void Refresh(const FItemInstance& Instance);

	// 슬롯 참조
	const FCampfireSlotRef& GetSlotRef() const { return SlotRef; }

protected:

	/* 마우스 이벤트 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
	virtual void NativeOnDragDetected(const FGeometry&, const FPointerEvent&, UDragDropOperation*&) override;
	virtual void NativeOnDragEnter(const FGeometry&, const FDragDropEvent&, UDragDropOperation*) override;
	virtual void NativeOnDragLeave(const FDragDropEvent&, UDragDropOperation*) override;
	virtual bool NativeOnDrop(const FGeometry&, const FDragDropEvent&, UDragDropOperation*) override;

private:

	// 호버링 처리
	void UpdateHoverVisual(bool bHover, bool bAllowed);

public:

	/* 아이템 이동 관련 델리게이트 */
	UPROPERTY(BlueprintAssignable)
	FOnCampfireSlotRightClicked OnSlotRightClicked;
	UPROPERTY(BlueprintAssignable)
	FOnInventoryDroppedOnCampfire OnInventoryDropped;
	UPROPERTY(BlueprintAssignable)
	FOnCampfireSlotDropped OnCampfireDropped;

	// WBP_Campfire에서 각 슬롯 인스턴스의 안내 이미지를 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Appearance")
	TObjectPtr<UTexture2D> SlotTypeTexture;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> SlotTypeIcon;

protected :

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidget> CountBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> MaxStackText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UBorder> SelectionBorder;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor AllowedColor = FLinearColor(0.35f, 0.75f, 0.15f, 0.5f);

	UPROPERTY(BlueprintReadOnly)
	FLinearColor DisallowedColor = FLinearColor(0.9f, 0.08f, 0.05f, 0.5f);

private:

	TWeakObjectPtr<ACampfire> Campfire;	// 연결된 모닥불 참조
	FCampfireSlotRef SlotRef;			// 이 슬롯이 담당하는 모닥불 슬롯 위치
	FItemInstance CachedInstance;		// 슬롯에서 표시 중인 아이템 정보 임시 저장용 변수
	bool bMiddleDrag = false;			// 마우스 가운데 버튼으로 드래그 여부, 조건을 충족하는 빈 슬롯 드롭 시 절반 이동에 사용
};
