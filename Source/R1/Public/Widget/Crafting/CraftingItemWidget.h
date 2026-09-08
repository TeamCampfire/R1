#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingItemWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UItemDataBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftingItemClicked, UItemDataBase*, Item);

// 레시피와 큐가 공유하는 아이콘 타일. 큐의 숫자는 WBP Overlay에 배치한다.
UCLASS()
class R1_API UCraftingItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetItem(UItemDataBase* InItem);
	void SetRecipeState(bool bCraftable, bool bSelected);
	void SetQueueState(int32 Count, float Seconds, bool bCompleted);

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingItemClicked OnItemClicked;

	UItemDataBase* GetItem() const { return Item; }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ItemButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> RemainingTimeText;

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	TObjectPtr<UItemDataBase> Item;
};
