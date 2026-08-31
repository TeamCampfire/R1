

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerSessionSubsystem.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnCreateSessionResult,
	bool,
	bSuccess
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnFindSessionsResult,
	bool,
	bSuccess
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnJoinSessionResult,
	bool,
	bSuccess
);

UCLASS()
class R1_API UMultiplayerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public :

	UMultiplayerSessionSubsystem();

public :

	void CreateSession(int32 NumPublicConnections, const FString& ServerName);

	void FindSessions(int32 MaxSearchResults);

	void JoinSession(int32 SessionIndex);

	void DestroySession();

public :

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FOnCreateSessionResult OnCreateSessionResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FOnFindSessionsResult OnFindSessionsResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FOnJoinSessionResult OnJoinSessionResult;

private :

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	void OnFindSessionsComplete(bool bWasSuccessful);

	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

private :

	IOnlineSessionPtr SessionInterface;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionDelegateHandle;

	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsDelegateHandle;
	
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionDelegateHandle;

};

USTRUCT(BlueprintType)
struct FSessionListItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString ServerName;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Ping = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 SearchResultIndex = INDEX_NONE;
};
