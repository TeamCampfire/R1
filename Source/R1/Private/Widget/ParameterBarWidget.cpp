// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/ParameterBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UParameterBarWidget::UpdateParameterBar(float inCurrent, float inMax)
{
	float Div = FMath::Max(inMax, 0.001f);
	Bar->SetPercent(inCurrent / Div);
	CurrentText->SetText(FText::AsNumber(FMath::FloorToInt(inCurrent)));
}

void UParameterBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Bar)
	{
		Bar->SetFillColorAndOpacity(FillColor);
	}
}

#if WITH_EDITOR

void UParameterBarWidget::PostEditChangeProperty(
	FPropertyChangedEvent& InPropertyChangedEvent)
{
	Super::PostEditChangeProperty(InPropertyChangedEvent);

	if (InPropertyChangedEvent.Property &&
		InPropertyChangedEvent.Property->GetFName() ==
		GET_MEMBER_NAME_CHECKED(UParameterBarWidget, FillColor))
	{
		if (Bar)
		{
			Bar->SetFillColorAndOpacity(FillColor);
		}
	}
}

#endif