#include "Widget/Crafting/CraftingWidget.h"

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

	// HUD에 배치된 위젯을 재사용하므로 버튼과 검색 이벤트는 초기화 시 한 번만 연결
	SearchBox->OnTextChanged.AddDynamic(this, &UCraftingWidget::HandleSearchChanged);
	DecreaseButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleDecrease);
	IncreaseButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleIncrease);
	MaximumButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleMaximum);
	CraftButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleCraft);
	CollectButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleCollect);
	CloseButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleClose);
}

// 개인 제작과 작업대 제작이 화면을 공유하되, 큐 조회 대상과 회수 기능은 모드별로 선택
// 패널을 다시 열 때 이전 검색어, 선택 항목과 제작 수량이 남지 않도록 화면 상태 초기화
void UCraftingWidget::BindCrafting(UCraftingComponent* InCrafting, AWorkbench* InBench)
{
	Crafting = InCrafting;
	BoundBench = InBench;

	bWorkbenchMode = InBench != nullptr;

	RefreshElapsed = 0.f;

	Selected = nullptr;
	SelectedIcon->SetVisibility(ESlateVisibility::Hidden);
	SelectedNameText->SetText(FText::FromString(TEXT("아이템을 선택하세요")));
	DescriptionText->SetText(FText::GetEmpty());

	DurationText->SetText(FText::GetEmpty());
	CraftQuantity = 0;

	SearchText.Reset();
	SearchBox->SetText(FText::GetEmpty());

	TitleText->SetText(bWorkbenchMode ? FText::FromString(TEXT("작업대 제작")) : FText::FromString(TEXT("제작")));

	CollectButton->SetVisibility(bWorkbenchMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	RebuildRecipes();

	Refresh();
}

// 바인딩된 게임 객체의 수명과 HUD 위젯의 수명 분리를 위해
// 화면을 닫을 때 참조와 타일을 정리
void UCraftingWidget::UnbindCrafting()
{
	Crafting = nullptr;
	BoundBench.Reset();

	bWorkbenchMode = false;

	Selected = nullptr;
	SelectedIcon->SetBrushFromTexture(nullptr);

	RefreshElapsed = 0.f;

	CraftQuantity = 0;

	SearchText.Reset();
	SearchBox->SetText(FText::GetEmpty());

	RecipeContainer->ClearChildren();
	MaterialContainer->ClearChildren();
	QueueContainer->ClearChildren();
	CompletedContainer->ClearChildren();
}

UCraftingComponent* UCraftingWidget::GetQueueSource() const
{
	return bWorkbenchMode
		? (
			BoundBench.IsValid()
			? BoundBench->GetCraftingComponent()
			: nullptr
			)
		: Crafting.Get();
}

int32 UCraftingWidget::GetMaximum() const
{
	if (!Crafting || (bWorkbenchMode && !BoundBench.IsValid()))
		return 0;

	return Crafting->GetMaximum(Selected, BoundBench.Get());
}

// 검색어와 아이템 이름에서 공백 제거, 소문자 변환 작업을 통해 같은 기준으로 비교할 수 있게 함
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

// 개인 제작에서는 작업대 필수 레시피를 제외하고, 작업대 제작에서는 전체 레시피를 표시
// 두 모드 모두 정규화한 아이템 이름에 검색어가 포함된 레시피만 타일로 만듦
void UCraftingWidget::RebuildRecipes()
{
	RecipeContainer->ClearChildren();

	// 레시피는 공통 카탈로그에서 읽고, 진행/완료 큐만 개인 또는 작업대에서 읽기
	if (!Crafting || !ItemWidgetClass)
		return;

	for (UItemDataBase* Item : Crafting->GetRecipes())
	{
		if (!Crafting->IsRecipeAvailable(Item, bWorkbenchMode)
			|| !NormalizeSearchText(Item->DisplayName.ToString()).Contains(SearchText, ESearchCase::CaseSensitive))
			continue;

		UCraftingItemWidget* Tile = CreateWidget<UCraftingItemWidget>(GetOwningPlayer(), ItemWidgetClass);
		if (!Tile)
			continue;

		// 클릭한 타일의 아이템 데이터를 상세 영역에 표시하도록 이벤트 연결
		Tile->SetItem(Item);
		Tile->OnItemClicked.AddDynamic(this, &UCraftingWidget::HandleRecipeClicked);

		// 필터를 통과한 레시피 타일을 목록에 추가
		RecipeContainer->AddChildToWrapBox(Tile);
	}
}

// 선택 레시피와 현재 인벤토리 상태를 기준으로
// 수량, 버튼, 재료 목록과 큐를 갱신
void UCraftingWidget::Refresh()
{
	if (!Crafting)
		return;

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
		? FText::FromString(TEXT("아이템을 선택하세요"))
		: Maximum == 0
			? FText::FromString(TEXT("재료가 부족하거나 제작 조건을 충족하지 못했습니다."))
			: Source && !Source->HasQueueSpace()
			? FText::FromString(TEXT("제작 큐가 가득 찼습니다. 완료된 아이템을 회수하세요."))
				: FText::GetEmpty()
	);

	// 각 레시피 타일의 제작 가능 여부와 현재 선택 상태를 갱신
	for (UWidget* Child : RecipeContainer->GetAllChildren())
	{
		UCraftingItemWidget* Tile = Cast<UCraftingItemWidget>(Child);
		if (Tile)
			Tile->SetRecipeState(Crafting->GetMaximum(Tile->GetItem(), BoundBench.Get()) > 0, Tile->GetItem() == Selected);
	}

	TMap<UItemDataBase*, int32> Costs;
	Crafting->CollectIngredientCosts(Selected, Costs); // 같은 재료가 여러 번 등록돼 있으면 합산된 비용으로 책정

	// 필요한 행 수만 맞춘 뒤, 기존 재료 위젯을 재사용해 보유량과 요구량을 갱신
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

// 진행 주문과 회수 대기 주문을 별도로 표시
// 서버 시간과 앞선 주문을 기준으로 남은 시간을 계산
void UCraftingWidget::RefreshQueue()
{
	const UCraftingComponent* Source = GetQueueSource();
	if (!Source || !ItemWidgetClass)
		return;

	// 진행/완료 패널 모두 기존 타일을 재사용해 주기적인 갱신 때 불필요한 생성 감소
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

			// 선두 주문: 복제된 완료 시각을 사용
			// 대기 주문: (앞선 주문들의 남은 시간 + 자신의 제작 시간)으로 화면에 표시할 예상 대기 시간을 계산
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

	if (!Crafting)
		return;

	if (bWorkbenchMode && (!BoundBench.IsValid()
		|| !IInteractableInterface::Execute_CanInteract(BoundBench.Get(), GetOwningPlayerPawn())))
	{
		HandleClose();
		return;
	}

	// 0.2초 간격으로 재료와 큐 표시 갱신
	RefreshElapsed += DeltaTime;
	if (RefreshElapsed >= 0.2f)
	{
		RefreshElapsed = 0.f;
		Refresh();
	}
}

FReply UCraftingWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	// 검색창에 포커스가 있으면 Q를 검색 문자로 사용
	// Escape는 포커스와 무관하게 닫음
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
	SelectedIcon->SetBrushFromTexture(Item ? Item->Icon.LoadSynchronous() : nullptr);
	SelectedNameText->SetText(Item ? Item->DisplayName : FText::GetEmpty());

	CraftQuantity = 1;

	DescriptionText->SetText(Item ? Item->Description : FText::GetEmpty());

	DurationText->SetText(Item ? FText::Format(FText::FromString(TEXT("개당 {0}초")), FText::AsNumber(Item->CraftingSeconds)) : FText::GetEmpty());

	Refresh();
}

void UCraftingWidget::HandleDecrease()
{
	CraftQuantity = FMath::Max(0, CraftQuantity - 1);

	Refresh();
}

void UCraftingWidget::HandleIncrease()
{
	if (CraftQuantity < GetMaximum())
		++CraftQuantity;

	Refresh();
}

void UCraftingWidget::HandleMaximum()
{
	CraftQuantity = GetMaximum();

	Refresh();
}

void UCraftingWidget::HandleCraft()
{
	// 클라이언트에서 계산한 수량은 표시용이며,
	// 실제 레시피·재료·거리 검증은 서버 RPC에서 다시 수행
	if (Crafting && CraftQuantity > 0 && GetQueueSource())
	{
		Crafting->Server_Enqueue(Selected, FMath::Min(CraftQuantity, GetMaximum()), BoundBench.Get());
	}
}

void UCraftingWidget::HandleCollect()
{
	if (Crafting && BoundBench.IsValid())
		Crafting->Server_CollectCompleted(BoundBench.Get());
}

void UCraftingWidget::HandleClose()
{
	if (AActionPlayerController* Controller = Cast<AActionPlayerController>(GetOwningPlayer()))
		Controller->CloseCrafting();
}
