// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/MainHUD.h"
#include "Blueprint/UserWidget.h"
#include "Widget/MainHUDWidget.h"
#include "Widget/Multiplayer/MultiPlayerMenuWidget.h"

UMainHUDWidget* AMainHUD::GetMainHudWidget() const
{
	return MainHudWidgetInstance;
}

UMultiplayerMenuWidget* AMainHUD::GetGameMenuWidget() const
{
	return GameMenuWidgetInstance;
}

void AMainHUD::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PC = GetOwningPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (MainHudWidgetClass)
	{
		UE_LOG(LogTemp, Log, TEXT("MainHudWidgetClass Eixst"));
		MainHudWidgetInstance = CreateWidget<UMainHUDWidget>(GetWorld(), MainHudWidgetClass);
		if (MainHudWidgetInstance)
		{
			UE_LOG(LogTemp, Log, TEXT("MainHudWidgetInstance Eixst"));
			MainHudWidgetInstance->AddToViewport();
		}
	}

	if (GameMenuWidgetClass)
	{
		UE_LOG(LogTemp, Log, TEXT("GameMenuWidgetClass Exist"));
		GameMenuWidgetInstance = CreateWidget<UMultiplayerMenuWidget>(PC, GameMenuWidgetClass);

		if (GameMenuWidgetInstance)
		{
			UE_LOG(LogTemp, Log, TEXT("GameMenuWidgetInstance Exist"));
			GameMenuWidgetInstance->AddToViewport(100);	// 다른 UI들보다 가장 위에 떠야 함
			GameMenuWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
