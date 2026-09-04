#include "Widget/GameMenu/GameMenuWidget.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

#include "Widget/Options/OptionsControlsWidget.h"
#include "Widget/Options/OptionsDisplayWidget.h"

namespace
{
	constexpr int32 SessionsPageIndex = 0;
	constexpr int32 OptionsPageIndex = 1;
	constexpr int32 ControlsTabIndex = 0;
	constexpr int32 DisplayTabIndex = 1;
}

void UGameMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 부모 클래스가 기존 세션 버튼들의 기능을 바인딩한다. 여기서는 화면 전환만 추가한다.
	GetOptionButton()->OnClicked.AddUniqueDynamic(this, &UGameMenuWidget::HandleShowOptionsClicked);
	SessionsButton->OnClicked.AddUniqueDynamic(this, &UGameMenuWidget::HandleShowSessionsClicked);
	ControlsTabButton->OnClicked.AddUniqueDynamic(this, &UGameMenuWidget::HandleControlsTabClicked);
	DisplayTabButton->OnClicked.AddUniqueDynamic(this, &UGameMenuWidget::HandleDisplayTabClicked);

	ShowSessionsPage();
	OptionsContentSwitcher->SetActiveWidgetIndex(ControlsTabIndex);
	UpdateOptionTabSelection(ControlsTabIndex);
}

void UGameMenuWidget::HandleShowOptionsClicked()
{
	ShowOptionsPage();
}

void UGameMenuWidget::HandleShowSessionsClicked()
{
	ShowSessionsPage();
}

void UGameMenuWidget::HandleControlsTabClicked()
{
	OptionsContentSwitcher->SetActiveWidgetIndex(ControlsTabIndex);
	UpdateOptionTabSelection(ControlsTabIndex);
	OptionsControlsContent->RefreshRows();
}

void UGameMenuWidget::HandleDisplayTabClicked()
{
	OptionsContentSwitcher->SetActiveWidgetIndex(DisplayTabIndex);
	UpdateOptionTabSelection(DisplayTabIndex);
	OptionsDisplayContent->RefreshFromCurrentSettings();
}

void UGameMenuWidget::ShowOptionsPage()
{
	MenuPageSwitcher->SetActiveWidgetIndex(OptionsPageIndex);
	HeaderModeSwitcher->SetActiveWidgetIndex(OptionsPageIndex);

	// 게임 메뉴 인스턴스는 계속 살아 있으므로 진입할 때마다 현재 설정을 다시 읽음
	OptionsControlsContent->RefreshRows();
	OptionsDisplayContent->RefreshFromCurrentSettings();
	UpdateOptionTabSelection(OptionsContentSwitcher->GetActiveWidgetIndex());
}

void UGameMenuWidget::ShowSessionsPage()
{
	MenuPageSwitcher->SetActiveWidgetIndex(SessionsPageIndex);
	HeaderModeSwitcher->SetActiveWidgetIndex(SessionsPageIndex);
}

void UGameMenuWidget::UpdateOptionTabSelection(int32 SelectedTabIndex)
{
	ControlsTabButton->SetBackgroundColor(
		SelectedTabIndex == ControlsTabIndex ? SelectedTabColor : UnselectedTabColor
	);
	DisplayTabButton->SetBackgroundColor(
		SelectedTabIndex == DisplayTabIndex ? SelectedTabColor : UnselectedTabColor
	);
}
