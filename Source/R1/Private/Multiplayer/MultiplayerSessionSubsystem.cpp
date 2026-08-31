#include "Multiplayer/MultiplayerSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

UMultiplayerSessionSubsystem::UMultiplayerSessionSubsystem()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}

	CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnCreateSessionComplete);

	FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnFindSessionsComplete);

	JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnJoinSessionComplete);

}

void UMultiplayerSessionSubsystem::CreateSession(int32 NumPublicConnections, const FString& ServerName)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession failed"));
		OnCreateSessionResult.Broadcast(false);
		return;
	}

	FOnlineSessionSettings Settings;

	Settings.bIsLANMatch = true;

	Settings.NumPublicConnections = NumPublicConnections;

	Settings.bShouldAdvertise = true;

	Settings.bAllowJoinInProgress = true;

	Settings.bAllowJoinViaPresence = false;

	Settings.bAllowInvites = false;

	Settings.Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	if (!SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		OnCreateSessionResult.Broadcast(false);
	}

}

void UMultiplayerSessionSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions failed"));
		OnFindSessionsResult.Broadcast(false);
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();

	SessionSearch->MaxSearchResults = MaxSearchResults;

	SessionSearch->bIsLanQuery = true;

	FindSessionsDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);

		OnFindSessionsResult.Broadcast(false);
	}
}

void UMultiplayerSessionSubsystem::JoinSession(int32 SessionIndex)
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession failed (session index: %d"), SessionIndex);
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	
}

void UMultiplayerSessionSubsystem::DestroySession()
{
}

void UMultiplayerSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);

	OnCreateSessionResult.Broadcast(bWasSuccessful);

	if (!bWasSuccessful)
		return;

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("No World"));
		return;
	}

	//World->ServerTravel(TEXT("\Game\Maps\Lv01_PlayerMoveTest?listen"));
}

void UMultiplayerSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		
		return;
	}
}

void UMultiplayerSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
}
