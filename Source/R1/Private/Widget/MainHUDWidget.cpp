// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/MainHUDWidget.h"
#include "Widget/Inventory/InventoryWidget.h"

void UMainHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//UE_LOG(LogTemp, Warning, TEXT("[InvToggle] MainHUDWidget::NativeOnInitialized. InventoryWidget=%s"), *GetNameSafe(InventoryWidget));

	// 인벤토리 패널은 토글로 열리는 화면이라 처음엔 닫힌 채로 시작한다.
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UMainHUDWidget::ToggleInventoryPanel()
{
	//UE_LOG(LogTemp, Warning, TEXT("[InvToggle] ToggleInventoryPanel called. InventoryWidget=%s, CurrentlyOpen=%d"),
	//	*GetNameSafe(InventoryWidget), IsInventoryPanelOpen());

	if (!InventoryWidget)
	{
		return false;
	}

	const bool bNewOpenState = !IsInventoryPanelOpen();
	InventoryWidget->SetVisibility(bNewOpenState ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	//UE_LOG(LogTemp, Warning, TEXT("[InvToggle] -> new open state=%d, resulting visibility=%d"),
	//	bNewOpenState, (int32)InventoryWidget->GetVisibility());

	return bNewOpenState;
}

bool UMainHUDWidget::IsInventoryPanelOpen() const
{
	return InventoryWidget && InventoryWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

