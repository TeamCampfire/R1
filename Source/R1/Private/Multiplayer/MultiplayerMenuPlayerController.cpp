#include "Multiplayer/MultiplayerMenuPlayerController.h"

#include "Widget/Multiplayer/MultiplayerMenuWidget.h"

void AMultiplayerMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	/* 자신의 메뉴 위젯과 UI 입력 모드 준비 */
	
	if (!IsLocalController() || !MultiplayerMenuWidgetClass)
	{
		return;
	}

	// 위젯 생성
	MultiplayerMenuWidget = CreateWidget<UMultiplayerMenuWidget>(this, MultiplayerMenuWidgetClass);
	// 위젯 안 만들어졌으면 return
	if (!MultiplayerMenuWidget)
	{
		return;
	}

	// 위젯 만들어졌으면 화면에 띄우기
	MultiplayerMenuWidget->AddToViewport();

	// 세션 목록 자동 새로고침
	MultiplayerMenuWidget->RefreshSessions();

	// UI 입력 모드로 전환
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MultiplayerMenuWidget->TakeWidget());
	SetInputMode(InputMode);	// 입력 모드 적용
	SetShowMouseCursor(true);	// 마우스 커서 보이게 유지

}
