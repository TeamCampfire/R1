// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/PickupNotificationWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Data/Item/ItemDataBase.h"

void UPickupNotificationWidget::Initialize(UItemDataBase* ItemData, int32 GainedAmount, int32 NewTotalCount, float InLifetime, float InFadeOutDuration)
{
	if (!ItemData)
	{
		return;
	}

	if (IconImage)
	{
		if (UTexture2D* Icon = ItemData->Icon.LoadSynchronous())
		{
			IconImage->SetBrushFromTexture(Icon);
			IconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (NameText)
	{
		NameText->SetText(ItemData->DisplayName);
	}

	if (AmountText)
	{
		AmountText->SetText(FText::Format(NSLOCTEXT("PickupNotificationWidget", "AmountFormat", "+{0} ({1})"),
			FText::AsNumber(GainedAmount), FText::AsNumber(NewTotalCount)));
	}

	Lifetime = InLifetime;
	FadeOutDuration = FMath::Clamp(InFadeOutDuration, 0.f, InLifetime);
	ElapsedTime = 0.f;
	bHasExpired = false;
	SetRenderOpacity(1.f);
}

void UPickupNotificationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bHasExpired)
	{
		return;
	}

	ElapsedTime += InDeltaTime;

	// 페이드 시작 시점 이후로는 매 틱 오파시티를 1→0으로 보간 — WBP 애니메이션 없이 코드만으로 처리.
	const float FadeStartTime = Lifetime - FadeOutDuration;
	if (FadeOutDuration > 0.f && ElapsedTime >= FadeStartTime)
	{
		const float FadeAlpha = 1.f - FMath::Clamp((ElapsedTime - FadeStartTime) / FadeOutDuration, 0.f, 1.f);
		SetRenderOpacity(FadeAlpha);
	}

	if (ElapsedTime >= Lifetime)
	{
		bHasExpired = true;
		OnExpired.Broadcast(this);
	}
}
