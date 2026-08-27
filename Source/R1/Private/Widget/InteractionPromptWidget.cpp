// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/InteractionPromptWidget.h"
#include "Component/InteractionComponent.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "GameFramework/Pawn.h"

void UInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UInteractionComponent* InteractionComp = OwningPawn->FindComponentByClass<UInteractionComponent>())
		{
			BoundInteractionComponent = InteractionComp;
			InteractionComp->OnInteractableTargetChanged.AddDynamic(this, &UInteractionPromptWidget::HandleInteractionTargetChanged);
		}
	}

	// 시작 시점엔 조준 대상이 없는 상태로 초기화 — 대상 패널을 숨겨둔다.
	HandleInteractionTargetChanged(nullptr, FText::GetEmpty(), nullptr);
}

void UInteractionPromptWidget::NativeDestruct()
{
	if (UInteractionComponent* InteractionComp = BoundInteractionComponent.Get())
	{
		InteractionComp->OnInteractableTargetChanged.RemoveDynamic(this, &UInteractionPromptWidget::HandleInteractionTargetChanged);
	}

	Super::NativeDestruct();
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
			// 아이콘 없는 상호작용 액터(창고 등) — 이미지 슬롯만 숨기고 이름은 그대로 보여준다.
			TargetIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
