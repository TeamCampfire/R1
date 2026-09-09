#include "Widget/Crafting/CraftingMaterialWidget.h"
#include "Components/TextBlock.h"
#include "Data/Item/ItemDataBase.h"

// 개당 비용과 선택 수량으로 필요량을 계산하고 보유량이 부족한 재료를 색으로 구분
void UCraftingMaterialWidget::SetMaterial(UItemDataBase* Item, int32 PerItem, int32 Quantity, int32 Owned)
{
	const int64 Required = int64(PerItem) * FMath::Max(1, Quantity);	// 필요한 개수

	PerItemText->SetText(FText::AsNumber(PerItem));

	MaterialNameText->SetText(Item ? Item->DisplayName : FText::GetEmpty());

	RequiredText->SetText(FText::AsNumber(Required));

	OwnedText->SetText(FText::AsNumber(Owned));

	const FSlateColor Color(Owned >= Required ? FLinearColor(0.85f, 0.84f, 0.8f) : FLinearColor(0.95f, 0.63f, 0.2f));
	RequiredText->SetColorAndOpacity(Color);
	OwnedText->SetColorAndOpacity(Color);
}
