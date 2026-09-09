#include "Widget/Crafting/CraftingItemWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/Item/ItemDataBase.h"

void UCraftingItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ItemButton->OnClicked.AddDynamic(this, &UCraftingItemWidget::HandleClicked);
}

void UCraftingItemWidget::SetItem(UItemDataBase* InItem)
{
	if (Item == InItem) return;
	Item = InItem;
	ItemIcon->SetBrushFromTexture(Item ? Item->Icon.LoadSynchronous() : nullptr);
	ItemNameText->SetText(Item ? Item->DisplayName : FText::GetEmpty());
	SetToolTipText(Item ? Item->DisplayName : FText::GetEmpty());
}

// [wdk59] 레시피의 제작 가능 여부와 선택 강조를 표시하며 큐에서는 같은 타일을 수량·시간 표시로 재사용한다.
void UCraftingItemWidget::SetRecipeState(bool bCraftable, bool bSelected)
{
	// 클릭 자체는 허용한다. 재료가 부족해도 상세 설명을 확인할 수 있다.
	ItemButton->SetIsEnabled(true);
	ItemButton->SetVisibility(ESlateVisibility::Visible);
	ItemNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ItemIcon->SetRenderOpacity(bCraftable ? 1.f : 0.3f);
	ItemNameText->SetRenderOpacity(bCraftable ? 1.f : 0.5f);
	ItemButton->SetBackgroundColor(bSelected ? FLinearColor(0.18f, 0.45f, 0.62f, 1.f) : FLinearColor(0.17f, 0.17f, 0.15f, 1.f));
	CountText->SetVisibility(ESlateVisibility::Collapsed);
	RemainingTimeText->SetVisibility(ESlateVisibility::Collapsed);
}

void UCraftingItemWidget::SetQueueState(int32 Count, float Seconds, bool bCompleted)
{
	ItemButton->SetIsEnabled(true);
	ItemButton->SetVisibility(ESlateVisibility::HitTestInvisible);
	ItemNameText->SetVisibility(ESlateVisibility::Collapsed);
	ItemIcon->SetRenderOpacity(1.f);
	CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
	RemainingTimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	CountText->SetText(FText::Format(NSLOCTEXT("Crafting", "Count", "×{0}"), FText::AsNumber(Count)));
	RemainingTimeText->SetText(bCompleted ? NSLOCTEXT("Crafting", "Completed", "회수 대기")
		: FText::Format(NSLOCTEXT("Crafting", "Seconds", "{0}초"), FText::AsNumber(FMath::CeilToInt(Seconds))));
}

void UCraftingItemWidget::HandleClicked()
{
	if (Item) OnItemClicked.Broadcast(Item);
}
