#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingWidget.generated.h"

class UCraftingComponent;
class UItemDataBase;
class AWorkbench;
class SVerticalBox;
class SWrapBox;
enum class ECheckBoxState : uint8;

/** 기본 제작과 작업대 제작이 공유하는 화면. 인벤토리 변경은 서버 컴포넌트에 요청한다. */
UCLASS()
class R1_API UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<UCraftingComponent> Crafting;

	UPROPERTY()
	TObjectPtr<AWorkbench> Bench;

	UPROPERTY()
	TObjectPtr<UItemDataBase> Selected;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;

private:
	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildRecipePanel();
	TSharedRef<SWidget> BuildDetailsPanel();
	TSharedRef<SWidget> BuildQuantityControls();
	// 검색·선택·수량·제작·취소 이벤트 처리.
	FReply HandleCloseClicked();
	void HandleSearchChanged(const FText& Text);
	void HandleCraftableFilterChanged(ECheckBoxState State);
	FReply HandleDecreaseClicked();
	FReply HandleIncreaseClicked();
	FReply HandleMaximumClicked();
	FReply HandleCraftClicked();
	FReply HandleRecipeClicked(UItemDataBase* Item);
	FReply HandleCancelClicked(FGuid OrderId);

	// 목록, 재료, 대기열 표시 갱신.
	void Refresh();
	void RefreshRecipes();
	void RefreshMaterials();
	void RefreshQueue();
	int32 GetMaxCraftableQuantity() const;

	// 수량은 1 이상으로 기억하되 제작 불가능하면 화면에는 0으로 표시한다.
	int32 CraftQuantity = 1;
	FString NormalizedSearchText;
	bool bOnlyCraftable = false;
	float RefreshElapsed = 0.f;
	TSharedPtr<SWrapBox> RecipeContainer;
	TSharedPtr<SVerticalBox> MaterialContainer;
	TSharedPtr<SWrapBox> QueueContainer;
};
