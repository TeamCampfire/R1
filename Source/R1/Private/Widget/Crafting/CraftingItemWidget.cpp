#include "Widget/Crafting/CraftingItemWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/Item/ItemDataBase.h"

void UCraftingItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//
	ItemButton->OnClicked.AddDynamic(this, &UCraftingItemWidget::HandleClicked);

	// 제작 취소 버튼 바인딩
	CancelButton->OnClicked.AddDynamic(this, &UCraftingItemWidget::HandleCancelClicked);
}

void UCraftingItemWidget::SetItem(UItemDataBase* InItem)
{
	if (Item == InItem)
		return;

	// 처음에는 취소 버튼 숨기기
	CancelButton->SetVisibility(ESlateVisibility::Collapsed);

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

void UCraftingItemWidget::SetQueueState(const FGuid& OrderId, int32 Count, float Seconds, bool bCompleted, bool bActive)
{
	BoundOrderId = OrderId;

	// 큐에 아이템 정보 설정
	ItemButton->SetIsEnabled(true);
	ItemButton->SetVisibility(ESlateVisibility::HitTestInvisible);
	ItemNameText->SetVisibility(ESlateVisibility::Collapsed);
	ItemIcon->SetRenderOpacity(1.f);

	CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
	CountText->SetText(FText::Format(FText::FromString(TEXT("×{0}")), FText::AsNumber(Count)));

	if (bCompleted)
	{
		RemainingTimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
		RemainingTimeText->SetText(FText::FromString(TEXT("회수 대기")));
	}
	else if (bActive)
	{
		RemainingTimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
		RemainingTimeText->SetText(FText::Format(FText::FromString(TEXT("{0}초")), FText::AsNumber(FMath::CeilToInt(Seconds))));
	}
	else
	{
		// 대기: 아직 제작 차례가 오지 않은 주문
		RemainingTimeText->SetVisibility(ESlateVisibility::Collapsed);	// 남은 시간 안 보이게
		RemainingTimeText->SetText(FText::GetEmpty());					// 안 보이더라도 텍스트 비워두기
	}

	// 진행 중이거나 대기 중인 주문에서만 취소 버튼 표시
	// 제작 진행/대기 타일 전체를 취소 버튼으로 사용
	CancelButton->SetVisibility(bCompleted ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UCraftingItemWidget::HandleClicked()
{
	if (Item)
		OnItemClicked.Broadcast(Item);
}


void UCraftingItemWidget::HandleCancelClicked()
{
	if (BoundOrderId.IsValid())
		OnCancelRequested.Broadcast(BoundOrderId);
}
