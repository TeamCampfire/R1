// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Options/OptionsControlsWidget.h"
#include "Widget/Options/KeyRebindRowWidget.h"
#include "Widget/Options/KeyConflictDialogWidget.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

namespace
{
	// 단축키 목록에 표시할 고정 순서 — 각 IA의 PlayerMappableKeySettings.Name 값이다.
	// Profile->GetPlayerMappingRows()는 TMap(+ 내부는 TSet)이라 등록 이력에 따라 실행마다
	// 순서가 달라질 수 있어서(예: 처음엔 상호작용이 맨 위, 나중엔 왼쪽이 맨 위) 여기서 직접 정렬한다.
	// 여기 없는 이름(나중에 새로 Player Mappable로 추가된 액션 등)은 그냥 이 목록 뒤에 붙는다 —
	// 순서만 못 정할 뿐 목록에 자동으로 나타나는 기존 동작은 그대로 유지된다.
	const TArray<FName> GRowOrderPriority = {
		TEXT("MoveForward"), TEXT("MoveBackward"), TEXT("MoveLeft"), TEXT("MoveRight"),
		TEXT("Jump"), TEXT("Sprint"), TEXT("Crouch"),
		TEXT("Interact"), TEXT("Attack"), TEXT("SecondaryAction"),
		TEXT("InventoryToggle"),
	};
}

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

	// 먼저 전부 모아서 GRowOrderPriority 기준으로 정렬한 다음 행을 만든다(순서 고정 이유는 위 주석 참고).
	TArray<FPlayerKeyMapping> SortedMappings;
	for (const TPair<FName, FKeyMappingRow>& RowPair : Profile->GetPlayerMappingRows())
	{
		for (const FPlayerKeyMapping& Mapping : RowPair.Value.Mappings)
		{
			SortedMappings.Add(Mapping);
		}
	}

	SortedMappings.Sort([](const FPlayerKeyMapping& A, const FPlayerKeyMapping& B)
	{
		int32 IndexA = GRowOrderPriority.IndexOfByKey(A.GetMappingName());
		int32 IndexB = GRowOrderPriority.IndexOfByKey(B.GetMappingName());
		if (IndexA == INDEX_NONE) { IndexA = GRowOrderPriority.Num(); }
		if (IndexB == INDEX_NONE) { IndexB = GRowOrderPriority.Num(); }
		return IndexA < IndexB;
	});

	for (const FPlayerKeyMapping& Mapping : SortedMappings)
	{
		UKeyRebindRowWidget* Row = WidgetTree->ConstructWidget<UKeyRebindRowWidget>(KeyRebindRowClass);
		if (!Row)
		{
			continue;
		}

		Row->Setup(this, Mapping.GetMappingName(), Mapping.GetSlot());
		KeyRowContainer->AddChild(Row);
	}

	UE_LOG(LogTemp, Warning, TEXT("[KeyRebind] RefreshRows 완료 — 총 매핑 %d개"), SortedMappings.Num());
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

	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	const FPlayerKeyMapping* NewActionMapping = UserSettings ? UserSettings->FindCurrentMappingForSlot(InMappingName, InSlot) : nullptr;

	PendingMappingName = InMappingName;
	PendingSlot = InSlot;
	PendingNewKey = NewKey;
	// 리바인딩 전에 PendingMappingName이 쓰고 있던 키 — Replace 시 충돌 상대에게 이걸 넘겨서 맞바꾼다.
	PendingOldKey = NewActionMapping ? NewActionMapping->GetCurrentKey() : FKey();
	PendingConflictMappingName = ConflictName;
	PendingConflictSlot = ConflictMapping->GetSlot();

	if (ConflictDialog)
	{
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
	// 기존에 그 키를 쓰던 액션(PendingConflictMappingName)은 자리를 비워두는 대신, 리바인딩 대상이
	// 원래 쓰고 있던 키(PendingOldKey)를 받는다 — 즉 두 액션의 키를 맞바꾼다(예: 앞으로 W ↔ 뒤로 S를
	// 뒤로에 W를 배정하면 앞으로 S / 뒤로 W로 스왑). 원래 미지정(빈 키)이었을 때만 그냥 비운다.
	if (PendingOldKey.IsValid())
	{
		ApplyKeyMapping(PendingConflictMappingName, PendingConflictSlot, PendingOldKey);
	}
	else if (UEnhancedInputUserSettings* UserSettings = GetUserSettings())
	{
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
