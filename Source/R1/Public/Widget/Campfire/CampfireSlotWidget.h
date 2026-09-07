#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Campfire/CampfireTypes.h"
#include "Component/InventoryComponent.h"
#include "Item/ItemInstance.h"
#include "CampfireSlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UWidget;
class UTexture2D;
class ACampfireActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCampfireSlotRightClicked, ACampfireActor*, Campfire, FCampfireSlotRef, CampfireSlotRef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnInventoryDroppedOnCampfire, ACampfireActor*, Campfire,
	FInventorySlotRef, From, FCampfireSlotRef, To, int32, Count, bool, bHalfSplit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnCampfireSlotDropped, ACampfireActor*, Campfire,
	FCampfireSlotRef, From, FCampfireSlotRef, To, int32, Count, bool, bHalfSplit);

UCLASS()
class R1_API UCampfireSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void SynchronizeProperties() override;

	// WBP_Campfire에서 각 슬롯 인스턴스의 안내 이미지를 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Appearance")
	TObjectPtr<UTexture2D> SlotTypeTexture;

	void InitializeSlot(ACampfireActor* InCampfire, const FCampfireSlotRef& InSlot);
	void Refresh(const FItemInstance& Instance);
	const FCampfireSlotRef& GetSlotRef() const { return SlotRef; }

	UPROPERTY(BlueprintAssignable) FOnCampfireSlotRightClicked OnSlotRightClicked;
	UPROPERTY(BlueprintAssignable) FOnInventoryDroppedOnCampfire OnInventoryDropped;
	UPROPERTY(BlueprintAssignable) FOnCampfireSlotDropped OnCampfireDropped;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UImage> SlotTypeIcon;

protected:

	virtual FReply NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
	virtual void NativeOnDragDetected(const FGeometry&, const FPointerEvent&, UDragDropOperation*&) override;
	virtual void NativeOnDragEnter(const FGeometry&, const FDragDropEvent&, UDragDropOperation*) override;
	virtual void NativeOnDragLeave(const FDragDropEvent&, UDragDropOperation*) override;
	virtual bool NativeOnDrop(const FGeometry&, const FDragDropEvent&, UDragDropOperation*) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UBorder> RootBorder;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UImage> IconImage;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UWidget> CountBox;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> MaxStackText;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UBorder> SelectionBorder;

private:
	void UpdateHoverVisual(bool bHover, bool bAllowed);
	TWeakObjectPtr<ACampfireActor> Campfire;
	FCampfireSlotRef SlotRef;
	FItemInstance CachedInstance;
	bool bMiddleDrag = false;
};
