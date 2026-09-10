/// 최초작성 : 2026.09.08
/// 작 성 자 : 최 요 환
/// 간단설명 : 아이템 획득 알림 스택의 행 하나(러스트 스타일)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PickupNotificationWidget.generated.h"

class UItemDataBase;
class UTextBlock;
class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickupNotificationExpired, UPickupNotificationWidget*, Notification);

/**
 * 획득 알림 스택의 행 하나 — "[아이콘] 아이템 이름   +획득량 (누적 총량)".
 *
 * 같은 아이템을 연달아 얻어도 기존 행과 합치지 않고 매번 새 행을 하나씩 쌓는다(실제 러스트
 * 동작 확인 후 확정 — 합치는 방식은 "이미 떠있는 행 중 같은 아이템을 찾아 갱신+타이머 리셋"
 * 이라는 추가 로직이 필요해서 이쪽보다 복잡하다).
 *
 * InitializeNotification()가 호출되면 (Lifetime - FadeOutDuration) 시점까지는 그대로 있다가, 남은
 * FadeOutDuration 동안 RenderOpacity를 1→0으로 매 틱 보간해 서서히 투명해지며 사라진다.
 * Lifetime에 도달하면(오파시티 0) 스스로 OnExpired를 브로드캐스트만 하고, 실제로 화면 패널에서
 * 빼는 건 소유자(UMainHUDWidget)의 몫이다 — 이 위젯 자신은 자기가 어떤 컨테이너/배열에 들어있는지
 * 전혀 몰라도 되게 하기 위한 설계. WBP 애니메이션(UWidgetAnimation) 없이 코드만으로 페이드를
 * 처리하므로 WBP 쪽엔 페이드용 애님을 따로 만들 필요가 없다.
 *
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다(전부 BindWidgetOptional):
 * - IconImage  : 아이콘. 아이템별 아이콘(ItemData->Icon)이 아니라 CommonPickupIcon 하나를
 *                모든 알림에 공통으로 사용한다("무엇을 얻었는지"는 이름 텍스트로 충분히
 *                구분되므로, 아이콘은 종류 상관없이 "뭔가 획득함"을 뜻하는 통일된 이미지로 고정).
 * - NameText   : 아이템 이름.
 * - AmountText : "+획득량 (누적 총량)" 텍스트.
 */
UCLASS()
class R1_API UPickupNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 아이콘/텍스트를 채우고 페이드 카운트다운을 시작한다(0으로 두면 그 항목은 즉시로 취급).
	void InitializeNotification(UItemDataBase* ItemData, int32 GainedAmount, int32 NewTotalCount, float InLifetime, float InFadeOutDuration);

	// Lifetime에 도달하면(페이드 완료) 브로드캐스트 — UMainHUDWidget이 구독해서 실제 제거를 처리한다.
	UPROPERTY(BlueprintAssignable, Category = "Notification")
	FOnPickupNotificationExpired OnExpired;

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~ End UUserWidget Interface

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmountText;

	// 아이템별 아이콘 대신 모든 획득 알림에 공통으로 쓸 아이콘. WBP_PickupNotification의
	// 클래스 디폴트에서 지정한다 — 비어있으면 IconImage를 숨긴다(기존 "아이콘 없는 아이템" 처리와 동일).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification")
	TSoftObjectPtr<UTexture2D> CommonPickupIcon;

private:
	float Lifetime = 0.f;
	float FadeOutDuration = 0.f;
	float ElapsedTime = 0.f;
	bool bHasExpired = false;
};
