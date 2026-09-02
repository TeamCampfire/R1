// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/InteractionPromptWidget.h"
#include "Component/InteractionComponent.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "GameFramework/Pawn.h"
#include "Character/ActionPlayerController.h"

void UInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->OnPossessedCharChange.AddDynamic(this, &UInteractionPromptWidget::RebindInteraction);
	}

	RebindInteraction();
}

void UInteractionPromptWidget::NativeDestruct()
{
	UnbindInteractionDelegates();

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer()))
	{
		PC->OnPossessedCharChange.RemoveDynamic(this, &UInteractionPromptWidget::RebindInteraction);
	}

	Super::NativeDestruct();
}

void UInteractionPromptWidget::UnbindInteractionDelegates()
{
	if (UInteractionComponent* InteractionComp = BoundInteractionComponent.Get())
	{
		InteractionComp->OnInteractableTargetChanged.RemoveDynamic(this, &UInteractionPromptWidget::HandleInteractionTargetChanged);
	}
}

void UInteractionPromptWidget::RebindInteraction()
{
	UnbindInteractionDelegates();
	BoundInteractionComponent = nullptr;

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UInteractionComponent* InteractionComp = OwningPawn->FindComponentByClass<UInteractionComponent>())
		{
			BoundInteractionComponent = InteractionComp;
			InteractionComp->OnInteractableTargetChanged.AddDynamic(this, &UInteractionPromptWidget::HandleInteractionTargetChanged);
		}
	}

	// 대상 없음 상태로 초기화(부활 직후엔 아직 아무것도 조준 안 한 상태이므로).
	HandleInteractionTargetChanged(nullptr, FText::GetEmpty(), nullptr);
}

void UInteractionPromptWidget::HandleInteractionTargetChanged(AActor* NewTarget, const FText& DisplayName, UTexture2D* Icon)
{
	if (TargetPanel)
	{
		TargetPanel->SetVisibility(NewTarget ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (TargetNameText)
	{
		TargetNameText->SetText(DisplayName);
	}

	if (TargetIcon)
	{
		if (Icon)
		{
			TargetIcon->SetBrushFromTexture(Icon);
			TargetIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// 아이콘 없는 상호작용 액터 — 이미지 슬롯만 숨기고 이름은 그대로 보여준다.
			TargetIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
