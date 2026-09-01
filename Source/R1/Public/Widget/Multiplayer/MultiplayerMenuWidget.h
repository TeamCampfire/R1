#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Multiplayer/MultiplayerSessionSubsystem.h"
#include "MultiplayerMenuWidget.generated.h"

class UEditableTextBox;
class UScrollBox;
class USpinBox;
class UTextBlock;

UCLASS()
class R1_API UMultiplayerSessionRowButton : public UButton
{
	GENERATED_BODY()

public:
	void InitializeRow(class UMultiplayerMenuWidget* InOwner, int32 InSessionIndex);

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	TObjectPtr<UMultiplayerMenuWidget> OwnerWidget;

	int32 SessionIndex = INDEX_NONE;
};

/** Rust의 서버 브라우저 흐름을 따르는 멀티플레이 메뉴. 네트워크 작업은 세션 서브시스템에 위임한다. */
UCLASS(Blueprintable)
class R1_API UMultiplayerMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SelectSession(int32 SessionIndex);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleJoinClicked();

	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleCreateResult(bool bSuccess);

	UFUNCTION()
	void HandleFindResult(bool bSuccess, const TArray<FSessionListItem>& Sessions);

	UFUNCTION()
	void HandleJoinResult(bool bSuccess);

	UFUNCTION()
	void HandleConnectionFailure(const FString& ErrorMessage);

	void BuildSessionRows();
	void SetBusy(bool bInBusy, const FText& Message);
	UTextBlock* MakeText(const FText& Text, int32 Size, const FLinearColor& Color);

	UPROPERTY()
	TObjectPtr<UMultiplayerSessionSubsystem> SessionSubsystem;

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
	TObjectPtr<USpinBox> MaxPlayersInput;

	UPROPERTY()
	TArray<FSessionListItem> FoundSessions;

	int32 SelectedSessionIndex = INDEX_NONE;
	bool bBusy = false;
};
