// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Options/OptionsDisplayWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"

namespace
{
	// 화면모드 콤보박스에 나열되는 순서 — WindowModeComboBox의 인덱스와 EWindowMode::Type을 서로 변환할 때 이 순서를 그대로 쓴다.
	const TArray<EWindowMode::Type> GWindowModeOrder = { EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed };

	FString ResolutionToString(const FIntPoint& Resolution)
	{
		return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
	}

	FText WindowModeToDisplayText(EWindowMode::Type Mode)
	{
		switch (Mode)
		{
		case EWindowMode::Fullscreen:			return NSLOCTEXT("OptionsDisplayWidget", "Fullscreen", "전체화면");
		case EWindowMode::WindowedFullscreen:	return NSLOCTEXT("OptionsDisplayWidget", "WindowedFullscreen", "테두리없는 창모드");
		case EWindowMode::Windowed:
		default:								return NSLOCTEXT("OptionsDisplayWidget", "Windowed", "창모드");
		}
	}
}

void UOptionsDisplayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddDynamic(this, &UOptionsDisplayWidget::HandleApplyClicked);
	}

	RefreshFromCurrentSettings();
}

void UOptionsDisplayWidget::RefreshFromCurrentSettings()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	if (ResolutionComboBox)
	{
		ResolutionComboBox->ClearOptions();

		TArray<FIntPoint> SupportedResolutions;
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedResolutions);

		const FIntPoint CurrentResolution = Settings->GetScreenResolution();
		bool bCurrentResolutionInList = false;

		for (const FIntPoint& Resolution : SupportedResolutions)
		{
			ResolutionComboBox->AddOption(ResolutionToString(Resolution));
			bCurrentResolutionInList |= (Resolution == CurrentResolution);
		}

		// 현재 해상도가 지원 목록에 없으면(예: 창모드에서 자유롭게 조절된 창 크기) 목록에 추가해둔다.
		if (!bCurrentResolutionInList)
		{
			ResolutionComboBox->AddOption(ResolutionToString(CurrentResolution));
		}

		ResolutionComboBox->SetSelectedOption(ResolutionToString(CurrentResolution));
	}

	if (WindowModeComboBox)
	{
		WindowModeComboBox->ClearOptions();

		for (EWindowMode::Type Mode : GWindowModeOrder)
		{
			WindowModeComboBox->AddOption(WindowModeToDisplayText(Mode).ToString());
		}

		WindowModeComboBox->SetSelectedOption(WindowModeToDisplayText(Settings->GetFullscreenMode()).ToString());
	}
}

void UOptionsDisplayWidget::HandleApplyClicked()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	if (WindowModeComboBox)
	{
		const FString SelectedText = WindowModeComboBox->GetSelectedOption();
		for (EWindowMode::Type Mode : GWindowModeOrder)
		{
			if (WindowModeToDisplayText(Mode).ToString() == SelectedText)
			{
				Settings->SetFullscreenMode(Mode);
				break;
			}
		}
	}

	if (ResolutionComboBox)
	{
		const FString SelectedText = ResolutionComboBox->GetSelectedOption();
		FString WidthStr, HeightStr;
		if (SelectedText.Split(TEXT(" x "), &WidthStr, &HeightStr))
		{
			const FIntPoint NewResolution(FCString::Atoi(*WidthStr), FCString::Atoi(*HeightStr));
			Settings->SetScreenResolution(NewResolution);
		}
	}

	Settings->ApplyResolutionSettings(false);
	Settings->SaveSettings();
}
