/// 최초작성 : 2026.09.02
/// 작 성 자 : 최 요 환
/// 간단설명 : 옵션 패널의 "단축키" 카테고리 — Player Mappable Key로 등록된 모든 매핑을 리스트로 보여주고 리바인딩/초기화한다.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OptionsControlsWidget.generated.h"

class UPanelWidget;
class UButton;
class UKeyRebindRowWidget;
class UEnhancedInputUserSettings;

/**
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다:
 * - KeyRowContainer : 리바인딩 행들을 담는 컨테이너(ScrollBox 추천).
 * - ResetButton     : 모든 리바인딩을 IMC 기본 키로 되돌린다.
 *
 * 행 목록은 정적으로 만들어두지 않고, UEnhancedInputUserSettings에 "Player Mappable"로 등록된
 * 매핑을 그대로 순회해서 만든다 — 에디터에서 IMC 매핑에 체크만 추가하면 C++ 재작업 없이 이 목록에
 * 자동으로 나타난다.
 */
UCLASS()
class R1_API UOptionsControlsWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	// KeyRowContainer를 비우고 현재 등록된 모든 Player Mappable Key 매핑으로 다시 채운다.
	void RefreshRows();

	UFUNCTION()
	void HandleResetClicked();

	UEnhancedInputUserSettings* GetUserSettings() const;

	// 각 행으로 만들어질 위젯 클래스 — WBP_KeyRebindRow를 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Options")
	TSubclassOf<UKeyRebindRowWidget> KeyRebindRowClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> KeyRowContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetButton;
};
