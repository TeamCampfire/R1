// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/StatusBarWidget.h"
#include "Interface/StatusEffectInterface.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"

void UStatusBarWidget::InitializeStatusEffectWidget(const EStatusEffect inEStatusEffect)
{
	switch (inEStatusEffect)
	{
	case EStatusEffect::Hungry:
		StatusEffectText->SetText(FText::FromString(TEXT("배고픔")));
		break;
	case EStatusEffect::Thirsty:
		StatusEffectText->SetText(FText::FromString(TEXT("목마름")));
		break;
	case EStatusEffect::Starving:
		StatusEffectText->SetText(FText::FromString(TEXT("굶주림!")));
		break;
	case EStatusEffect::Dehydrated:
		StatusEffectText->SetText(FText::FromString(TEXT("탈수!")));
		break;
	}

	if (TSoftObjectPtr<UMaterialInterface>* FoundMaterial =
		StatusIconMaterials.Find(inEStatusEffect))
	{
		UMaterialInterface* Material = FoundMaterial->LoadSynchronous();

		if (Material && IconImage)
		{
			IconImage->SetBrushFromMaterial(Material);
		}
	}
}
