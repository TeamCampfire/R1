// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/ParameterBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UParameterBarWidget::UpdateParameterBar(float inCurrent, float inMax)
{
	float Div = FMath::Max(inMax, 0.001f);
	TargetPercent = inCurrent / Div;
	CurrentText->SetText(FText::AsNumber(FMath::FloorToInt(inCurrent)));

	if (GetWorld()->GetTimerManager().IsTimerActive(ParameterBarTimerHandle)) return;

	float DeltaTime = GetWorld()->GetDeltaSeconds();
	GetWorld()->GetTimerManager().SetTimer(
		ParameterBarTimerHandle,
		FTimerDelegate::CreateLambda([this, DeltaTime]()
			{
				CurrentPercent = FMath::FInterpTo(
					CurrentPercent,
					TargetPercent,
					DeltaTime,
					InterpSpeed
			);
				Bar->SetPercent(CurrentPercent);

				if (FMath::IsNearlyEqual(CurrentPercent, TargetPercent, 0.001f))
				{
					GetWorld()->GetTimerManager().ClearTimer(ParameterBarTimerHandle);
				}
			}),
		DeltaTime,
		true
	);
}

void UParameterBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Bar)
	{
		Bar->SetFillColorAndOpacity(FillColor);
	}
}

void UParameterBarWidget::SetTargetPercent(float inPercent)
{
	TargetPercent = inPercent;
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