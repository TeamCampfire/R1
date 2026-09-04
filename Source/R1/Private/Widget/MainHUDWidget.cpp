// Fill out your copyright notice in the Description page of Project Settings.

#include "Widget/MainHUDWidget.h"
#include "Widget/Inventory/InventoryWidget.h"
#include "TimerManager.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

void UMainHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//UE_LOG(LogTemp, Warning, TEXT("[InvToggle] MainHUDWidget::NativeOnInitialized. InventoryWidget=%s"), *GetNameSafe(InventoryWidget));

	// 인벤토리 패널은 토글로 열리는 화면이라 처음엔 닫힌 채로 시작한다.
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 게임을 시작했을 때 이전 디자인용 테스트 문구가 화면에 표시되지 않도록 숨겨요
	if (true == IsValid(Border_BuildingPlacementMessage))
		Border_BuildingPlacementMessage->SetVisibility(ESlateVisibility::Collapsed);
}

void UMainHUDWidget::NativeDestruct()
{
	// 위젯이 제거될 때 예약된 타이머가 남아 제거된 위젯을 다시 호출하지 않도록 정리해줍니다
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(BuildingPlacementMessageTimerHandle);

	// 위젯이 제거될 때 실행 중인 건축 실패 메시지 애니메이션도 정지
	if (true == IsValid(Anim_BuildingPlacementMessage))
		StopAnimation(Anim_BuildingPlacementMessage);

	Super::NativeDestruct();
}

void UMainHUDWidget::HideBuildingPlacementMessage()
{
	if (true == IsValid(Border_BuildingPlacementMessage)) // 메시지 보더 숨겨요
		Border_BuildingPlacementMessage->SetVisibility(ESlateVisibility::Collapsed);
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

void UMainHUDWidget::ShowBuildingPlacementMessage(const FText& Message, float DisplayDuration)
{
	if (false == IsValid(Border_BuildingPlacementMessage) || false == IsValid(Text_BuildingPlacementMessage)) return;

	// 전달받은 설치 실패 문구로 텍스트를 갱신
	Text_BuildingPlacementMessage->SetText(Message);

	// 안내 UI가 게임 입력을 막지 않도록 마우스 입력을 받지 않는 Visibility 상태로 표시해요
	Border_BuildingPlacementMessage->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 연속 좌클릭 시 기존 페이드를 이어서 재생하지 않고 완전히 첨부터 진행
	if (true == IsValid(Anim_BuildingPlacementMessage))
	{
		StopAnimation(Anim_BuildingPlacementMessage);
		PlayAnimation(Anim_BuildingPlacementMessage, 0.0f, 1, EUMGSequencePlayMode::Forward);
	}

	UWorld* World = GetWorld();
	if (false == IsValid(World)) return;

	FTimerManager& TimerManager = World->GetTimerManager();

	// 좌클릭을 연속으로 누른 경우 이전 타이머를 제거하여 메시지가 중간에 갑자기 사라지지 않게 해요
	TimerManager.ClearTimer(BuildingPlacementMessageTimerHandle);

	// 잘못된 시간이 전달되어 즉시 사라지지 않도록 최소 표시 시간을 보장..
	const float SafeDisplayDuration = FMath::Max(DisplayDuration, 0.1f);

	TimerManager.SetTimer(
		BuildingPlacementMessageTimerHandle,
		this,
		&UMainHUDWidget::HideBuildingPlacementMessage,
		SafeDisplayDuration,
		false);
}
