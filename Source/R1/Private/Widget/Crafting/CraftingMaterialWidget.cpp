#include "Widget/Crafting/CraftingMaterialWidget.h"
#include "Components/TextBlock.h"
#include "Data/Item/ItemDataBase.h"

void UCraftingMaterialWidget::SetMaterial(UItemDataBase* Item, int32 PerItem, int32 Quantity, int32 Owned)
{
	const int64 Required = int64(PerItem) * FMath::Max(1, Quantity);
	PerItemText->SetText(FText::AsNumber(PerItem));
	MaterialNameText->SetText(Item ? Item->DisplayName : FText::GetEmpty());
	RequiredText->SetText(FText::AsNumber(Required));
	OwnedText->SetText(FText::AsNumber(Owned));
	const FSlateColor Color(Owned >= Required ? FLinearColor(0.85f, 0.84f, 0.8f) : FLinearColor(0.95f, 0.63f, 0.2f));
	RequiredText->SetColorAndOpacity(Color);
	OwnedText->SetColorAndOpacity(Color);
}
