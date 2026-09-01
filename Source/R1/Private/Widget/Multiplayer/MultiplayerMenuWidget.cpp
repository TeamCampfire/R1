#include "Widget/Multiplayer/MultiplayerMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

namespace MultiplayerMenuStyle
{
	const FLinearColor Row(0.075f, 0.078f, 0.074f, 1.f);
	const FLinearColor Text(0.82f, 0.80f, 0.75f, 1.f);
	const FLinearColor Accent(0.49f, 0.60f, 0.27f, 1.f);
}

void UMultiplayerSessionRowButton::InitializeRow(UMultiplayerMenuWidget* InOwner, int32 InSessionIndex)
{
	OwnerWidget = InOwner;
	SessionIndex = InSessionIndex;
	OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}

void UMultiplayerSessionRowButton::HandleClicked()
{
	if (OwnerWidget)
	{
		OwnerWidget->SelectSession(SessionIndex);
	}
}

void UMultiplayerMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RefreshButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRefreshClicked);
	JoinButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleJoinClicked);
	HostButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleHostClicked);
	JoinButton->SetIsEnabled(false);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		SessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionSubsystem>();
	}
	if (SessionSubsystem)
	{
		SessionSubsystem->OnCreateSessionResult.AddUniqueDynamic(this, &ThisClass::HandleCreateResult);
		SessionSubsystem->OnFindSessionsResult.AddUniqueDynamic(this, &ThisClass::HandleFindResult);
		SessionSubsystem->OnJoinSessionResult.AddUniqueDynamic(this, &ThisClass::HandleJoinResult);
		SessionSubsystem->OnConnectionFailure.AddUniqueDynamic(this, &ThisClass::HandleConnectionFailure);
		HandleRefreshClicked();
	}
}

void UMultiplayerMenuWidget::NativeDestruct()
{
	if (SessionSubsystem)
	{
		SessionSubsystem->OnCreateSessionResult.RemoveDynamic(this, &ThisClass::HandleCreateResult);
		SessionSubsystem->OnFindSessionsResult.RemoveDynamic(this, &ThisClass::HandleFindResult);
		SessionSubsystem->OnJoinSessionResult.RemoveDynamic(this, &ThisClass::HandleJoinResult);
		SessionSubsystem->OnConnectionFailure.RemoveDynamic(this, &ThisClass::HandleConnectionFailure);
	}
	Super::NativeDestruct();
}

void UMultiplayerMenuWidget::HandleRefreshClicked()
{
	if (SessionSubsystem && !bBusy)
	{
		SetBusy(true, FText::FromString(TEXT("Searching LAN servers...")));
		SessionSubsystem->FindSessions(100);
	}
}

void UMultiplayerMenuWidget::HandleJoinClicked()
{
	if (SessionSubsystem && !bBusy && SelectedSessionIndex != INDEX_NONE)
	{
		SetBusy(true, FText::FromString(TEXT("Joining server...")));
		SessionSubsystem->JoinSession(SelectedSessionIndex);
	}
}

void UMultiplayerMenuWidget::HandleHostClicked()
{
	if (!SessionSubsystem || bBusy || !ServerNameInput || !MaxPlayersInput)
	{
		return;
	}
	const FString ServerName = ServerNameInput->GetText().ToString().TrimStartAndEnd();
	if (ServerName.IsEmpty())
	{
		StatusText->SetText(FText::FromString(TEXT("Enter a server name.")));
		return;
	}
	SetBusy(true, FText::FromString(TEXT("Creating server...")));
	SessionSubsystem->CreateSession(FMath::RoundToInt(MaxPlayersInput->GetValue()), ServerName);
}

void UMultiplayerMenuWidget::HandleCreateResult(bool bSuccess)
{
	SetBusy(false, FText::FromString(bSuccess ? TEXT("Server created. Travelling...") : TEXT("Could not create server.")));
}

void UMultiplayerMenuWidget::HandleFindResult(bool bSuccess, const TArray<FSessionListItem>& Sessions)
{
	FoundSessions = Sessions;
	SelectedSessionIndex = INDEX_NONE;
	BuildSessionRows();
	SetBusy(false, FText::FromString(!bSuccess ? TEXT("Server search failed.") : Sessions.IsEmpty() ? TEXT("No LAN servers found.") : *FString::Printf(TEXT("%d servers found."), Sessions.Num())));
}

void UMultiplayerMenuWidget::HandleJoinResult(bool bSuccess)
{
	SetBusy(false, FText::FromString(bSuccess ? TEXT("Connected. Travelling...") : TEXT("Could not join server.")));
}

void UMultiplayerMenuWidget::HandleConnectionFailure(const FString& ErrorMessage)
{
	SetBusy(false, FText::FromString(FString::Printf(TEXT("Connection lost: %s"), *ErrorMessage)));
}

void UMultiplayerMenuWidget::SelectSession(int32 SessionIndex)
{
	SelectedSessionIndex = SessionIndex;
	if (JoinButton)
	{
		JoinButton->SetIsEnabled(!bBusy && SessionIndex != INDEX_NONE);
	}
	if (StatusText && FoundSessions.IsValidIndex(SessionIndex))
	{
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("Selected: %s"), *FoundSessions[SessionIndex].ServerName)));
	}
}

void UMultiplayerMenuWidget::BuildSessionRows()
{
	if (!SessionList)
	{
		return;
	}
	SessionList->ClearChildren();
	for (const FSessionListItem& Item : FoundSessions)
	{
		UMultiplayerSessionRowButton* RowButton = WidgetTree->ConstructWidget<UMultiplayerSessionRowButton>();
		RowButton->InitializeRow(this, Item.SearchResultIndex);
		RowButton->SetBackgroundColor(MultiplayerMenuStyle::Row);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		RowButton->AddChild(Row);
		UTextBlock* Name = MakeText(FText::FromString(Item.ServerName), 16, MultiplayerMenuStyle::Text);
		Row->AddChildToHorizontalBox(Name)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UTextBlock* Players = MakeText(FText::FromString(FString::Printf(TEXT("%d / %d"), Item.CurrentPlayers, Item.MaxPlayers)), 14, MultiplayerMenuStyle::Text);
		Players->SetMinDesiredWidth(110.f);
		Row->AddChildToHorizontalBox(Players);
		UTextBlock* Ping = MakeText(FText::FromString(FString::Printf(TEXT("%d ms"), Item.Ping)), 14, MultiplayerMenuStyle::Accent);
		Ping->SetMinDesiredWidth(80.f);
		Row->AddChildToHorizontalBox(Ping);
		SessionList->AddChild(RowButton);
	}
}

void UMultiplayerMenuWidget::SetBusy(bool bInBusy, const FText& Message)
{
	bBusy = bInBusy;
	if (StatusText) StatusText->SetText(Message);
	if (RefreshButton) RefreshButton->SetIsEnabled(!bBusy);
	if (HostButton) HostButton->SetIsEnabled(!bBusy);
	if (JoinButton) JoinButton->SetIsEnabled(!bBusy && SelectedSessionIndex != INDEX_NONE);
}

UTextBlock* UMultiplayerMenuWidget::MakeText(const FText& Text, int32 Size, const FLinearColor& Color)
{
	UTextBlock* Result = WidgetTree->ConstructWidget<UTextBlock>();
	Result->SetText(Text);
	Result->SetColorAndOpacity(FSlateColor(Color));
	Result->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), Size));
	return Result;
}
