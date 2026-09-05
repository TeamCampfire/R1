/// 최초작성 : 2026.09.02
/// 작 성 자 : 최 요 환
/// 간단설명 : 옵션 패널의 "단축키" 카테고리 — Player Mappable Key로 등록된 모든 매핑을 리스트로 보여주고 리바인딩/초기화한다.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UserSettings/EnhancedInputUserSettings.h" // EPlayerMappableKeySlot을 값으로 들고 있어서 전방선언으로는 부족하다.
#include "OptionsControlsWidget.generated.h"

class UPanelWidget;
class UButton;
class UKeyRebindRowWidget;
class UKeyConflictDialogWidget;

/**
 * WBP에서 아래 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다:
 * - KeyRowContainer : 리바인딩 행들을 담는 컨테이너(ScrollBox 추천).
 * - ResetButton     : 모든 리바인딩을 IMC 기본 키로 되돌린다.
 * - ConflictDialog  : WBP_KeyConflictDialog(UKeyConflictDialogWidget 부모) — 충돌 시 뜨는 모달.
 *                     패널 전체를 덮는 오버레이로 배치하고 기본 Visibility는 Collapsed로 둔다.
 *
 * 행 목록은 정적으로 만들어두지 않고, UEnhancedInputUserSettings에 "Player Mappable"로 등록된
 * 매핑을 그대로 순회해서 만든다 — 에디터에서 IMC 매핑에 체크만 추가하면 C++ 재작업 없이 이 목록에
 * 자동으로 나타난다.
 *
 * 리바인딩 적용/충돌 검사/행 단위 초기화는 전부 이 위젯이 처리한다 — 각 KeyRebindRowWidget은
 * 자신이 눌린 키만 RequestKeyRebind로 올려보내고, 다른 행과의 충돌 여부를 판단하려면 전체
 * 매핑을 알아야 하기 때문이다.
 */
UCLASS()
class R1_API UOptionsControlsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// KeyRebindRowWidget이 리스닝 중 새 키를 잡으면 호출한다. 다른 액션과 충돌하면
	// ConflictDialog를 띄우고, 아니면 바로 적용한다.
	void RequestKeyRebind(FName InMappingName, EPlayerMappableKeySlot InSlot, const FKey& NewKey);

	// KeyRebindRowWidget의 행별 초기화 버튼(호버 시에만 보임)이 호출한다 — 이 행 하나만 기본 키로.
	void RequestRowReset(FName InMappingName);

	// KeyRowContainer를 비우고 현재 등록된 모든 Player Mappable Key 매핑으로 다시 채운다.
	// public인 이유: 옵션 패널을 열 때마다(OptionsWidget::RefreshActiveCategory) 다시 호출해서,
	// 이 위젯이 최초 생성되던 시점에 아직 Enhanced Input 쪽 등록(RegisterInputMappingContext)이
	// 안 끝나 있었더라도 그 다음에 열 때는 최신 데이터로 다시 채워지게 한다.
	void RefreshRows();

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleConflictReplaceConfirmed();

	UFUNCTION()
	void HandleConflictCancelled();

	// 충돌 검사 없이 실제로 키를 매핑하고 저장 + 행 목록을 새로고침한다.
	void ApplyKeyMapping(FName InMappingName, EPlayerMappableKeySlot InSlot, const FKey& NewKey);

	UEnhancedInputUserSettings* GetUserSettings() const;
	UEnhancedPlayerMappableKeyProfile* GetActiveProfile() const;

	// 각 행으로 만들어질 위젯 클래스 — WBP_KeyRebindRow를 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Options")
	TSubclassOf<UKeyRebindRowWidget> KeyRebindRowClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> KeyRowContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UKeyConflictDialogWidget> ConflictDialog;

	// 충돌 다이얼로그가 떠 있는 동안 보류 중인 리바인딩 요청 — Replace를 누르면 이 값으로 적용한다.
	FName PendingMappingName;
	EPlayerMappableKeySlot PendingSlot = EPlayerMappableKeySlot::First;
	FKey PendingNewKey;

	// PendingMappingName이 리바인딩 전에 쓰고 있던 키 — Replace 시 충돌 상대에게 "교환"으로 넘겨준다.
	FKey PendingOldKey;

	// 보류 중인 요청과 충돌 중인, 기존에 그 키를 쓰고 있던 매핑 — Replace를 누르면 이 매핑이
	// PendingOldKey를 받는다(스왑). PendingOldKey가 없으면(원래 미지정 상태였으면) 그냥 비운다.
	FName PendingConflictMappingName;
	EPlayerMappableKeySlot PendingConflictSlot = EPlayerMappableKeySlot::First;
};
