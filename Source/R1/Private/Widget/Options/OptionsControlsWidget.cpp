// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Options/OptionsControlsWidget.h"
#include "Widget/Options/KeyRebindRowWidget.h"
#include "Widget/Options/KeyConflictDialogWidget.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

void UOptionsControlsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ResetButton)
	{
		ResetButton->OnClicked.AddDynamic(this, &UOptionsControlsWidget::HandleResetClicked);
	}

	if (ConflictDialog)
	{
		ConflictDialog->OnReplaceConfirmed.AddDynamic(this, &UOptionsControlsWidget::HandleConflictReplaceConfirmed);
		ConflictDialog->OnCancelled.AddDynamic(this, &UOptionsControlsWidget::HandleConflictCancelled);
	}

	RefreshRows();
}

void UOptionsControlsWidget::RefreshRows()
{
	if (!KeyRowContainer || !KeyRebindRowClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KeyRebind] RefreshRows 중단 — KeyRowContainer=%s KeyRebindRowClass=%s"),
			KeyRowContainer ? TEXT("OK") : TEXT("NULL"),
			KeyRebindRowClass ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	KeyRowContainer->ClearChildren();

	UEnhancedPlayerMappableKeyProfile* Profile = GetActiveProfile();
	if (!Profile)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KeyRebind] RefreshRows 중단 — ActiveProfile이 NULL (UserSettings=%s)"),
			GetUserSettings() ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	int32 RowCount = 0;
	int32 MappingCount = 0;

	for (const TPair<FName, FKeyMappingRow>& RowPair : Profile->GetPlayerMappingRows())
	{
		++RowCount;
		for (const FPlayerKeyMapping& Mapping : RowPair.Value.Mappings)
		{
			++MappingCount;
			UKeyRebindRowWidget* Row = WidgetTree->ConstructWidget<UKeyRebindRowWidget>(KeyRebindRowClass);
			if (!Row)
			{
				continue;
			}

			Row->Setup(this, Mapping.GetMappingName(), Mapping.GetSlot());
			KeyRowContainer->AddChild(Row);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[KeyRebind] RefreshRows 완료 — 매핑 행 %d개, 총 매핑 %d개"), RowCount, MappingCount);
}

void UOptionsControlsWidget::HandleResetClicked()
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings)
	{
		return;
	}

	if (UEnhancedPlayerMappableKeyProfile* Profile = UserSettings->GetActiveKeyProfile())
	{
		Profile->ResetToDefault();
		UserSettings->SaveSettings();
	}

	RefreshRows();
}

void UOptionsControlsWidget::RequestKeyRebind(FName InMappingName, EPlayerMappableKeySlot InSlot, const FKey& NewKey)
{
	UEnhancedPlayerMappableKeyProfile* Profile = GetActiveProfile();
	if (!Profile)
	{
		return;
	}

	// 같은 액션(같은 MappingName)에 다시 바인딩하는 건 충돌이 아니다.
	TArray<FName> ConflictingNames;
	Profile->GetMappingNamesForKey(NewKey, ConflictingNames);
	ConflictingNames.Remove(InMappingName);

	if (ConflictingNames.Num() == 0)
	{
		ApplyKeyMapping(InMappingName, InSlot, NewKey);
		return;
	}

	// 충돌난 매핑을 찾아서(보통 하나) Replace 확인창에 띄울 정보를 뽑는다.
	const FName ConflictName = ConflictingNames[0];
	const FKeyMappingRow* ConflictRow = Profile->FindKeyMappingRow(ConflictName);
	const FPlayerKeyMapping* ConflictMapping = nullptr;
	if (ConflictRow)
	{
		for (const FPlayerKeyMapping& Mapping : ConflictRow->Mappings)
		{
			if (Mapping.GetCurrentKey() == NewKey)
			{
				ConflictMapping = &Mapping;
				break;
			}
		}
	}

	if (!ConflictMapping)
	{
		// 이론상 오면 안 되는 경우(충돌 이름은 찾았는데 정작 그 키를 쓰는 매핑이 없음) — 그냥 적용.
		ApplyKeyMapping(InMappingName, InSlot, NewKey);
		return;
	}

	PendingMappingName = InMappingName;
	PendingSlot = InSlot;
	PendingNewKey = NewKey;
	PendingConflictMappingName = ConflictName;
	PendingConflictSlot = ConflictMapping->GetSlot();

	if (ConflictDialog)
	{
		UEnhancedInputUserSettings* UserSettings = GetUserSettings();
		const FPlayerKeyMapping* NewActionMapping = UserSettings ? UserSettings->FindCurrentMappingForSlot(InMappingName, InSlot) : nullptr;
		const FText NewActionName = NewActionMapping ? NewActionMapping->GetDisplayName() : FText::FromName(InMappingName);

		ConflictDialog->ShowConflict(NewKey.GetDisplayName(), ConflictMapping->GetDisplayName(), NewActionName);
	}
}

void UOptionsControlsWidget::RequestRowReset(FName InMappingName)
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings)
	{
		return;
	}

	FMapPlayerKeyArgs Args;
	Args.MappingName = InMappingName;

	FGameplayTagContainer FailureReason;
	UserSettings->ResetAllPlayerKeysInRow(Args, FailureReason);

	if (FailureReason.IsEmpty())
	{
		UserSettings->SaveSettings();
	}

	RefreshRows();
}

void UOptionsControlsWidget::ApplyKeyMapping(FName InMappingName, EPlayerMappableKeySlot InSlot, const FKey& NewKey)
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings)
	{
		return;
	}

	FMapPlayerKeyArgs Args;
	Args.MappingName = InMappingName;
	Args.Slot = InSlot;
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
		UE_LOG(LogTemp, Warning, TEXT("[KeyRebind] MapPlayerKey 실패 (%s): %s"), *InMappingName.ToString(), *FailureReason.ToString());
	}

	RefreshRows();
}

void UOptionsControlsWidget::HandleConflictReplaceConfirmed()
{
	if (UEnhancedInputUserSettings* UserSettings = GetUserSettings())
	{
		// 기존에 그 키를 쓰던 액션에서 먼저 빼앗는다 — 안 그러면 두 액션이 같은 키를 같이 쓰게 된다.
		FMapPlayerKeyArgs UnmapArgs;
		UnmapArgs.MappingName = PendingConflictMappingName;
		UnmapArgs.Slot = PendingConflictSlot;

		FGameplayTagContainer FailureReason;
		UserSettings->UnMapPlayerKey(UnmapArgs, FailureReason);
	}

	ApplyKeyMapping(PendingMappingName, PendingSlot, PendingNewKey);

	if (ConflictDialog)
	{
		ConflictDialog->CloseDialog();
	}
}

void UOptionsControlsWidget::HandleConflictCancelled()
{
	if (ConflictDialog)
	{
		ConflictDialog->CloseDialog();
	}
}

UEnhancedInputUserSettings* UOptionsControlsWidget::GetUserSettings() const
{
	if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer()))
	{
		return SubSystem->GetUserSettings();
	}

	return nullptr;
}

UEnhancedPlayerMappableKeyProfile* UOptionsControlsWidget::GetActiveProfile() const
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	return UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
}
