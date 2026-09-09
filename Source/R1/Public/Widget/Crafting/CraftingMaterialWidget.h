#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingMaterialWidget.generated.h"

class UTextBlock;
class UItemDataBase;

// 제작 재료의 개당 비용, 총 필요량과 보유량을 표시하는 독립 행 위젯
UCLASS()
class R1_API UCraftingMaterialWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetMaterial(UItemDataBase* Item, int32 PerItem, int32 Quantity, int32 Owned);

protected:

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PerItemText;			// 개당 필요량

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> MaterialNameText;	// 재료 이름

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> RequiredText;		// 총 필요량

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> OwnedText;			// 현재 보유량
};
