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
	if (Item == InItem)
		return;

	Item = InItem;
	ItemIcon->SetBrushFromTexture(Item ? Item->Icon.LoadSynchronous() : nullptr);
	ItemNameText->SetText(Item ? Item->DisplayName : FText::GetEmpty());
	SetToolTipText(Item ? Item->DisplayName : FText::GetEmpty());
}

// 레시피의 제작 가능 여부와 선택 강조를 표시
// 큐에서는 같은 타일을 수량, 시간 표시로 재사용
void UCraftingItemWidget::SetRecipeState(bool bCraftable, bool bSelected)
{
	// 클릭 자체는 허용
	// 재료가 부족해도 상세 설명 확인 가능
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
	// 큐에 아이템 정보 설정
	ItemButton->SetIsEnabled(true);
	ItemButton->SetVisibility(ESlateVisibility::HitTestInvisible);
	ItemNameText->SetVisibility(ESlateVisibility::Collapsed);
	ItemIcon->SetRenderOpacity(1.f);

	CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
	CountText->SetText(FText::Format(FText::FromString(TEXT("×{0}")), FText::AsNumber(Count)));

	RemainingTimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	RemainingTimeText->SetText(
		bCompleted
		? FText::FromString(TEXT("회수 대기"))
		: FText::Format(FText::FromString(TEXT("{0}초")), FText::AsNumber(FMath::CeilToInt(Seconds)))
	);
}

void UCraftingItemWidget::HandleClicked()
{
	if (Item)
		OnItemClicked.Broadcast(Item);
}
