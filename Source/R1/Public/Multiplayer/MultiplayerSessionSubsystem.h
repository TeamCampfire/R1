#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "MultiplayerSessionSubsystem.generated.h"

class UNetDriver;

/**
 * 세션 검색 결과를 UI가 바로 표시할 수 있도록 가공한 데이터
 * FOnlineSessionSearchResult 자체는 Blueprint에 그대로 노출하기 어렵기 때문에,
 * 화면에 필요한 값과 원본 검색 결과를 다시 찾기 위한 Index만 전달
 */
USTRUCT(BlueprintType)
struct FSessionListItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString ServerName;						// 서버 이름

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;				// 현재 참여 중인 플레이어 수

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;					// 방에 참가할 수 있는 최대 플레이어 수

	UPROPERTY(BlueprintReadOnly)
	int32 Ping = 0;							// 서버 핑

	UPROPERTY(BlueprintReadOnly)
	int32 SearchResultIndex = INDEX_NONE;	// 방 인덱스
};

/* 세션 처리 결과를 UI나 블루프린트에 전달하기 위한 이벤트 */
// 세션 생성 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateSessionResult, bool, bSuccess);
// 세션 찾기 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindSessionsResult, bool, bSuccess, const TArray<FSessionListItem>&, Sessions);
// 세션 참가 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinSessionResult, bool, bSuccess);
// 세션 삭제 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDestroySessionResult, bool, bSuccess, bool, bWasHost);
// 세션 연결 실패 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionFailure, const FString&, ErrorMessage);

UCLASS()
class R1_API UMultiplayerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMultiplayerSessionSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	void CreateSession(int32 NumPublicConnections, const FString& ServerName);

	UFUNCTION(BlueprintCallable)
	void FindSessions(int32 MaxSearchResults);

	UFUNCTION(BlueprintCallable)
	void JoinSession(int32 SessionIndex);

	UFUNCTION(BlueprintCallable)
	void DestroySession();

	UFUNCTION(BlueprintCallable)
	void LeaveSession();

	UFUNCTION(BlueprintCallable)
	void EndHostedSession();

	UPROPERTY(BlueprintAssignable)
	FOnCreateSessionResult OnCreateSessionResult;

	UPROPERTY(BlueprintAssignable)
	FOnFindSessionsResult OnFindSessionsResult;

	UPROPERTY(BlueprintAssignable)
	FOnJoinSessionResult OnJoinSessionResult;

	UPROPERTY(BlueprintAssignable)
	FOnDestroySessionResult OnDestroySessionResult;

	UPROPERTY(BlueprintAssignable)
	FOnConnectionFailure OnConnectionFailure;

private:

	/* Online Subsystem의 비동기 요청이 끝났을 때 호출되는 Callback 함수들 */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	void TravelToMainMenu();

	void ClearOnlineDelegateHandles();

	static const FName ServerNameSettingKey;

	// 현재 활성 Online Subsystem(이 프로젝트에서는 Null)이 제공하는 세션 API입니다.
	IOnlineSessionPtr SessionInterface;

	// FindSessions는 비동기로 끝나므로 완료 시점까지 검색 객체가 살아 있어야 합니다.
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionDelegateHandle;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsDelegateHandle;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionDelegateHandle;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionDelegateHandle;
	FDelegateHandle NetworkFailureDelegateHandle;

	// 같은 비동기 요청이 중복 등록되어 Delegate Handle이 유실되는 것을 막습니다.
	bool bCreateInProgress = false;
	bool bFindInProgress = false;
	bool bJoinInProgress = false;
	bool bDestroyInProgress = false;
	bool bCreateSessionAfterDestroy = false;
	bool bDestroyingHostedSession = false;

	// 기존 세션을 제거한 뒤 다시 CreateSession할 때 사용할 임시 입력값입니다.
	int32 PendingNumPublicConnections = 0;
	FString PendingServerName;
};
