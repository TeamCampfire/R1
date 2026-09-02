// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Options/OptionsWidget.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

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
	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidgetIndex(static_cast<int32>(Category));
	}
}
