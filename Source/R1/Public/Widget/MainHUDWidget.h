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

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UInteractionPromptWidget> InteractionPromptWidget;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UInventoryWidget> InventoryWidget;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBeltBarWidget> BeltBarWidget;
};
