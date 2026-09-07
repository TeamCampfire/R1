/// 최초작성 : 2026.08.31
/// 작 성 자 : 최 요 환
/// 간단설명 : 메인 UI용 위젯 클래스
/// 
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class UCanvasPanel;
class UInteractionPromptWidget;
class UInventoryWidget;
class UBeltBarWidget;
class UDeathScreenOverlayWidget;
class UWarehouseWidget;
class UWarehouseInventoryComponent;
class AActionPlayerController;
class UCampfireWidget;
class ACampfireActor;
/**
 * 
 */
UCLASS()
class R1_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 인벤토리 패널(장비+메인)을 열려있으면 닫고, 닫혀있으면 연다. 전환 후 열림 상태를 돌려준다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ToggleInventoryPanel();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsInventoryPanelOpen() const;

	// 창고 패널을 연다 — 대상 창고의 UWarehouseInventoryComponent를 넘겨 WarehouseWidget에
	// 바인딩한다(플레이어마다, 그리고 창고마다 매번 다른 대상을 열 수 있으므로 인벤토리 패널과
	// 달리 항상 바인딩 대상을 새로 지정해야 한다). AActionPlayerController::Client_OpenWarehouse가 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Warehouse")
	void OpenWarehousePanel(UWarehouseInventoryComponent* Warehouse);

	// 창고 패널을 닫는다 — WBP_Warehouse의 닫기 버튼이 UWarehouseWidget::CloseWarehouse를 통해
	// 호출하므로 보통 직접 부를 일은 없지만, 외부(예: 상호작용 대상이 바뀌었을 때)에서 강제로
	// 닫아야 할 경우를 위해 공개해둔다.
	UFUNCTION(BlueprintCallable, Category = "Warehouse")
	void CloseWarehousePanel();

	UFUNCTION(BlueprintPure, Category = "Warehouse")
	bool IsWarehousePanelOpen() const;

	// AActionPlayerController::Client_OpenWarehouse_Implementation이 "지금 열려있는 창고가
	// 방금 상호작용한 그 창고와 같은지" 판단해 재상호작용 시 닫을지 결정하는 데 쓴다.
	UFUNCTION(BlueprintPure, Category = "Warehouse")
	UWarehouseInventoryComponent* GetOpenWarehouse() const;

	// 모닥불 열기
	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void OpenCampfire(ACampfireActor* Campfire);

	// 모닥불 닫기
	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void CloseCampfire();

	// 건축 설치 실패 메시지를 화면에 표시하는 함수
	// 같은 메시지를 연속으로 요청하면 기존 타이머를 초기화하여 마지막 요청 시점부터 DisplayDuration 동안 다시 표시
	void ShowBuildingPlacementMessage(const FText& Message, float DisplayDuration = 1.5f);

	// 건물 내구도 UI 표시하는 함수
	UFUNCTION(BlueprintCallable, Category = "Building|Durability")
	void ShowBuildingDurability(float CurrentDurability, float MaxDurability);

	// 건물 내구도 UI 숨기는 함수
	UFUNCTION(BlueprintCallable, Category = "Building|Durability")
	void HideBuildingDurability();

public:
	// 플레이어 컨트롤러 캐싱
	UPROPERTY()
	TObjectPtr<AActionPlayerController> CachedController;

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~ End UUserWidget Interface

	// 창고 패널이 열려있는 동안 매 틱 거리 체크 — 플레이어가 창고의 MaxInteractDistance보다
	// 멀어지면 자동으로 닫는다(월드를 돌아다니며 조작하지 못하게 막는 InventoryComponent 서버
	// 검증과는 별개로, UI 자체도 따라와서 계속 열려있는 게 부자연스러워서 클라이언트에서 처리).
	void CheckWarehouseAutoClose();

	// 현재 표시 중인 건축 안내 메시지를 숨기는 함수
	void HideBuildingPlacementMessage();

	// 사망창 띄우기
	void ShowDeathScreen();
	// 사망창 숨기기
	void HideDeathScreen();

	UFUNCTION()
	void OnPossessedCharChange();
	UFUNCTION()
	void OnDeath();
	UFUNCTION()
	void OnRespawnClicked();


	void BindDelegatesToNewChar();
	// 설정, 사망화면 등을 제외한 모든 HUD 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> HUDPanel;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UInteractionPromptWidget> InteractionPromptWidget;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UInventoryWidget> InventoryWidget;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCampfireWidget> CampfireWidget;

	bool bCampfireSessionOpen = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBeltBarWidget> BeltBarWidget;

	// 창고 패널 — WBP_MainHUD에 이 이름 + UWarehouseWidget 타입으로 배치하면 자동 바인딩된다.
	// 인벤토리/벨트와 달리 기본적으로 숨겨진 채 시작해서 상호작용으로 열 때만 보인다.
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWarehouseWidget> WarehouseWidget;

	// 건축 메시지 관련
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UBorder> Border_BuildingPlacementMessage; // 건축 설치 실패 메시지 전체 배경

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UTextBlock> Text_BuildingPlacementMessage; // 실제 건축 설치 실패 문구를 표시하는 텍스트

	FTimerHandle BuildingPlacementMessageTimerHandle; // 건축 안내 메시지를 자동으로 숨기는 타이머
	// 연속 클릭 시 기존 타이머를 취소하고 다시 시작

	// OpenWarehousePanel이 호출될 때 InventoryWidget이 이미 열려있지 않아서 대신 열어준 경우 true —
	// CloseWarehousePanel이 이 값을 보고 인벤토리도 같이 닫을지(true) 그대로 둘지(false, 플레이어가
	// 직접 인벤토리를 열어둔 상태였던 경우) 판단한다.
	bool bInventoryAutoOpenedForWarehouse = false;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_BuildingPlacementMessage; // WBP_MainHUD에서 만든 설치 실패 메시지 페이드 애니메이션

	// 사망 화면
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UDeathScreenOverlayWidget> DeathScreenOverlay;

	// BuildingDurability 건물 내구도 UI
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UBuildingDurabilityWidget> BuildingDurabilityWidget;
};
