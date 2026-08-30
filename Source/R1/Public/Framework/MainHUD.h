/// 최초작성 : 2026.08.28
/// 작 성 자 : 최 요 환
/// 간단설명 : 메인 HUD용 클래스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

class UInteractionPromptWidget;

/**
 * 
 */
UCLASS()
class R1_API AMainHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	// 인터렉션용 위젯을 임시 메인으로 설정
	// TODO : 래핑용 메인 위젯 작업되면 교체 필요
	UFUNCTION(BlueprintCallable)
	UInteractionPromptWidget* GetMainHudWidget() const;

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UInteractionPromptWidget> MainHudWidgetClass = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UInteractionPromptWidget> MainHudWidgetInstance = nullptr;
};
