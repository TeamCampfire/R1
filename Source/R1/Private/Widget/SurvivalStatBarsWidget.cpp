// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/SurvivalStatBarsWidget.h"
#include "Widget/ParameterBarWidget.h"
#include "Interface/StatInterface.h"
#include "Interface/HealthInterface.h"
#include "Interface/HydrationInterface.h"
#include "Interface/CaloriesInterface.h"
#include "Component/StatComponent.h"

void USurvivalStatBarsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeSurvivalStatBars();
}

void USurvivalStatBarsWidget::InitializeSurvivalStatBars()
{
	if (IStatInterface* OwnerPlayer = Cast<IStatInterface>(GetOwningPlayerPawn()))
	{
		if (UStatComponent* StatComp = OwnerPlayer->GetStatComponent())
		{
			StatComp->OnHealthChange.AddDynamic(HealthBar, &UParameterBarWidget::UpdateParameterBar);

			HealthBar->UpdateParameterBar(
				IHealthInterface::Execute_GetCurrentHealth(StatComp),
				IHealthInterface::Execute_GetMaxHealth(StatComp));
			UE_LOG(LogTemp, Log, TEXT("%f"), IHealthInterface::Execute_GetCurrentHealth(StatComp));
		}
	}

}
