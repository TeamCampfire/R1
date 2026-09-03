// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/MainHUDWidget.h"
#include "Widget/Inventory/InventoryWidget.h"
#include "Widget/Options/OptionsWidget.h"

void UMainHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//UE_LOG(LogTemp, Warning, TEXT("[InvToggle] MainHUDWidget::NativeOnInitialized. InventoryWidget=%s"), *GetNameSafe(InventoryWidget));

	// 인벤토리 패널은 토글로 열리는 화면이라 처음엔 닫힌 채로 시작한다.
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 옵션 패널도 마찬가지로 ESC 토글 전엔 닫힌 채로 시작한다.
	if (OptionsWidget)
	{
		OptionsWidget->SetVisibility(ESlateVisibility::Collapsed);
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

	if (!bNewOpenState)
	{
		// 닫을 때는 선택 상태(파란 테두리)도 같이 초기화 — 다음에 열었을 때 예전 선택이 남아있지 않게.
		InventoryWidget->ClearSelection();
	}

	//UE_LOG(LogTemp, Warning, TEXT("[InvToggle] -> new open state=%d, resulting visibility=%d"),
	//	bNewOpenState, (int32)InventoryWidget->GetVisibility());

	return bNewOpenState;
}

bool UMainHUDWidget::IsInventoryPanelOpen() const
{
	return InventoryWidget && InventoryWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

bool UMainHUDWidget::ToggleOptionsPanel()
{
	if (!OptionsWidget)
	{
		return false;
	}

	const bool bNewOpenState = !IsOptionsPanelOpen();
	OptionsWidget->SetVisibility(bNewOpenState ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bNewOpenState)
	{
		// 열 때마다 다시 채운다 — 이 위젯은 게임 시작 시 한 번만 만들어지고 이후엔 Visibility만
		// 토글되므로, 최초 생성 시점에 Enhanced Input 등록이 아직 안 끝나 있었어도 다음에 열 때는
		// 최신 키 매핑으로 다시 채워진다.
		OptionsWidget->RefreshActiveCategory();
	}

	return bNewOpenState;
}

bool UMainHUDWidget::IsOptionsPanelOpen() const
{
	return OptionsWidget && OptionsWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

