#include "Widget/CraftingWidget.h"

#include "Character/ActionPlayerController.h"
#include "Component/CraftingComponent.h"
#include "Component/InventoryComponent.h"
#include "Data/Item/ItemDataBase.h"
#include "Item/PlaceableItem/Workbench.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace CraftingUI
{
	constexpr float RefreshInterval = 0.25f;

	// 작은 레시피 목록은 선형 검색으로 충분하다. 검색어와 이름에 같은 정규화를 적용한다.
	FString NormalizeSearchText(FString Text)
	{
		Text.ToLowerInline();
		Text.ReplaceInline(TEXT(" "), TEXT(""));
		return Text;
	}

	TSharedRef<SWidget> MakeButton(const TCHAR* Label, TFunction<FReply()> Action)
	{
		return SNew(SButton).OnClicked_Lambda(MoveTemp(Action))
		[
			SNew(STextBlock).Text(FText::FromString(Label))
		];
	}

	TSharedRef<SWidget> MakeItemTile(UItemDataBase* Item, const FText& Caption)
	{
		TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
		if (UTexture2D* Texture = Item->Icon.LoadSynchronous())
		{
			// 이미지 속성의 람다가 브러시를 소유하여 Slate가 그리는 동안 수명을 보장한다.
			TSharedRef<FSlateDynamicImageBrush> Brush = MakeShared<FSlateDynamicImageBrush>(Texture, FVector2D(64, 64), NAME_None);
			Content->AddSlot().AutoHeight()
			[
				SNew(SImage).Image_Lambda([Brush]() -> const FSlateBrush* { return &Brush.Get(); })
			];
		}
		Content->AddSlot().AutoHeight()[SNew(STextBlock).Text(Caption).AutoWrapText(true)];
		return SNew(SBox).WidthOverride(110).MinDesiredHeight(90)[Content];
	}
}

int32 UCraftingWidget::GetMaxCraftableQuantity() const
{
	return Crafting ? Crafting->GetMaximum(Selected, Bench) : 0;
}

TSharedRef<SWidget> UCraftingWidget::RebuildWidget()
{
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	Root->AddSlot().AutoHeight()[BuildHeader()];
	Root->AddSlot().FillHeight(1.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(10)[BuildRecipePanel()]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(10)[BuildDetailsPanel()]
	];
	Refresh();
	return SNew(SBorder).Padding(40).BorderBackgroundColor(FLinearColor(0.08f, 0.09f, 0.08f, 0.97f))[Root];
}

TSharedRef<SWidget> UCraftingWidget::BuildHeader()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock).Text_Lambda([this]
			{
				return FText::FromString(IsValid(Bench) ? TEXT("작업대 제작") : TEXT("제작"));
			})
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			CraftingUI::MakeButton(TEXT("닫기 (Q)"), [this] { return HandleCloseClicked(); })
		];
}

TSharedRef<SWidget> UCraftingWidget::BuildRecipePanel()
{
	TSharedRef<SVerticalBox> Panel = SNew(SVerticalBox);
	Panel->AddSlot().FillHeight(1.f)
	[
		SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(RecipeContainer, SWrapBox).UseAllottedSize(true)]
	];
	Panel->AddSlot().AutoHeight()
	[
		SNew(SSearchBox).HintText(FText::FromString(TEXT("아이템 이름 검색")))
		.OnTextChanged_UObject(this, &UCraftingWidget::HandleSearchChanged)
	];
	Panel->AddSlot().AutoHeight()
	[
		SNew(SCheckBox).OnCheckStateChanged_UObject(this, &UCraftingWidget::HandleCraftableFilterChanged)[SNew(STextBlock).Text(FText::FromString(TEXT("보유 재료로 제작 가능한 아이템만")))]
	];
	Panel->AddSlot().AutoHeight().Padding(0, 16)
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("CRAFTING QUEUE · 클릭하여 취소")))
	];
	Panel->AddSlot().MaxHeight(180)
	[
		SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(QueueContainer, SWrapBox).UseAllottedSize(true)]
	];
	return Panel;
}

TSharedRef<SWidget> UCraftingWidget::BuildDetailsPanel()
{
	TSharedRef<SVerticalBox> Panel = SNew(SVerticalBox);
	Panel->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Text_Lambda([this]
		{
			return Selected ? Selected->DisplayName : FText::FromString(TEXT("아이템을 선택하세요"));
		})
	];
	Panel->AddSlot().FillHeight(1.f).Padding(0, 20)
	[
		SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]
		{
			return Selected ? Selected->Description : FText::GetEmpty();
		})
	];
	Panel->AddSlot().AutoHeight()[SAssignNew(MaterialContainer, SVerticalBox)];
	Panel->AddSlot().AutoHeight().Padding(0, 16)[BuildQuantityControls()];
	return Panel;
}

TSharedRef<SWidget> UCraftingWidget::BuildQuantityControls()
{
	TSharedRef<SHorizontalBox> Controls = SNew(SHorizontalBox);
	Controls->AddSlot().AutoWidth()[CraftingUI::MakeButton(TEXT("−"), [this] { return HandleDecreaseClicked(); })];
	Controls->AddSlot().AutoWidth().Padding(16, 0)
	[
		SNew(STextBlock).Text_Lambda([this] { return FText::AsNumber(FMath::Min(CraftQuantity, GetMaxCraftableQuantity())); })
	];
	Controls->AddSlot().AutoWidth()[CraftingUI::MakeButton(TEXT("+"), [this] { return HandleIncreaseClicked(); })];
	Controls->AddSlot().AutoWidth()[CraftingUI::MakeButton(TEXT("최대"), [this] { return HandleMaximumClicked(); })];
	Controls->AddSlot().Padding(20, 0)
	[
		SNew(SButton)
		.IsEnabled_Lambda([this] { return Crafting && GetMaxCraftableQuantity() > 0 && Crafting->Queue.Num() < UCraftingComponent::MaxQueueSize; })
		.OnClicked_UObject(this, &UCraftingWidget::HandleCraftClicked)[SNew(STextBlock).Text(FText::FromString(TEXT("제작하기")))]
	];
	return Controls;
}

void UCraftingWidget::Refresh()
{
	if (!RecipeContainer || !MaterialContainer || !QueueContainer || !Crafting)
	{
		return;
	}
	CraftQuantity = FMath::Clamp(CraftQuantity, 1, FMath::Max(1, GetMaxCraftableQuantity()));
	RefreshRecipes();
	RefreshMaterials();
	RefreshQueue();
}

void UCraftingWidget::RefreshRecipes()
{
	RecipeContainer->ClearChildren();
	for (const TObjectPtr<UItemDataBase>& Recipe : Crafting->Recipes)
	{
		UItemDataBase* Item = Recipe.Get();
		if (!Item || (Item->bRequiresWorkbench && !IsValid(Bench))
			|| !CraftingUI::NormalizeSearchText(Item->DisplayName.ToString()).Contains(NormalizedSearchText))
		{
			continue;
		}
		const bool bAvailable = Crafting->GetMaximum(Item, Bench) > 0;
		if (bOnlyCraftable && !bAvailable)
		{
			continue;
		}
		// 재료가 부족해도 설명은 읽을 수 있도록 클릭은 허용하고 아이콘만 흐리게 표시한다.
		RecipeContainer->AddSlot().Padding(4)
		[
			SNew(SButton).ContentPadding(8).OnClicked_UObject(this, &UCraftingWidget::HandleRecipeClicked, Item)
			[
				SNew(SBorder).RenderOpacity(bAvailable ? 1.f : 0.35f)[CraftingUI::MakeItemTile(Item, Item->DisplayName)]
			]
		];
	}
}

void UCraftingWidget::RefreshMaterials()
{
	MaterialContainer->ClearChildren();
	if (!Selected)
	{
		return;
	}
	MaterialContainer->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Text(FText::FromString(FString::Printf(
			TEXT("제작 시간: %.1f초 / 개\n수량     필요한 아이템                   합계     보유"), Selected->CraftingSeconds)))
	];
	const UInventoryComponent* PlayerInventory = Crafting->Inventory();
	for (const FCraftIngredient& Cost : Selected->CraftingCost)
	{
		UItemDataBase* Material = Cost.Item.LoadSynchronous();
		const int32 Owned = PlayerInventory ? PlayerInventory->GetItemCount(Material) : 0;
		// 제작 불가능한 아이템도 1개 기준 필요 재료를 보여준다.
		const int64 TotalRequired = int64(Cost.Amount) * CraftQuantity;
		MaterialContainer->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d     %s                   %lld     %d"),
				Cost.Amount, Material ? *Material->DisplayName.ToString() : TEXT("누락"), TotalRequired, Owned)))
		];
	}
}

void UCraftingWidget::RefreshQueue()
{
	QueueContainer->ClearChildren();
	float PrecedingSeconds = 0.f;
	for (const FCraftingOrder& Order : Crafting->Queue)
	{
		if (!Order.Item)
		{
			continue;
		}
		const float Duration = FMath::Max(0.1f, Order.Item->CraftingSeconds);
		// 선두 주문만 서버 완료 시각이 있다. 이후 주문은 앞선 수량의 소요 시간을 누적한다.
		const float SecondsLeft = Order.FinishTime > 0.f
			? FMath::Max(0.f, Order.FinishTime - Crafting->GetServerTime())
			: PrecedingSeconds + Duration;
		PrecedingSeconds = SecondsLeft + (Order.Remaining - 1) * Duration;
		const FText Caption = FText::FromString(FString::Printf(TEXT("%s ×%d\n%.0f초"),
			*Order.Item->DisplayName.ToString(), Order.Remaining, SecondsLeft));
		QueueContainer->AddSlot().Padding(3)
		[
			SNew(SButton).OnClicked_UObject(this, &UCraftingWidget::HandleCancelClicked, Order.Id)[CraftingUI::MakeItemTile(Order.Item, Caption)]
		];
	}
}

void UCraftingWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	RefreshElapsed += DeltaTime;
	if (RefreshElapsed >= CraftingUI::RefreshInterval)
	{
		RefreshElapsed = 0.f;
		Refresh();
	}
}

void UCraftingWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	RecipeContainer.Reset();
	MaterialContainer.Reset();
	QueueContainer.Reset();
}

// 입력 처리: UI 배치와 분리하여 수량 변경 및 서버 요청 경로를 한곳에서 읽을 수 있게 한다.

FReply UCraftingWidget::HandleCloseClicked()
{
	if (AActionPlayerController* Controller = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		Controller->CloseCrafting();
	}
	return FReply::Handled();
}

void UCraftingWidget::HandleSearchChanged(const FText& Text)
{
	NormalizedSearchText = CraftingUI::NormalizeSearchText(Text.ToString());
	Refresh();
}

void UCraftingWidget::HandleCraftableFilterChanged(ECheckBoxState State)
{
	bOnlyCraftable = State == ECheckBoxState::Checked;
	Refresh();
}

FReply UCraftingWidget::HandleDecreaseClicked()
{
	CraftQuantity = FMath::Max(1, CraftQuantity - 1);
	Refresh();
	return FReply::Handled();
}

FReply UCraftingWidget::HandleIncreaseClicked()
{
	if (CraftQuantity < GetMaxCraftableQuantity())
	{
		++CraftQuantity;
	}
	Refresh();
	return FReply::Handled();
}

FReply UCraftingWidget::HandleMaximumClicked()
{
	CraftQuantity = FMath::Max(1, GetMaxCraftableQuantity());
	Refresh();
	return FReply::Handled();
}

FReply UCraftingWidget::HandleCraftClicked()
{
	if (Crafting)
	{
		Crafting->Server_Enqueue(Selected, FMath::Min(CraftQuantity, GetMaxCraftableQuantity()), Bench);
	}
	return FReply::Handled();
}

FReply UCraftingWidget::HandleRecipeClicked(UItemDataBase* Item)
{
	Selected = Item;
	CraftQuantity = 1;
	Refresh();
	return FReply::Handled();
}

FReply UCraftingWidget::HandleCancelClicked(FGuid OrderId)
{
	Crafting->Server_Cancel(OrderId);
	return FReply::Handled();
}
