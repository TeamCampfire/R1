// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Options/KeyRebindRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

void UKeyRebindRowWidget::Setup(FName InMappingName, EPlayerMappableKeySlot InSlot)
{
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

void UKeyRebindRowWidget::HandleKeyButtonClicked()
{
	BeginListening();
}

void UKeyRebindRowWidget::BeginListening()
{
	bIsListening = true;

	if (KeyText)
	{
		KeyText->SetText(NSLOCTEXT("KeyRebindRowWidget", "Listening", "키를 누르세요..."));
	}

	SetFocus();
}

void UKeyRebindRowWidget::ApplyNewKey(const FKey& NewKey)
{
	bIsListening = false;

	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings)
	{
		RefreshKeyText();
		return;
	}

	FMapPlayerKeyArgs Args = FMapPlayerKeyArgs();
	Args.MappingName = MappingName;
	Args.Slot = Slot;
	Args.NewKey = NewKey;
	Args.bCreateMatchingSlotIfNeeded = true;

	FGameplayTagContainer FailureReason;
	UserSettings->MapPlayerKey(Args, FailureReason);

	if (FailureReason.IsEmpty())
	{
		UserSettings->SaveSettings();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[KeyRebind] MapPlayerKey 실패 (%s): %s"), *MappingName.ToString(), *FailureReason.ToString());
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
