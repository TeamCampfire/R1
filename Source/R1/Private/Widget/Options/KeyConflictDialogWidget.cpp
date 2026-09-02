// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Options/KeyConflictDialogWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UKeyConflictDialogWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ReplaceButton)
	{
		ReplaceButton->OnClicked.AddDynamic(this, &UKeyConflictDialogWidget::HandleReplaceClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &UKeyConflictDialogWidget::HandleCancelClicked);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UKeyConflictDialogWidget::ShowConflict(const FText& InConflictKeyName, const FText& InOldActionName, const FText& InNewActionName)
{
	if (ConflictKeyText)
	{
		ConflictKeyText->SetText(InConflictKeyName);
	}

	if (OldActionNameText)
	{
		OldActionNameText->SetText(InOldActionName);
	}

	if (NewActionNameText)
	{
		NewActionNameText->SetText(InNewActionName);
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UKeyConflictDialogWidget::CloseDialog()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UKeyConflictDialogWidget::HandleReplaceClicked()
{
	OnReplaceConfirmed.Broadcast();
}

void UKeyConflictDialogWidget::HandleCancelClicked()
{
	OnCancelled.Broadcast();
}
