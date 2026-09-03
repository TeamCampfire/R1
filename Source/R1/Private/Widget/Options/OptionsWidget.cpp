// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Options/OptionsWidget.h"
#include "Widget/Options/OptionsControlsWidget.h"
#include "Widget/Options/OptionsDisplayWidget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/WidgetSwitcher.h"

namespace
{
	// 왼쪽 카테고리 목록에서 현재 선택된 항목의 배경색 — 선택 안 된 항목은 완전 투명.
	const FLinearColor GSelectedCategoryColor = FLinearColor(0.176f, 0.4f, 0.176f, 0.8f);
	const FLinearColor GUnselectedCategoryColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
}

void UOptionsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (CategoryButton_Controls)
	{
		CategoryButton_Controls->OnClicked.AddDynamic(this, &UOptionsWidget::HandleCategoryControlsClicked);
	}

	if (CategoryButton_Display)
	{
		CategoryButton_Display->OnClicked.AddDynamic(this, &UOptionsWidget::HandleCategoryDisplayClicked);
	}

	// 처음 열었을 때 기본으로 보여줄 카테고리.
	SwitchToCategory(EOptionsCategory::Controls);
}

void UOptionsWidget::HandleCategoryControlsClicked()
{
	SwitchToCategory(EOptionsCategory::Controls);
}

void UOptionsWidget::HandleCategoryDisplayClicked()
{
	SwitchToCategory(EOptionsCategory::Display);
}

void UOptionsWidget::SwitchToCategory(EOptionsCategory Category)
{
	// 컨텐츠에 표시할 번호 할당 
	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidgetIndex(static_cast<int32>(Category));
	}

	/// 각 버튼별 현재 선택된 카테고리 번호인지 확인 해서 배경색 표시
	if (CategoryHighlight_Controls)
	{
		CategoryHighlight_Controls->SetBrushColor(Category == EOptionsCategory::Controls ? GSelectedCategoryColor : GUnselectedCategoryColor);
	}

	if (CategoryHighlight_Display)
	{
		CategoryHighlight_Display->SetBrushColor(Category == EOptionsCategory::Display ? GSelectedCategoryColor : GUnselectedCategoryColor);
	}
}

void UOptionsWidget::RefreshActiveCategory()
{
	if (!ContentSwitcher)
	{
		return;
	}

	if (UOptionsControlsWidget* Controls = Cast<UOptionsControlsWidget>(ContentSwitcher->GetWidgetAtIndex(static_cast<int32>(EOptionsCategory::Controls))))
	{
		Controls->RefreshRows();
	}

	if (UOptionsDisplayWidget* Display = Cast<UOptionsDisplayWidget>(ContentSwitcher->GetWidgetAtIndex(static_cast<int32>(EOptionsCategory::Display))))
	{
		Display->RefreshFromCurrentSettings();
	}
}
