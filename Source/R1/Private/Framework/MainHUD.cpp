// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/MainHUD.h"
#include "Blueprint/UserWidget.h"
#include "Widget/MainHUDWidget.h"

UMainHUDWidget* AMainHUD::GetMainHudWidget() const
{
	return MainHudWidgetInstance;
}

void AMainHUD::BeginPlay()
{
	Super::BeginPlay();

	if (MainHudWidgetClass)
	{
		MainHudWidgetInstance = CreateWidget<UMainHUDWidget>(GetWorld(), MainHudWidgetClass);
		if (MainHudWidgetInstance)
		{
			MainHudWidgetInstance->AddToViewport();
		}
	}

}
