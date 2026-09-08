#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingMaterialWidget.generated.h"

class UTextBlock;
class UItemDataBase;

UCLASS()
class R1_API UCraftingMaterialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMaterial(UItemDataBase* Item, int32 PerItem, int32 Quantity, int32 Owned);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PerItemText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> MaterialNameText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> RequiredText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> OwnedText;
};
