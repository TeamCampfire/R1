/// 최초작성 : 2026.09.02
/// 작 성 자 : 최 요 환
/// 간단설명 : 옵션 "단축키" 패널에서 리바인딩 가능한 액션 한 줄 — Enhanced Input Player Mappable Key 하나에 대응한다.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "KeyRebindRowWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다:
 * - ActionNameText : 액션의 표시 이름(예: "앞으로").
 * - KeyButton      : 누르면 다음 키 입력을 기다리는 리스닝 모드로 들어간다.
 * - KeyText        : 현재 매핑된 키 이름, 리스닝 중엔 안내 문구.
 *
 * OptionsControlsWidget이 UEnhancedInputUserSettings::GetActiveKeyProfile()의 매핑 하나마다
 * 이 위젯을 동적으로 생성하고 Setup()을 호출해준다.
 *
 * 리스닝 중엔 이 위젯이 키보드 포커스를 가져가 NativeOnKeyDown으로 다음 키 입력을 잡는다.
 * 마우스 버튼 리바인딩(NativeOnMouseButtonDown)은 KeyButton 자체를 클릭하면 버튼이 클릭을
 * 먼저 소비해버려서 캡처되지 않는다 — 리스닝 중 이 행의 KeyButton 이외 영역을 클릭해야 잡힌다.
 */
UCLASS()
class R1_API UKeyRebindRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(FName InMappingName, EPlayerMappableKeySlot InSlot);

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End UUserWidget Interface

	UFUNCTION()
	void HandleKeyButtonClicked();

	void RefreshKeyText();
	void BeginListening();
	void ApplyNewKey(const FKey& NewKey);

	class UEnhancedInputUserSettings* GetUserSettings() const;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> KeyButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyText;

	FName MappingName;
	EPlayerMappableKeySlot Slot = EPlayerMappableKeySlot::First;
	bool bIsListening = false;
};
