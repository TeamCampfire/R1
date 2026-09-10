// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/SurvivalStatBarsWidget.h"
#include "Widget/ParameterBarWidget.h"
#include "Widget/ParameterBarWidgetTest.h"
#include "Widget/StatusBarWidget.h"
#include "Interface/StatInterface.h"
#include "Interface/HealthInterface.h"
#include "Interface/HydrationInterface.h"
#include "Interface/CaloriesInterface.h"
#include "Interface/StatusEffectInterface.h"
#include "Character/ActionPlayerController.h"
#include "Component/StatComponent.h"
#include "Components/VerticalBox.h"
#include "Widget/PickupNotificationWidget.h"
#include "Components/PanelWidget.h"

void USurvivalStatBarsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AActionPlayerController* PC = Cast<AActionPlayerController>(GetOwningPlayer());
	if (PC)
	{
		PC->OnPossessedCharChange.AddDynamic(
			this,
			&USurvivalStatBarsWidget::InitializeSurvivalStatBars
		);
	}
	InitializeSurvivalStatBars();
}

void USurvivalStatBarsWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void USurvivalStatBarsWidget::InitializeSurvivalStatBars()
{
	UnbindStatDelegates();
	if (IStatInterface* OwnerPlayer = Cast<IStatInterface>(GetOwningPlayerPawn()))
	{
		if (StatComp = OwnerPlayer->GetStatComponent())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("=== BIND HEALTH === Widget=%p Pawn=%p PawnName=%s StatComp=%p HealthBar=%p"),
				this,
				GetOwningPlayerPawn(),
				*GetNameSafe(GetOwningPlayerPawn()),
				StatComp.Get(),
				HealthBar.Get());

			StatComp->OnHealthChange.AddDynamic(HealthBar, &UParameterBarWidget::UpdateParameterBar);
			StatComp->OnHydrationChange.AddDynamic(HydrationBar, &UParameterBarWidget::UpdateParameterBar);
			StatComp->OnCaloryChange.AddDynamic(CaloriesBar, &UParameterBarWidget::UpdateParameterBar);
			StatComp->OnStatusEffectChange.AddDynamic(this, &USurvivalStatBarsWidget::UpdateStatusEffects);

			HealthBar->UpdateParameterBar(
				IHealthInterface::Execute_GetCurrentHealth(StatComp),
				IHealthInterface::Execute_GetMaxHealth(StatComp));
			HydrationBar->UpdateParameterBar(
				IHydrationInterface::Execute_GetCurrentHydration(StatComp),
				IHydrationInterface::Execute_GetMaxHydration(StatComp));
			CaloriesBar->UpdateParameterBar(
				ICaloriesInterface::Execute_GetCurrentCalories(StatComp),
				ICaloriesInterface::Execute_GetMaxCalories(StatComp));
			UpdateStatusEffects();
		}
	}
}

void USurvivalStatBarsWidget::UnbindStatDelegates()
{
	if (StatComp)
	{
		StatComp->OnHealthChange.RemoveDynamic(HealthBar, &UParameterBarWidget::UpdateParameterBar);
		StatComp->OnHydrationChange.RemoveDynamic(HydrationBar, &UParameterBarWidget::UpdateParameterBar);
		StatComp->OnCaloryChange.RemoveDynamic(CaloriesBar, &UParameterBarWidget::UpdateParameterBar);
		StatComp->OnStatusEffectChange.RemoveDynamic(this, &USurvivalStatBarsWidget::UpdateStatusEffects);
	}
}

void USurvivalStatBarsWidget::UpdateStatusEffects()
{

    UWorld* World = GetWorld();

    if (!World || World->bIsTearingDown) return;
    
    if (!IsValid(StatusBarWidgetClass)) return;

    IStatInterface* OwnerActor =
        Cast<IStatInterface>(GetOwningPlayerPawn());

    if (!OwnerActor) return;

    //UStatComponent* StatComp = OwnerActor->GetStatComponent();

    if (!StatComp) return;

    const EStatusEffect ActiveEffects =
        IStatusEffectInterface::Execute_GetCurrentStatusEffect(
            StatComp
        );

    //TMap을 순회하면서 이미 존재하는 효과는 제거
    for (auto It = StatusBarWidgets.CreateIterator(); It; ++It)
    {
        if (!EnumHasAnyFlags(ActiveEffects, It.Key()))
        {
            It.Value()->RemoveFromParent(); // 위젯을 Debuffs 세로박스에서 제거
            It.RemoveCurrent();             // TMap에서 Key/Value 제거
        }
    }

    const UEnum* StatusEffectEnum = StaticEnum<EStatusEffect>();

    // StatusEffect의 이름을 순회하며 현재 활성화된 효과 추가
    for (int32 Index = 0; Index < StatusEffectEnum->NumEnums(); ++Index)
    {
        const EStatusEffect EachEffect = static_cast<EStatusEffect>(StatusEffectEnum->GetValueByIndex(Index));

        if (EachEffect == EStatusEffect::None) continue;
        if (!EnumHasAnyFlags(ActiveEffects, EachEffect)) continue;

        // 이미 존재하는지 체크
        if (!StatusBarWidgets.Contains(EachEffect))
        {
            TWeakObjectPtr<USurvivalStatBarsWidget> WeakThis(this);

            GetWorld()->GetTimerManager().SetTimerForNextTick(
                [WeakThis, EachEffect]()
                {
                    if (!WeakThis.IsValid()) return;

                    USurvivalStatBarsWidget* This = WeakThis.Get();

                    UStatusBarWidget* StatusWidget =
                        CreateWidget<UStatusBarWidget>(
                            This,
                            This->StatusBarWidgetClass
                        );

                    if (!IsValid(StatusWidget)) return;

                    StatusWidget->InitializeStatusEffectWidget(EachEffect);
                    UVerticalBoxSlot* Slot = This->Debuffs->AddChildToVerticalBox(StatusWidget);
                    Slot->SetPadding(FMargin(0.f, 2.f));
                    This->StatusBarWidgets.Add(EachEffect, StatusWidget);
                }
            );
        }
    }
}

void USurvivalStatBarsWidget::AddPickupNotification(UItemDataBase* ItemData, int32 GainedAmount, int32 NewTotalCount)
{
	if (!PickupNotificationContainer || !PickupNotificationWidgetClass || !ItemData || GainedAmount <= 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PickupNotif] AddPickupNotification early-out. Container=%s WidgetClass=%s ItemData=%s GainedAmount=%d"),
			*GetNameSafe(PickupNotificationContainer),
			*GetNameSafe(PickupNotificationWidgetClass),
			*GetNameSafe(ItemData),
			GainedAmount);
		return;
	}

	UPickupNotificationWidget* Notification = CreateWidget<UPickupNotificationWidget>(this, PickupNotificationWidgetClass);
	if (!Notification)
	{
		return;
	}

	Notification->Initialize(ItemData, GainedAmount, NewTotalCount, PickupNotificationLifetime, PickupNotificationFadeOutDuration);
	Notification->OnExpired.AddDynamic(this, &USurvivalStatBarsWidget::RemovePickupNotification);

	// 같은 아이템이어도 합치지 않고 매번 새 행 — 항상 맨 앞(최신/맨 위)에 꽂는다.
	ActivePickupNotifications.Insert(Notification, 0);
	RefreshPickupNotificationContainer();
}

void USurvivalStatBarsWidget::RemovePickupNotification(UPickupNotificationWidget* Notification)
{
	ActivePickupNotifications.RemoveSingle(Notification);

	// Notification을 지우기만 하면 나머지 행들이 그자리에 남아 있어 빈 공간이 생기므로,
	// 컨테이너를 새로 갱신하여 남은 행들을 아래로 내린다.
	RefreshPickupNotificationContainer();
}

void USurvivalStatBarsWidget::RefreshPickupNotificationContainer()
{
	if (!PickupNotificationContainer)
	{
		return;
	}

	PickupNotificationContainer->ClearChildren();
	for (UPickupNotificationWidget* Notification : ActivePickupNotifications)
	{
		if (Notification)
		{
			PickupNotificationContainer->AddChild(Notification);
		}
	}
}
