#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingWidget.generated.h"

class UCraftingComponent;
class UCraftingItemWidget;
class UCraftingMaterialWidget;
class UItemDataBase;
class AWorkbench;
class UWrapBox;
class UVerticalBox;
class UEditableTextBox;
class UButton;
class UTextBlock;
class UImage;

// HUD에 바인딩된 제작창을 Q와 작업대 상호작용에서 재사용하고 데이터 원본만 교체
// 제작 화면의 검색/수량/재료/큐 표시를 조정
// 개별 타일과 재료 행은 분리된 위젯으로 구성
UCLASS()
class R1_API UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindCrafting(UCraftingComponent* InCrafting, AWorkbench* InBench);
	// 창을 숨길 때 화면의 참조만 해제
	// 실제 제작 큐는 컴포넌트에 유지
	void UnbindCrafting();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
	UFUNCTION()
	void HandleSearchChanged(const FText& Text);
	UFUNCTION()
	void HandleRecipeClicked(UItemDataBase* Item);
	UFUNCTION()
	void HandleDecrease();
	UFUNCTION()
	void HandleIncrease();
	UFUNCTION()
	void HandleMaximum();
	UFUNCTION()
	void HandleCraft();
	UFUNCTION()
	void HandleCollect();
	UFUNCTION()
	void HandleClose();

	static FString NormalizeSearchText(const FString& Text);
	void RebuildRecipes();
	void Refresh();
	void RefreshQueue();
	UCraftingComponent* GetQueueSource() const;
	int32 GetMaximum() const;

protected :

	UPROPERTY(EditDefaultsOnly, Category = "Crafting")
	TSubclassOf<UCraftingItemWidget> ItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Crafting")
	TSubclassOf<UCraftingMaterialWidget> MaterialWidgetClass;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWrapBox> RecipeContainer;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> SearchBox;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWrapBox> QueueContainer;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWrapBox> CompletedContainer;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> MaterialContainer;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> SelectedIcon;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SelectedNameText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DurationText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> QuantityText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> DecreaseButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> IncreaseButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> MaximumButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CraftButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CollectButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CloseButton;

private :

	UPROPERTY()
	TObjectPtr<UCraftingComponent> Crafting;

	UPROPERTY()
	TObjectPtr<UItemDataBase> Selected;

	TWeakObjectPtr<AWorkbench> BoundBench;

	bool bWorkbenchMode = false;

	FString SearchText;

	int32 CraftQuantity = 0;

	float RefreshElapsed = 0.f;
};
