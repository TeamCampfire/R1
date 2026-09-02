// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Options/OptionsControlsWidget.h"
#include "Widget/Options/KeyRebindRowWidget.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Engine/LocalPlayer.h"

void UOptionsControlsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ResetButton)
	{
		ResetButton->OnClicked.AddDynamic(this, &UOptionsControlsWidget::HandleResetClicked);
	}

	RefreshRows();
}

void UOptionsControlsWidget::RefreshRows()
{
	if (!KeyRowContainer || !KeyRebindRowClass)
	{
		return;
	}

	KeyRowContainer->ClearChildren();

	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	const UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
	if (!Profile)
	{
		return;
	}

	for (const TPair<FName, FKeyMappingRow>& RowPair : Profile->GetPlayerMappingRows())
	{
		for (const FPlayerKeyMapping& Mapping : RowPair.Value.Mappings)
		{
			UKeyRebindRowWidget* Row = WidgetTree->ConstructWidget<UKeyRebindRowWidget>(KeyRebindRowClass);
			if (!Row)
			{
				continue;
			}

			Row->Setup(Mapping.GetMappingName(), Mapping.GetSlot());
			KeyRowContainer->AddChild(Row);
		}
	}
}

void UOptionsControlsWidget::HandleResetClicked()
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings)
	{
		return;
	}

	if (UEnhancedPlayerMappableKeyProfile* Profile = UserSettings->GetActiveKeyProfile())
	{
		Profile->ResetToDefault();
		UserSettings->SaveSettings();
	}

	RefreshRows();
}

UEnhancedInputUserSettings* UOptionsControlsWidget::GetUserSettings() const
{
	if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer()))
	{
		return SubSystem->GetUserSettings();
	}

	return nullptr;
}
