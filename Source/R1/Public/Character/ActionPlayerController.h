/// 최초작성 : 2026.08.25
/// 작 성 자 : 최 요 환

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "Interface/RespawnPointInterface.h"
#include "Component/InventoryComponent.h"
#include "Item/PlaceableItem/Campfire/CampfireTypes.h"
#include "ActionPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPossessedCharChange);

class UInputMappingContext;
class UInputAction;
struct FInventorySlotRef;
/**
 * 
 */
UCLASS()
class R1_API AActionPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	// 제작 대기열은 화면과 분리하여 창을 닫아도 유지한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<class UCraftingComponent> CraftingComponent;

	UPROPERTY()
	TObjectPtr<class UCraftingWidget> CraftingWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	TSubclassOf<class UCraftingWidget> CraftingWidgetClass;

	// Bench가 nullptr이면 기본 제작 화면을 연다.
	UFUNCTION(Client, Reliable)
	void Client_OpenCrafting(class AWorkbench* Bench);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void CloseCrafting();

	void ToggleCrafting();
	UFUNCTION(Client, Reliable)
	void Client_OpenCampfire(class ACampfire* Campfire);

	UFUNCTION(Server, Reliable)
	void Server_MoveInventoryToCampfire(ACampfire* Campfire, FInventorySlotRef From,
		FCampfireSlotRef To, int32 Count, bool bHalfSplit);

	UFUNCTION(Server, Reliable)
	void Server_MoveCampfireToInventory(ACampfire* Campfire, FCampfireSlotRef From,
		FInventorySlotRef To, int32 Count, bool bHalfSplit);

	UFUNCTION(Server, Reliable)
	void Server_QuickMoveInventoryToCampfire(ACampfire* Campfire, FInventorySlotRef From);

	UFUNCTION(Server, Reliable)
	void Server_QuickMoveCampfireToInventory(ACampfire* Campfire, FCampfireSlotRef From);

	UFUNCTION(Server, Reliable)
	void Server_SetCampfireLit(ACampfire* Campfire, bool bLit);

	AActionPlayerController();

protected:

	virtual void BeginPlay() override;

	// Called to bind functionality to input
	virtual void SetupInputComponent() override;


public:

	// 플레이어가 건축 좌키 눌렀을 때 컨트롤러로 넘어온 함수
	void OnConfirmBuildingPlacement();

	// 플레이어가 건축 파츠 회전을 위해 휠키 눌렀을 때 컨트롤러로 넘어온 함수
	void OnRotateBuildingPart();

	// BuildingPlacementComponent의 Start/Stop Placement 래핑 함수
	void OnStartPlacement(class UBuildingPartDefinition* Definition);
	void OnStopPlacement();

	// BuildingPlacementComponent의 Placeable 아이템 Start/Stop Placement 래핑 함수
	void OnStartPlaceablePlacement( class UPlaceableItemData* ItemData,
		const FInventorySlotRef& SourceSlot, const FGuid& SourceInstanceID);

	// 배치 모드가 활성화되어 있다면 종료하고 true를 반환해요
	bool TryCancelPlacement();

	// 배치 모드가 활성화 되어 있다면 설치를 요청하고 true를 반환해요
	bool TryConfirmPlacement();
	//  ===================================================================================
public:

	virtual void OnPossess(APawn* InPawn) override;
	UFUNCTION()
	void PossessChar(AActionCharacter* InNewChar);

public:

	// 인벤토리 패널이 열리면 마우스 커서를 보여주고 UI 입력을 받도록, 닫히면 다시 게임 전용 입력으로 되돌린다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventoryInputState(bool bOpen);
	// 옵션 패널이 열리면 마우스 커서를 보여주고 UI 입력을 받도록, 닫히면 다시 게임 전용 입력으로 되돌린다.
	// SetInventoryInputState와 동일한 카운터(OpenUIPanelCount)를 공유하므로, 인벤토리와 옵션을
	// 동시에 열어도 마지막 하나가 닫힐 때까지 게임 입력이 잘못 복구되지 않는다.
	UFUNCTION(BlueprintCallable, Category = "Options")
	void SetOptionsInputState(bool bOpen);

	// 창고 패널이 열리면 마우스 커서를 보여주고 UI 입력을 받도록, 닫히면 다시 게임 전용 입력으로
	// 되돌린다. SetInventoryInputState/SetOptionsInputState와 동일한 카운터를 공유한다.
	UFUNCTION(BlueprintCallable, Category = "Warehouse")
	void SetWarehouseInputState(bool bOpen);

	// 인벤토리/옵션/창고 등 UI 패널이 하나라도 열려있는지 — AActionCharacter가 이동은 계속 받고
	// 시야 회전/점프/공격 등 나머지 입력만 걸러내는 데 쓴다(ApplyUIInputState 참고).
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsAnyUIPanelOpen() const { return OpenUIPanelCount > 0; }

	// AWarehouseBase::Interact_Implementation(서버)이 상호작용을 요청한 클라이언트에게만
	// 창고 UI를 열라고 알리는 용도 — Interact 자체는 서버에서 실행되므로 클라이언트에 UI를
	// 열려면 이 Client RPC가 필요하다.
	UFUNCTION(Client, Reliable)
	void Client_OpenWarehouse(class UWarehouseInventoryComponent* Warehouse);

	// Temp: 없애도 될 거 같은데 !!!!!!!!!
	// GameMenu 패널이 열리면 마우스 커서를 보여주고 UI 입력을 받도록, 닫히면 다시 게임 전용 입력으로 되돌린다.
	UFUNCTION(BlueprintCallable, Category = "GameMenu")
	void SetGameMenuInputState(bool bOpen);

	// 리스폰 지점(액터) 설정 함수
	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void SetRespawnPoint(AActor* InRespawnPoint);
	// 리스폰 지점(액터) 반환 함수

	// 폰 변경 감지 함수
	virtual void OnRep_Pawn() override;

	// 서버에 스폰 요청 함수
	UFUNCTION(Server, Reliable)
	void RequestSpawn();

	// Debug---------------------------------------------------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Respawn")
	AActor* GetRespawnPoint();

protected:
	UFUNCTION(Server, Reliable)
	void ServerTestInflictDamage();
	// --------------------------------------------------------------------------------------------------------------------------
	
	UFUNCTION(Exec)
	void TestDamage(int32 PlayerIndex);
	UFUNCTION(Exec)
	void TestHydrationDamage(int32 PlayerIndex);

protected:

	// 기본 입력 맵핑 컨텍스트(캐릭터 조작) — UI가 열려있어도 이동(WASD)은 계속 받아야 해서
	// 더 이상 제거하지 않는다(ApplyUIInputState 참고). 대신 이동/상호작용을 제외한 나머지 액션은
	// 각 핸들러가 AActionCharacter::IsUIBlockingGameplayInput()으로 직접 걸러낸다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UBuildingPlacementComponent> BuildingPlacementComponent; // 건축물 설치 컴포넌트
	// UI 입력 맵핑 컨텍스트(인벤토리 토글 등) — BeginPlay에 한 번 추가되면 제거되지 않는다.
	// DefaultMappingContext를 뺐다 켰다 해도 이 토글 키만은 항상 눌리게 하기 위함.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> UIMappingContext = nullptr;

	// 게임 메뉴 토글
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_GameMenuToggle;

	void OnGameMenuTogglePressed();	// 게임 메뉴 토글

	// 인벤토리 토글 — 캐릭터가 아니라 컨트롤러에 바인딩해야 한다. 폰의 InputComponent는 그 폰이
	// Unpossess되면 같이 사라지므로, 캐릭터 쪽에 바인딩하면 나중에 탈것 등 다른 폰을 빙의한 동안
	// 인벤토리 토글 키가 아예 안 눌리게 된다. 컨트롤러는 빙의 대상이 바뀌어도 유지되므로 여기에
	// 두면 어떤 폰을 조종 중이든 항상 눌린다 — MainHUDWidget/InventoryWidget은 폰이 아니라
	// 컨트롤러에 종속돼 있고, 옛 캐릭터의 InventoryComponent 참조도 그대로 유효하게 남아있어서
	// (OnPossess/OnRep_Pawn이 AActionCharacter 캐스트에 실패하면 리바인딩을 안 하므로) 탈것을
	// 모는 동안에도 "내 캐릭터의 인벤토리"가 정확히 열린다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> IA_InventoryToggle;

	void OnInventoryTogglePressed();	// 인벤토리 패널 토글

	// SetInventoryInputState/SetOptionsInputState가 공유하는 "현재 열려있는 UI 패널 개수" —
	// 0→1로 바뀔 때만 게임 입력을 끄고, 1→0으로 바뀔 때만 게임 입력을 복구한다.
	void ApplyUIInputState(bool bOpen);
	int32 OpenUIPanelCount = 0;

	// 플레이어 리스폰 지점
	UPROPERTY()
	TObjectPtr<AActor> RespawnPoint;

public:

	// 컨트롤러 연결 캐릭터 변경 시 호출되는 델리게이트
	FOnPossessedCharChange OnPossessedCharChange;

private:
	UInventoryComponent* GetPlayerInventory() const;
	bool CanUseCampfire(ACampfire* Campfire) const;

	// 입력 우선 순위
	int32 GameInputPriority = 1;
	int32 UIInputPriority = 2;

	/* UI 사용 상태 */
	bool bInventoryOpen = false;
	bool bGameMenuOpen = false;
};
