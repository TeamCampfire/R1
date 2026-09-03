/// 최초작성 : 2026.08.31
/// 작 성 자 : 최 요 환
/// 간단설명 : 메인 UI용 위젯 클래스
/// 
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class UInteractionPromptWidget;
class UInventoryWidget;
class UBeltBarWidget;
class UOptionsWidget;
/**
 * 
 */
UCLASS()
class R1_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 인벤토리 패널(장비+메인)을 열려있으면 닫고, 닫혀있으면 연다. 전환 후 열림 상태를 돌려준다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ToggleInventoryPanel();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsInventoryPanelOpen() const;

	// 건축 설치 실패 메시지를 화면에 표시하는 함수
	// 같은 메시지를 연속으로 요청하면 기존 타이머를 초기화하여 마지막 요청 시점부터 DisplayDuration 동안 다시 표시
	void ShowBuildingPlacementMessage(const FText& Message, float DisplayDuration = 1.5f);

	// 옵션(환경설정) 패널을 열려있으면 닫고, 닫혀있으면 연다. 전환 후 열림 상태를 돌려준다.
	UFUNCTION(BlueprintCallable, Category = "Options")
	bool ToggleOptionsPanel();

	UFUNCTION(BlueprintPure, Category = "Options")
	bool IsOptionsPanelOpen() const;

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

	// 현재 표시 중인 건축 안내 메시지를 숨기는 함수
	void HideBuildingPlacementMessage();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UInteractionPromptWidget> InteractionPromptWidget;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UInventoryWidget> InventoryWidget;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBeltBarWidget> BeltBarWidget;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UOptionsWidget> OptionsWidget;

	// 건축 메시지 관련
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UBorder> Border_BuildingPlacementMessage; // 건축 설치 실패 메시지 전체 배경

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UTextBlock> Text_BuildingPlacementMessage; // 실제 건축 설치 실패 문구를 표시하는 텍스트

	FTimerHandle BuildingPlacementMessageTimerHandle; // 건축 안내 메시지를 자동으로 숨기는 타이머
	// 연속 클릭 시 기존 타이머를 취소하고 다시 시작

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_BuildingPlacementMessage; // WBP_MainHUD에서 만든 설치 실패 메시지 페이드 애니메이션
};
