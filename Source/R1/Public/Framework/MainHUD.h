/// 최초작성 : 2026.08.28
/// 작 성 자 : 최 요 환
/// 간단설명 : 메인 HUD용 클래스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

class UInteractionPromptWidget;
class UMainHUDWidget;

/**
 *
 */
UCLASS()
class R1_API AMainHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	UMainHUDWidget* GetMainHudWidget() const;

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UMainHUDWidget> MainHudWidgetClass = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UMainHUDWidget> MainHudWidgetInstance = nullptr;

};
