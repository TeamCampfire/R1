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
	int32 SearchResultIndex = INDEX_NONE;	// 세션 찾기 결과에 대한 일시적 방 인덱스
};

/*
* 세션 처리 결과를 UI나 블루프린트에 전달하기 위한 이벤트
* Online Subsystem 자체의 Complete 델리게이트와 다름
*/
// 세션 생성 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateSessionResult, bool, bSuccess);
// 세션 찾기 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindSessionsResult, bool, bSuccess, const TArray<FSessionListItem>&, Sessions);
// 세션 참가 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinSessionResult, bool, bSuccess);
// 세션 삭제 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDestroySessionResult, bool, bSuccess, bool, bWasHost);
// 네트워크 실패 전달용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionFailure, const FString&, ErrorMessage);

UCLASS()
class R1_API UMultiplayerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMultiplayerSessionSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 세션에 참가 중인지 확인
	UFUNCTION(BlueprintCallable)
	bool IsInSession() const;

	// 호스트인지 확인
	UFUNCTION(BlueprintCallable)
	bool IsHostingSession() const;

	// 세션 생성
	UFUNCTION(BlueprintCallable)
	void CreateSession(int32 NumPublicConnections, const FString& ServerName);

	// 세션 찾기
	UFUNCTION(BlueprintCallable)
	void FindSessions(int32 MaxSearchResults);

	// 세션 참가
	UFUNCTION(BlueprintCallable)
	void JoinSession(int32 SessionIndex);

	// 세션 삭제
	UFUNCTION(BlueprintCallable)
	void DestroySession();

	// 세션 떠나기 (클라이언트)
	UFUNCTION(BlueprintCallable)
	void LeaveSession();

	// 호스트가 방 운영 종료를 위한 작업 처리
	UFUNCTION(BlueprintCallable)
	void EndHostedSession();

private:

	/* Online Subsystem의 비동기 요청이 끝났을 때 호출되는 Callback 함수들 */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// GEngine 소유의 연결 실패 델리게이트 Callback 함수
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	// Main Menu로 이동하는 함수
	void TravelToMainMenu();

	/*
	* Online Session Interface에 등록했던 세션 Complete 델리게이트들 해제
	* Deinitialize()에서 호출
	* 비동기 작업 도중 비정상 종료(PIE 종료 또는 GameInstance 종료)가 발생하여
	* Deinitialize가 호출될 수 있으므로 남아있는 바인딩을 여기서 일괄 정리
	*/
	void ClearOnlineCompleteDelegateHandles();

	// 세션 광고에 저장할 사용자 표시용 방 이름의 Custom Setting Key
	// CreateSession에서 값을 저장하고, FindSessions 결과에서 같은 Key로 읽음
	// 실제 방 이름은 이 Key의 Value로 저장됨
	static const FName ServerNameSettingKey;

public:

	UPROPERTY(BlueprintAssignable)
	FOnCreateSessionResult OnCreateSessionResult;	// 세션 생성 결과 전달용 델리게이트

	UPROPERTY(BlueprintAssignable)
	FOnFindSessionsResult OnFindSessionsResult;		// 세션 찾기 결과 전달용 델리게이트

	UPROPERTY(BlueprintAssignable)
	FOnJoinSessionResult OnJoinSessionResult;		// 세션 참가 결과 전달용 델리게이트

	UPROPERTY(BlueprintAssignable)
	FOnDestroySessionResult OnDestroySessionResult;	// 세션 삭제 결과 전달용 델리게이트

	UPROPERTY(BlueprintAssignable)
	FOnConnectionFailure OnConnectionFailure;		// 네트워크 실패 전달용 델리게이트

private :

	/*
	* 현재 활성 Online Subsystem이 제공하는 세션 API
	* 프로젝트 설정에서 OnlineSubsystemNull 기반 LAN 구조를 선택했으므로
	* DefaultPlatformService=Null에 해당하는 Session Interface를 사용
	*/
	// IOnlineSessionPtr: Online Subsystem에서 정의한 Shared Pointer의 별칭
	IOnlineSessionPtr SessionInterface;

	// FindSessions는 비동기로 끝나므로 완료 시점까지 검색 객체가 살아 있어야 함
	TSharedPtr<FOnlineSessionSearch> SessionSearch;


	// ~CompleteDelegate: 비동기 처리가 완료됐을 때 실행할 콜백 함수를 담음
	// FDelegateHandle: 콜백을 실제 델리게이트 목록에 등록한 결과로 받은 식별자
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionDelegateHandle;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsDelegateHandle;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionDelegateHandle;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionDelegateHandle;

	// GEngine->OnNetworkFailure()에 등록한 콜백을 나중에 제거하기 위한 식별자
	FDelegateHandle NetworkFailureDelegateHandle;

	// 같은 비동기 요청이 중복 등록되어 Delegate Handle이 유실되는 것 방지
	bool bCreateInProgress = false;
	bool bFindInProgress = false;
	bool bJoinInProgress = false;
	bool bDestroyInProgress = false;

	// 기존 세션이 존재하는 상태에서 새 세션을 만들 때 세션 교체를 위한 Destroy라고 표시하기 위한 변수
	bool bCreateSessionAfterDestroy = false;

	// Destroy Complete 콜백에는 호스트 여부가 전달되지 않기 때문에
	// DestroySession 호출 시점의 호스트 여부를 비동기 Destroy 완료 Callback까지 보관
	bool bDestroyingHostedSession = false;

	// 기존 세션을 제거한 뒤 다시 CreateSession할 때 사용할 임시 입력값
	int32 PendingNumPublicConnections = 0;
	FString PendingServerName;

	// 온라인 세션의 광고 정원과 게임 서버의 로그인 정원에 동일한 값을 적용하기 위해 보관
	int32 HostedMaxPlayers = 0;
};
