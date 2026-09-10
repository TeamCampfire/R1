/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "SurvivalStatBarsWidget.generated.h"

class UParameterBarWidget;
class UParameterBarWidgetTest;
class UStatusBarWidget;
class UVerticalBox;
class UStatComponent;
class UPickupNotificationWidget;
class UItemDataBase;
class UPanelWidget;
enum class EStatusEffect:uint8;

/**
 *
 */
UCLASS()
class R1_API USurvivalStatBarsWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 상태이상 업데이트 함수
	UFUNCTION()
	void UpdateStatusEffects();

	// 스탯 UI 초기화 함수
	UFUNCTION()
	void InitializeSurvivalStatBars();

	// 스탯컴포넌트<->UI 델리게이트 구독 해제 함수
	void UnbindStatDelegates();
private:

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UStatusBarWidget> StatusBarWidgetClass;

	// 아이템 획득 알림을 스택 맨 위에 하나 추가한다(몇 초 뒤 자동 소멸) —
	// AActionPlayerController::Client_NotifyItemAcquired가 UMainHUDWidget::AddPickupNotification을
	// 거쳐 호출한다. Debuffs 세로박스와 같은 화면 영역에서 "디버프가 아래쪽, 알림이 그 위로 쌓이는"
	// 배치를 만들기 위해 컨테이너를 (MainHUDWidget이 아니라) 이 위젯이 직접 소유한다.
	UFUNCTION(BlueprintCallable, Category = "Notification")
	void AddPickupNotification(UItemDataBase* ItemData, int32 GainedAmount, int32 NewTotalCount);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UParameterBarWidget> HealthBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UParameterBarWidget> HydrationBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UParameterBarWidget> CaloriesBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> Debuffs;

	UPROPERTY()
	TMap<EStatusEffect, TObjectPtr<UStatusBarWidget>> StatusBarWidgets;

	UPROPERTY()
	TObjectPtr<UStatComponent> StatComp;

	// 아이템 획득 알림 스택이 쌓일 컨테이너 — WBP_SurvivalStatBars_New의 세로 박스 안, Debuffs와
	// 같은 위치(바로 위)에 이 이름 + 패널(VerticalBox 권장) 타입으로 배치하면 자동 바인딩된다.
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> PickupNotificationContainer;

	// 알림 행 하나를 표현할 위젯 클래스. WBP 디폴트에서 WBP_PickupNotification(UPickupNotificationWidget
	// 부모)으로 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification")
	TSubclassOf<UPickupNotificationWidget> PickupNotificationWidgetClass;

	// 알림 행 하나가 화면에 떠있는 시간(초) — 페이드아웃 구간도 포함한 총 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification")
	float PickupNotificationLifetime = 3.f;

	// 사라지기 전 서서히 투명해지는 구간의 길이(초). PickupNotificationLifetime 중 마지막
	// 이 시간만큼을 오파시티 1→0 페이드에 쓴다(WBP 애니메이션 불필요, 코드로만 처리).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification")
	float PickupNotificationFadeOutDuration = 0.75f;

	// 지금 떠있는 알림 행들 — [0]이 항상 최신(맨 위). RefreshPickupNotificationContainer가 이
	// 순서 그대로 컨테이너에 다시 채운다.
	UPROPERTY()
	TArray<TObjectPtr<UPickupNotificationWidget>> ActivePickupNotifications;

	// PickupNotificationWidget이 자기 수명(Lifetime)이 다 되어 OnExpired를 쏘면 호출된다 —
	// 배열에서 빼고 컨테이너를 다시 그린다.
	UFUNCTION()
	void RemovePickupNotification(UPickupNotificationWidget* Notification);

	// ActivePickupNotifications 순서(맨 앞이 최신)를 그대로 컨테이너에 다시 채운다.
	void RefreshPickupNotificationContainer();
};
