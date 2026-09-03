#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Multiplayer/MultiplayerSessionSubsystem.h"
#include "MultiplayerMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UMultiplayerSessionRowWidget;
class UScrollBox;
class UTextBlock;

/* 연결된 네트워크 작업은 SessionSubsystem에 구현 */
UCLASS(Blueprintable)
class R1_API UMultiplayerMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMultiplayerMenuWidget(const FObjectInitializer& ObjectInitializer);

	// 호스트 인원 선택에 사용하는 단일 범위 정의
	// UI와 세션 생성 요청이 같은 값을 공유
	static constexpr int32 MinAllowedPlayers = 1;
	static constexpr int32 MaxAllowedPlayers = 4;
	static constexpr int32 DefaultMaxPlayers = MaxAllowedPlayers;

	// 세션 행이 클릭되었을 때 참가 대상으로 사용할 검색 결과를 선택
	void SelectSession(int32 SessionIndex);

	UFUNCTION()
	void RefreshMenuState();

	UFUNCTION()
	void RefreshSessions();

protected:

	// 위젯 인스턴스당 한 번만 호출
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:

	// 위젯 UI 기능 처리 함수

	// 세션 나가기
	UFUNCTION()
	void HandleLeaveSessionClicked();

	// LeaveSession 버튼 보이기/숨기기 조정
	UFUNCTION()
	void UpdateLeaveSessionVisibility();

	// 게임 종료
	UFUNCTION()
	void HandleExitGameClicked();

	// 설정 UI
	UFUNCTION()
	void HandleOptionClicked();

	// 세션 목록 새로고침
	UFUNCTION()
	void HandleRefreshClicked();

	// 선택 세션 참가
	UFUNCTION()
	void HandleJoinClicked();

	// 선택 세션 참가 가능 여부 판단
	UFUNCTION()
	bool CanJoinSelectedSession() const;

	// 세션 호스트
	UFUNCTION()
	void HandleHostClicked();

	// 세션 인원 수용량 감소
	UFUNCTION()
	void HandleDecreaseMaxPlayersClicked();

	// 세션 인원 수용량 증가
	UFUNCTION()
	void HandleIncreaseMaxPlayersClicked();

	UFUNCTION()
	void HandleCreateResult(bool bSuccess);

	UFUNCTION()
	void HandleFindResult(bool bSuccess, const TArray<FSessionListItem>& Sessions);

	UFUNCTION()
	void HandleJoinResult(bool bSuccess);

	UFUNCTION()
	void HandleDestroyResult(bool bSuccess, bool bWasHost);

	UFUNCTION()
	void HandleConnectionFailure(const FString& ErrorMessage);

	void BuildSessionRows();
	void SetBusy(bool bInBusy, const FText& Message);	// 비동기 세션 요청 중 중복 요청과 인원 변경 차단
	void UpdateMaxPlayersDisplay();

private :

	// SessionSubsystem을 캐시해 모든 네트워크 작업을 위임
	UPROPERTY()
	TObjectPtr<UMultiplayerSessionSubsystem> SessionSubsystem;

	// 위젯 UI 변수 연결

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> LeaveSessionButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> ExitGameButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> OptionButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UScrollBox> SessionList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> JoinButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UEditableTextBox> ServerNameInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> DecreaseMaxPlayersButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> MaxPlayersText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> IncreaseMaxPlayersButton;

	/** 서버 목록의 개별 행에 사용할 위젯 클래스. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Multiplayer|Sessions", meta = (AllowPrivateAccess = "true"))
	TSoftClassPtr<UMultiplayerSessionRowWidget> SessionRowWidgetClass;

	/** 호스트가 생성할 방의 최대 인원 */
	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer|Host", meta = (AllowPrivateAccess = "true"))
	int32 MaxPlayers = DefaultMaxPlayers;

	// 찾은 세션들을 배열로 저장
	UPROPERTY()
	TArray<FSessionListItem> FoundSessions;

	// INDEX_NONE은 아직 참가할 행을 선택하지 않았음을 뜻한다.
	int32 SelectedSessionIndex = INDEX_NONE;
	bool bBusy = false;
};
