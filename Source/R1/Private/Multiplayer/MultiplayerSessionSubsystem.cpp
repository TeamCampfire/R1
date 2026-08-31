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
	if (!SessionInterface.IsValid() ||
		!SessionSearch.IsValid() ||
		!SessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession failed (session index: %d"), SessionIndex);
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	JoinSessionDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	if (!SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SessionIndex]))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);

		OnJoinSessionResult.Broadcast(false);
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
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession failed"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("No World"));
		return;
	}

	World->ServerTravel(TEXT("/Game/Maps/Lv01_PlayerMoveTest?listen"));
}

void UMultiplayerSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions failed"));
		OnFindSessionsResult.Broadcast(false);
		return;
	}

	for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
	{
		const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[i];

		FString ServerName;

		Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);

		const int32 MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;

		const int32 CurrentPlayers = MaxPlayers - Result.Session.NumOpenPublicConnections;

		const int32 Ping = Result.PingInMs;

		UE_LOG(LogTemp, Log, TEXT("%s: %d/%d Ping=%d"), *ServerName, CurrentPlayers, MaxPlayers, Ping);
	}

	OnFindSessionsResult.Broadcast(true);
}

void UMultiplayerSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSessionResult Failed"));
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	FString ConnectString;

	// 검색된 세션의 실제 접속 정보 얻기
	// IP를 UI에서 다룰 필요 X
	if (!SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("ConnectString Failed"));
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();

	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("Playercontroller Failed"));
		OnJoinSessionResult.Broadcast(false);
		return;
	}

	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);

	OnJoinSessionResult.Broadcast(true);
}
