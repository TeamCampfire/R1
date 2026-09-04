// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Options/KeyRebindRowWidget.h"
#include "Widget/Options/OptionsControlsWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

namespace
{
	// 리스닝(다음 키 입력 대기) 중 KeyButton에 곱해줄 강조색 — "PRESS A KEY" 상태를 시각적으로 표시.
	const FLinearColor GListeningColor = FLinearColor(0.376f, 0.702f, 0.294f, 1.f);
	const FLinearColor GNormalColor = FLinearColor::White;
}

void UKeyRebindRowWidget::Setup(UOptionsControlsWidget* InOwner, FName InMappingName, EPlayerMappableKeySlot InSlot)
{
	OwnerControlsWidget = InOwner;
	MappingName = InMappingName;
	Slot = InSlot;

	if (const UEnhancedInputUserSettings* UserSettings = GetUserSettings())
	{
		if (const FPlayerKeyMapping* Mapping = UserSettings->FindCurrentMappingForSlot(MappingName, Slot))
		{
			if (ActionNameText)
			{
				ActionNameText->SetText(Mapping->GetDisplayName());
			}
		}
	}

	RefreshKeyText();
}

void UKeyRebindRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetIsFocusable(true);

	if (KeyButton)
	{
		KeyButton->OnClicked.AddDynamic(this, &UKeyRebindRowWidget::HandleKeyButtonClicked);
	}

	if (ResetRowButton)
	{
		ResetRowButton->OnClicked.AddDynamic(this, &UKeyRebindRowWidget::HandleResetRowClicked);
		// 평소엔 투명하게 숨겨뒀다가 마우스가 이 행 위에 있는 동안만(NativeOnMouseEnter/Leave) 보여준다.
		// Visibility(Hidden/Collapsed)로 토글하면 그 전환 순간 레이아웃/히트테스트 경로가 다시 계산되면서
		// KeyButton -> ResetRowButton으로 커서를 옮기는 도중에 부모(이 행)의 MouseLeave가 잘못 튀어
		// 버튼이 도로 숨어버리는 문제가 있었다. RenderOpacity+IsEnabled만 바꾸면 위젯이 항상 Visible
		// 상태를 유지해서 히트테스트 트리 구조가 절대 안 바뀌므로 이 문제가 안 생긴다.
		ResetRowButton->SetRenderOpacity(0.f);
		ResetRowButton->SetIsEnabled(false);
	}
}

FReply UKeyRebindRowWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bIsListening)
	{
		const FKey PressedKey = InKeyEvent.GetKey();

		if (PressedKey == EKeys::Escape)
		{
			// 취소 — 새 키를 매핑하지 않고 리스닝만 종료한다.
			bIsListening = false;
			if (KeyButton)
			{
				KeyButton->SetBackgroundColor(GNormalColor);
			}
			RefreshKeyText();
		}
		else
		{
			ApplyNewKey(PressedKey);
		}

		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UKeyRebindRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsListening)
	{
		ApplyNewKey(InMouseEvent.GetEffectingButton());
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UKeyRebindRowWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (ResetRowButton)
	{
		ResetRowButton->SetRenderOpacity(1.f);
		ResetRowButton->SetIsEnabled(true);
	}
}

void UKeyRebindRowWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (ResetRowButton)
	{
		ResetRowButton->SetRenderOpacity(0.f);
		ResetRowButton->SetIsEnabled(false);
	}
}

void UKeyRebindRowWidget::HandleKeyButtonClicked()
{
	BeginListening();
}

void UKeyRebindRowWidget::HandleResetRowClicked()
{
	if (OwnerControlsWidget)
	{
		OwnerControlsWidget->RequestRowReset(MappingName);
	}
}

void UKeyRebindRowWidget::BeginListening()
{
	bIsListening = true;

	if (KeyText)
	{
		KeyText->SetText(NSLOCTEXT("KeyRebindRowWidget", "Listening", "키를 누르세요..."));
	}

	if (KeyButton)
	{
		KeyButton->SetBackgroundColor(GListeningColor);
	}

	SetFocus();
}

void UKeyRebindRowWidget::ApplyNewKey(const FKey& NewKey)
{
	bIsListening = false;

	if (KeyButton)
	{
		KeyButton->SetBackgroundColor(GNormalColor);
	}

	// 실제 매핑(+충돌 검사)은 다른 행들의 상태까지 알아야 해서 OptionsControlsWidget에게 넘긴다.
	// 충돌이 없으면 그쪽에서 바로 적용 후 RefreshRows로 이 행 자체를 다시 만들고,
	// 충돌이 있으면 다이얼로그를 띄운 채 대기하므로 여기서는 일단 원래 키로 되돌려 보여준다.
	if (OwnerControlsWidget)
	{
		OwnerControlsWidget->RequestKeyRebind(MappingName, Slot, NewKey);
	}

	RefreshKeyText();
}

void UKeyRebindRowWidget::RefreshKeyText()
{
	if (!KeyText)
	{
		return;
	}

	if (const UEnhancedInputUserSettings* UserSettings = GetUserSettings())
	{
		if (const FPlayerKeyMapping* Mapping = UserSettings->FindCurrentMappingForSlot(MappingName, Slot))
		{
			KeyText->SetText(Mapping->GetCurrentKey().GetDisplayName());
			return;
		}
	}

	KeyText->SetText(NSLOCTEXT("KeyRebindRowWidget", "Unbound", "미지정"));
}

UEnhancedInputUserSettings* UKeyRebindRowWidget::GetUserSettings() const
{
	if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer()))
	{
		return SubSystem->GetUserSettings();
	}

	return nullptr;
}
