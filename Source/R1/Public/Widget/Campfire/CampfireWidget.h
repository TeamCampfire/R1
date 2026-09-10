#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonHeader/CampfireTypes.h"
#include "CampfireWidget.generated.h"

class ACampfire;
class UCampfireComponent;
class UCampfireSlotWidget;
class UProgressBar;
class UButton;
class UTextBlock;

// 모닥불 슬롯과 진행도를 표시하며 사용자 조작은 컨트롤러의 서버 RPC로 전달
UCLASS()
class R1_API UCampfireWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 모닥불 상태 갱신 관련 델리게이트 구독
	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void BindCampfire(ACampfire* InCampfire);

	// 모닥불 상태 갱신 관련 델리게이트 해제
	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void UnbindCampfire();

protected:

	// 위젯 관련 델리게이트 구독, 슬롯 초기화
	virtual void NativeOnInitialized() override;

	// 위젯 관련 델리게이트들은 위젯과 함께 제거되기 때문에 모닥불 델리게이트 해제만 호출
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCampfireSlotWidget> FuelSlot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCampfireSlotWidget> InputSlot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCampfireSlotWidget> FuelOutputSlot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCampfireSlotWidget> CookingOutputSlot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> CookingProgressBar;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> FuelProgressBar;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ToggleFireButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ToggleFireText;

private:

	// 모닥불 위젯 새로고침
	UFUNCTION()
	void Refresh();

	// 모닥불 켜고 끄기 버튼
	UFUNCTION()
	void HandleToggleFire();

	UFUNCTION()
	void HandleSlotRightClicked(ACampfire* Campfire, FCampfireSlotRef CampfireSlotRef);

	UFUNCTION()
	void HandleInventoryDropped(ACampfire* Campfire, FInventorySlotRef From,FCampfireSlotRef To, int32 Count, bool bHalfSplit);

	UFUNCTION()
	void HandleCampfireDropped(ACampfire* Campfire, FCampfireSlotRef From, FCampfireSlotRef To, int32 Count, bool bHalfSplit);

	// 각 슬롯 초기화 및 델리게이트 구독
	void InitializeSlots();

	// 현재 참조 중인 모닥불
	TWeakObjectPtr<ACampfire> BoundCampfire;
};
