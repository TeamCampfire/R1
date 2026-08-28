// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/MainHUD.h"
#include "Blueprint/UserWidget.h"
#include "Widget/InteractionPromptWidget.h"

UInteractionPromptWidget* AMainHUD::GetMainHudWidget() const
{
	return MainHudWidgetInstance;
}

void AMainHUD::BeginPlay()
{
	Super::BeginPlay();

	if (MainHudWidgetClass)
	{
		MainHudWidgetInstance = CreateWidget<UInteractionPromptWidget>(GetWorld(), MainHudWidgetClass);
		if (MainHudWidgetInstance)
		{
			MainHudWidgetInstance->AddToViewport();
		}
	}
}
