#pragma once

#include "CoreMinimal.h"
#include "Widget/Multiplayer/MultiplayerMenuWidget.h"
#include "GameMenuWidget.generated.h"

class UButton;
class UOptionsControlsWidget;
class UOptionsDisplayWidget;
class UWidgetSwitcher;

/**
 * 게임 메뉴의 세션 화면과 환경설정 화면 사이 전환만 담당
 * 세션 기능은 UMultiplayerMenuWidget, 옵션 기능은 기존 Options 위젯 클래스에 위임
 */
UCLASS(Blueprintable)
class R1_API UGameMenuWidget : public UMultiplayerMenuWidget
{
	GENERATED_BODY()

protected:

	virtual void NativeOnInitialized() override;

private:

	// 환경설정 화면으로 전환
	UFUNCTION()
	void HandleShowOptionsClicked();

	// 세션 목록 화면으로 전환
	UFUNCTION()
	void HandleShowSessionsClicked();

	// 환경설정 기능 - 조작키
	UFUNCTION()
	void HandleControlsTabClicked();

	// 환경설정 기능 - 화면
	UFUNCTION()
	void HandleDisplayTabClicked();

	void ShowOptionsPage();
	void ShowSessionsPage();
	void UpdateOptionTabSelection(int32 SelectedTabIndex);

	// 선택이 해제되면 WBP Designer에서 지정한 원래 색으로 복원한다.
	FLinearColor UnselectedTabColor = FLinearColor::White;
	FLinearColor SelectedTabColor = FLinearColor::Black;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetSwitcher> MenuPageSwitcher;		// Body - 세션 연결, 환경설정 화면 전환 WidgetSwitcher

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetSwitcher> HeaderModeSwitcher;		// Header - Option버튼, Sessions 버튼 전환 WidgetSwitcher

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> SessionsButton;					// 세션 연결 기능

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> ControlsTabButton;				// 환경설정 기능

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> DisplayTabButton;				// 환경설정 기능

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetSwitcher> OptionsContentSwitcher;	// 환경설정 기능 - 조작키 설정, 화면 설정 전환 WidgetSwitcher

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UOptionsControlsWidget> OptionsControlsContent;	// 조작키 설정 위젯

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UOptionsDisplayWidget> OptionsDisplayContent;	// 화면 설정 위젯
};
