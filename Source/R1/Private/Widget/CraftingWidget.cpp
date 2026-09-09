#include "Widget/CraftingWidget.h"

#include "Character/ActionPlayerController.h"
#include "Component/CraftingComponent.h"
#include "Component/InventoryComponent.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Data/Item/ItemDataBase.h"
#include "Item/PlaceableItem/Workbench.h"
#include "Widget/Crafting/CraftingItemWidget.h"
#include "Widget/Crafting/CraftingMaterialWidget.h"
#include "InputCoreTypes.h"

void UCraftingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SearchBox->OnTextChanged.AddDynamic(this, &UCraftingWidget::HandleSearchChanged);
	DecreaseButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleDecrease);
	IncreaseButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleIncrease);
	MaximumButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleMaximum);
	CraftButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleCraft);
	CollectButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleCollect);
	CloseButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleClose);
}

// 개인 제작과 작업대 제작이 화면을 공유하되, 레시피 및 큐 조회 대상은 모드별로 선택
void UCraftingWidget::BindCrafting(UCraftingComponent* InCrafting, AWorkbench* InBench)
{
	Crafting = InCrafting;
	BoundBench = InBench;
	bWorkbenchMode = InBench != nullptr;
	Selected = nullptr;
	RefreshElapsed = 0.f;
	SelectedIcon->SetVisibility(ESlateVisibility::Hidden);
	SelectedNameText->SetText(FText::FromString("아이템을 선택하세요"));
	DescriptionText->SetText(FText::GetEmpty());
	DurationText->SetText(FText::GetEmpty());
	CraftQuantity = 0;
	SearchText.Reset();
	SearchBox->SetText(FText::GetEmpty());
	TitleText->SetText(bWorkbenchMode ? FText::FromString("작업대 제작") : FText::FromString("제작"));
	CollectButton->SetVisibility(bWorkbenchMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	RebuildRecipes();
	Refresh();
}

void UCraftingWidget::UnbindCrafting()
{
	Crafting = nullptr;
	BoundBench.Reset();
	bWorkbenchMode = false;
	Selected = nullptr;
	CraftQuantity = 0;
	RefreshElapsed = 0.f;
	SearchText.Reset();
	SearchBox->SetText(FText::GetEmpty());
	SelectedIcon->SetBrushFromTexture(nullptr);
	RecipeContainer->ClearChildren();
	MaterialContainer->ClearChildren();
	QueueContainer->ClearChildren();
	CompletedContainer->ClearChildren();
}

UCraftingComponent* UCraftingWidget::GetQueueSource() const
{
	return bWorkbenchMode ? (BoundBench.IsValid() ? BoundBench->GetCraftingComponent() : nullptr) : Crafting.Get();
}

int32 UCraftingWidget::GetMaximum() const
{
	if (!Crafting || (bWorkbenchMode && !BoundBench.IsValid()))
		return 0;

	return Crafting->GetMaximum(Selected, BoundBench.Get());
}

// 공백 제거, 소문자로 변환
FString UCraftingWidget::NormalizeSearchText(const FString& Text)
{
	FString Result;
	Result.Reserve(Text.Len());
	for (TCHAR Character : Text)
	{
		if (!FChar::IsWhitespace(Character)) Result.AppendChar(FChar::ToLower(Character));
	}
	return Result;
}

// 작업대 필요 조건과 공백, 대소문자를 정규화한 검색어로 레시피 타일을 필터링
void UCraftingWidget::RebuildRecipes()
{
	RecipeContainer->ClearChildren();

	// 레시피는 공통 카탈로그에서 읽고, 진행/완료 큐만 개인 또는 작업대에서 읽기
	if (!Crafting || !ItemWidgetClass)
		return;

	for (UItemDataBase* Item : Crafting->GetRecipes())
	{
		if (!Crafting->IsRecipeAvailable(Item, bWorkbenchMode)
			|| !NormalizeSearchText(Item->DisplayName.ToString()).Contains(SearchText, ESearchCase::CaseSensitive)) continue;
		UCraftingItemWidget* Tile = CreateWidget<UCraftingItemWidget>(GetOwningPlayer(), ItemWidgetClass);
		if (!Tile) continue;
		Tile->SetItem(Item);
		Tile->OnItemClicked.AddDynamic(this, &UCraftingWidget::HandleRecipeClicked);
		RecipeContainer->AddChildToWrapBox(Tile);
	}
}

void UCraftingWidget::Refresh()
{
	if (!Crafting) return;
	const int32 Maximum = GetMaximum();
	CraftQuantity = Maximum > 0 ? FMath::Clamp(CraftQuantity, 1, Maximum) : 0;
	QuantityText->SetText(FText::AsNumber(CraftQuantity));
	DecreaseButton->SetIsEnabled(CraftQuantity > 1);
	IncreaseButton->SetIsEnabled(CraftQuantity < Maximum);
	MaximumButton->SetIsEnabled(Maximum > 0 && CraftQuantity != Maximum);
	const UCraftingComponent* Source = GetQueueSource();
	CraftButton->SetIsEnabled(Source && Source->HasQueueSpace() && Maximum > 0);
	CollectButton->SetIsEnabled(Source && !Source->GetCompletedOrders().IsEmpty());

	StatusText->SetText(
		!Selected
		? FText::FromString("아이템을 선택하세요")
		: Maximum == 0
			? FText::FromString("재료가 부족하거나 제작 조건을 충족하지 못했습니다.")
			: Source && !Source->HasQueueSpace()
				? FText::FromString("제작 큐가 가득 찼습니다. 완료된 아이템을 회수하세요.")
				: FText::GetEmpty()
	);

	for (UWidget* Child : RecipeContainer->GetAllChildren())
	{
		UCraftingItemWidget* Tile = Cast<UCraftingItemWidget>(Child);
		if (Tile) Tile->SetRecipeState(Crafting->GetMaximum(Tile->GetItem(), BoundBench.Get()) > 0, Tile->GetItem() == Selected);
	}

	TMap<UItemDataBase*, int32> Costs;
	Crafting->CollectIngredientCosts(Selected, Costs);

	// 선택 변경 때만 행 개수를 바꾸고, 인벤토리/수량 변경은 기존 텍스트만 갱신한다.
	while (MaterialContainer->GetChildrenCount() > Costs.Num())
	{
		MaterialContainer->RemoveChildAt(MaterialContainer->GetChildrenCount() - 1);
	}

	while (MaterialWidgetClass && MaterialContainer->GetChildrenCount() < Costs.Num())
	{
		UCraftingMaterialWidget* Row = CreateWidget<UCraftingMaterialWidget>(GetOwningPlayer(), MaterialWidgetClass);
		if (!Row) break;
		MaterialContainer->AddChildToVerticalBox(Row);
	}

	const UInventoryComponent* Inventory = Crafting->Inventory();
	int32 Index = 0;
	for (const auto& Cost : Costs)
	{
		UCraftingMaterialWidget* Row = Cast<UCraftingMaterialWidget>(MaterialContainer->GetChildAt(Index++));
		if (Row) Row->SetMaterial(Cost.Key, Cost.Value, CraftQuantity, Inventory ? Inventory->GetItemCount(Cost.Key) : 0);
	}

	RefreshQueue();
}

// 진행 주문과 회수 대기 주문을 별도로 표시하고 서버 시간과 앞선 주문을 기준으로 남은 시간을 계산
void UCraftingWidget::RefreshQueue()
{
	const UCraftingComponent* Source = GetQueueSource();
	if (!Source || !ItemWidgetClass)
		return;

	// 진행/완료 패널 모두 기존 타일을 재사용
	// 매 프레임 위젯 트리를 다시 만들지 않음
	for (int32 PanelIndex = 0; PanelIndex < 2; ++PanelIndex)
	{
		const bool bCompleted = PanelIndex == 1;
		UWrapBox* Panel = bCompleted ? CompletedContainer.Get() : QueueContainer.Get();
		const TArray<FCraftingOrder>& Orders = bCompleted ? Source->GetCompletedOrders() : Source->GetQueue();

		while (Panel->GetChildrenCount() > Orders.Num())
			Panel->RemoveChildAt(Panel->GetChildrenCount() - 1);

		while (Panel->GetChildrenCount() < Orders.Num())
		{
			UCraftingItemWidget* Tile = CreateWidget<UCraftingItemWidget>(GetOwningPlayer(), ItemWidgetClass);
			if (!Tile) break;
			Panel->AddChildToWrapBox(Tile);
		}

		float PrecedingSeconds = 0.f;
		for (int32 Index = 0; Index < Orders.Num(); ++Index)
		{
			const FCraftingOrder& Order = Orders[Index];
			if (!Order.Item)
				continue;

			const float Duration = FMath::Max(0.1f, Order.Item->CraftingSeconds);
			const float Seconds = Order.FinishTime > 0.f ? FMath::Max(0.f, Order.FinishTime - Source->GetServerTime()) : PrecedingSeconds + Duration;
			PrecedingSeconds = Seconds + (Order.Remaining - 1) * Duration;
			UCraftingItemWidget* Tile = Cast<UCraftingItemWidget>(Panel->GetChildAt(Index));
			if (Tile)
			{
				Tile->SetItem(Order.Item);
				Tile->SetQueueState(Order.Remaining, Seconds, bCompleted);
			}
		}
	}
}

// 작업대 거리 이탈 시 화면을 닫고, 유지 중에는 0.2초 간격으로 재료와 큐 표시를 갱신
void UCraftingWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	if (!Crafting) return;
	if (bWorkbenchMode && (!BoundBench.IsValid()
		|| !IInteractableInterface::Execute_CanInteract(BoundBench.Get(), GetOwningPlayerPawn())))
	{
		HandleClose();
		return;
	}
	RefreshElapsed += DeltaTime;
	if (RefreshElapsed >= 0.2f)
	{
		RefreshElapsed = 0.f;
		Refresh();
	}
}

FReply UCraftingWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	// 검색 입력 중 Q는 문자로 입력
	// Escape는 포커스 위치와 무관하게 닫기.
	if (KeyEvent.GetKey() == EKeys::Escape || (KeyEvent.GetKey() == EKeys::Q && !SearchBox->HasKeyboardFocus() && !SearchBox->HasFocusedDescendants()))
	{
		HandleClose();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

void UCraftingWidget::HandleSearchChanged(const FText& Text)
{
	SearchText = NormalizeSearchText(Text.ToString());
	RebuildRecipes();
	Refresh();
}

void UCraftingWidget::HandleRecipeClicked(UItemDataBase* Item)
{
	Selected = Item;
	SelectedIcon->SetVisibility(Item ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	CraftQuantity = 1;
	SelectedIcon->SetBrushFromTexture(Item ? Item->Icon.LoadSynchronous() : nullptr);
	SelectedNameText->SetText(Item ? Item->DisplayName : FText::GetEmpty());
	DescriptionText->SetText(Item ? Item->Description : FText::GetEmpty());
	DurationText->SetText(Item ? FText::Format(NSLOCTEXT("Crafting", "Duration", "개당 {0}초"), FText::AsNumber(Item->CraftingSeconds)) : FText::GetEmpty());
	Refresh();
}

void UCraftingWidget::HandleDecrease()
{
	CraftQuantity = FMath::Max(0, CraftQuantity - 1);
	Refresh();
}

void UCraftingWidget::HandleIncrease()
{
	if (CraftQuantity < GetMaximum()) ++CraftQuantity;
	Refresh();
}

void UCraftingWidget::HandleMaximum()
{
	CraftQuantity = GetMaximum();
	Refresh();
}

void UCraftingWidget::HandleCraft()
{
	if (Crafting && CraftQuantity > 0 && GetQueueSource())
	{
		Crafting->Server_Enqueue(Selected, FMath::Min(CraftQuantity, GetMaximum()), BoundBench.Get());
	}
}

void UCraftingWidget::HandleCollect()
{
	if (Crafting && BoundBench.IsValid()) Crafting->Server_CollectCompleted(BoundBench.Get());
}

void UCraftingWidget::HandleClose()
{
	if (AActionPlayerController* Controller = Cast<AActionPlayerController>(GetOwningPlayer()))
		Controller->CloseCrafting();
}
