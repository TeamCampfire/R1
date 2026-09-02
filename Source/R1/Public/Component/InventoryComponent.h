/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 메인 인벤토리 컴포넌트 클래스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Item/ItemTypes.h"
#include "Item/ItemInstance.h"
#include "InventoryComponent.generated.h"

/**
 * 인벤토리 데이터 매니저.
 *
 * 장비(Equipment) / 메인(Main) / 벨트(Belt) 3개의 독립된 슬롯 풀을 갖는다 — 실제 Rust
 * 플레이 동작을 참고해 확정한 구조로, 벨트는 메인 슬롯을 가리키는 참조가 아니라
 * 진짜 별도 저장공간이다. 세 풀 사이의 이동/장착/해제는 전부 TransferItem() 하나로 처리한다.
 *
 * 무기(Weapon)/도구(Tool)는 장비슬롯을 쓰지 않는다 — 벨트 슬롯을 선택(UseBeltSlot)하면
 * 손에 들고, 같은 슬롯을 다시 선택하면 손에서 내리는 방식으로 "장착" 개념을 대신한다.
 */

class UItemDataBase;
class AItemPickup;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

// 슬롯 내용물과는 무관한, "지금 어느 슬롯을 선택(클릭)해서 보고 있는지"만 알리는 델리게이트.
// 인벤토리 배열이 바뀌지 않아도(클릭만 해도) 발생하므로 OnInventoryChanged와 분리했다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventorySelectionChanged);

/**
 * 인벤토리 슬롯 풀 종류. FInventorySlotRef가 이 값 + 인덱스로 슬롯 하나를 특정한다.
 */
UENUM(BlueprintType)
enum class EInventorySlotCategory : uint8
{
	Equipment,
	Main,
	Belt
};

/**
 * "어느 풀의 몇 번 슬롯인지"를 하나로 표현하는 참조.
 * Category == Equipment일 때 Index는 (int32)EEquipmentSlotType 값으로 해석한다.
 */
USTRUCT(BlueprintType)
struct FInventorySlotRef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	EInventorySlotCategory Category = EInventorySlotCategory::Main;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int32 Index = INDEX_NONE;
};

/**
 * TransferItem()의 결과 — 대상이 비어있어서 그냥 이동했는지/같은 아이템이라 합쳐졌는지/
 * 다른 아이템이라 자리를 바꿨는지/장비슬롯에 장착·해제됐는지를 구분해서 돌려준다.
 * 드래그앤드롭 UI에서 결과에 따라 다른 사운드/이펙트를 재생하고 싶을 때 이 값을 쓰면 된다.
 */
UENUM(BlueprintType)
enum class EMoveSlotResult : uint8
{
	Failed,
	Moved,
	Merged,
	Swapped,
	Equipped,
	Unequipped
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class R1_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInventoryComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ItemData/Count를 MainSlots에 넣는다(항상 메인 슬롯 우선). 스택 가능한 아이템은 같은 종류의
	// 기존 슬롯에 먼저 채우고, 남으면 빈 슬롯에 새로 놓는다. 다 못 넣으면 못 넣은 수량을
	// OutRemainder로 돌려준다(0이면 전부 성공, 반환값 true).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(UItemDataBase* ItemData, int32 Count, int32& OutRemainder);

	// 메인↔벨트 이동/병합/교환, (메인|벨트)→장비(부위 일치 시 자동 장착, 기존 장착품은
	// From 자리로 스왑), 장비→(메인|벨트)(해제)까지 전부 이 함수 하나로 처리한다.
	// Count가 0 이하이면 슬롯 전체를 이동시킨다 — 단, bAutoHalfSplitIfTargetEmpty가 true고
	// Count가 0 이하이며 대상이 빈 슬롯이고 원본이 2개 이상 쌓여있으면 절반만(내림) 옮긴다
	// (휠클릭 드래그로 스택형 아이템을 빈 슬롯에 놓았을 때의 "절반 분할" 용도).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	EMoveSlotResult TransferItem(FInventorySlotRef From, FInventorySlotRef To, int32 Count, bool bAutoHalfSplitIfTargetEmpty = false);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TransferItem(FInventorySlotRef From, FInventorySlotRef To, int32 Count, bool bAutoHalfSplitIfTargetEmpty);

	// 우클릭 "빠른 이동" — 대상 슬롯을 직접 고르지 않고 아이템 종류 + 어느 풀에서 눌렀는지로 알아서 보낼 곳을 정한다.
	//  - 메인   ┬ 장비 아이템 → 장착(같은 부위 착용중이면 교체)
	//           └ 그 외      → 빈 벨트 슬롯으로 이동(빈 칸 없으면 무동작)
	//  - 장비   → 빈 메인 슬롯으로 해제(빈 칸 없으면 무동작 — 다른 아이템과 교환하지 않는다)
	//  - 벨트   ┬ 장비 아이템 → 장착/교체(장비 ↔ 벨트)
	//           └ 그 외      → 빈 메인 슬롯으로 이동(빈 칸 없으면 무동작)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	EMoveSlotResult QuickMoveItem(const FInventorySlotRef& SlotRef);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_QuickMoveItem(FInventorySlotRef SlotRef);

	// 벨트 슬롯 "사용/선택" — 이동(TransferItem)과는 별개의 액션이다(예: 단축키 1~6).
	// 빈 슬롯이면: 지금 손에 든(HeldBeltIndex) 무기/도구가 있으면 그걸 내려놓고, 없으면 무동작.
	// Weapon/Tool은 HeldBeltIndex를 토글(선택 = 손에 듦, 같은 칸 재선택 = 손을 내림).
	// Equipment(의류)는 장비창에 장착/교체. Consumable은 즉시 효과 적용 후 1개 소모(0개가 되면
	// 슬롯을 비움). 그 외(Misc 등)는 무동작.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseBeltSlot(int32 BeltIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseBeltSlot(int32 BeltIndex);

	// 지정한 슬롯의 아이템을 사용한다. Consumable만 처리(1개 소모, 효과 적용은 StatComponent
	// 연동 후 TODO) — 그 외 카테고리는 무동작(장착/손에 들기는 QuickMoveItem/UseBeltSlot이 전담).
	// SelectedSlotRef는 서버로 리플리케이트되지 않는 순수 로컬 상태라, 서버가 뭘 써야 할지
	// 알려면 호출하는 쪽(클라이언트)이 슬롯을 직접 파라미터로 넘겨야 한다 — ThrowItem과 동일한 이유.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseSelectedItem(const FInventorySlotRef& SlotRef);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseSelectedItem(FInventorySlotRef SlotRef);

	// 지정한 슬롯에서 Count만큼(0 이하이면 전체) 빼서 DropTransform 위치에 AItemPickup으로 스폰한다.
	// ThrowImpulse가 0이 아니면 스폰 직후 그 방향/크기로 밀어준다(던지는 연출용).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DropItem(FInventorySlotRef Slot, int32 Count, const FTransform& DropTransform, const FVector& ThrowImpulse = FVector::ZeroVector);

	// 드래그로 인벤토리 밖(빈 공간)에 드롭했을 때 쓰는 진입점 — 소유자(캐릭터) 눈 위치에서
	// 바라보는 방향으로 조금 앞에 스폰하고, 같은 방향으로 살짝 던지듯 impulse를 준다.
	// 스폰 위치/방향 계산까지 여기서 전담하므로 UI 쪽은 어느 슬롯인지만 넘기면 된다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ThrowItem(FInventorySlotRef Slot, int32 Count = 0);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ThrowItem(FInventorySlotRef Slot, int32 Count);

	// 인벤토리 정보 Log 출력하는 테스트 함수
	void PrintInventoryInfo();

	// 슬롯 하나를 "선택"(클릭) 상태로 표시한다 — 장비/메인/벨트를 통틀어 항상 최대 1개만 선택된다.
	// 실제 이동(TransferItem)과는 무관한, 정보창 표시용 상태다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SelectSlot(const FInventorySlotRef& SlotRef);

	// 선택을 해제한다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearSelection();

	// 지정한 슬롯이 현재 선택된 슬롯인지.
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotSelected(const FInventorySlotRef& SlotRef) const;

	// 현재 선택된 슬롯의 아이템 정보. 선택이 없거나 그 슬롯이 비어있으면 무효(FItemInstance())를 반환한다.
	// 나중에 추가될 상단 중앙 "선택된 아이템 정보" 패널이 이 함수만 보고 그리면 된다.
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FItemInstance GetSelectedItemInstance() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	inline int32 GetCurrentHeledBeltIndex() const { return HeldBeltIndex; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//~ Begin UActorComponent Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent Interface

private:
	// 단일 관문 — Main/Belt/Equipment 중 어디를 건드리든 반드시 이걸 거친다.
	// 브로드캐스트가 누락되는 실수를 막고, 나중에 검증/리플리케이션 로직을 얹을 자리를 확보해둔다.
	void SetSlot(EInventorySlotCategory Category, int32 Index, const FItemInstance& NewValue);

	// Category에 해당하는 실제 배열 참조를 돌려준다(Main/Belt/Equipment 공통 처리용).
	TArray<FItemInstance>& GetSlotArray(EInventorySlotCategory Category);
	const TArray<FItemInstance>& GetSlotArray(EInventorySlotCategory Category) const;

	// TransferItem의 대상이 장비슬롯일 때 처리하는 부분만 분리 — 부위 일치 검사 + 스왑 로직.
	EMoveSlotResult EquipToSlot(const FInventorySlotRef& From, const FItemInstance& SourceInstance);

	UFUNCTION()
	void OnRep_Slots();

public:
	// 메인 슬롯 개수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 MainSlotCount = 24;

	// 벨트(퀵슬롯) 개수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 BeltSlotCount = 6;

	// 장비(착용) 칸 수 — 부위별로 고정 배정되지 않는 자유 슬롯 개수(실제 Rust 화면 기준).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 EquipmentSlotCount = 6;

	// ThrowItem으로 버릴 때 소유자 눈 위치에서 얼마나 앞(cm)에 스폰할지.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Drop")
	float ThrowSpawnDistance = 60.f;

	// ThrowItem으로 버릴 때 바라보는 방향으로 주는 impulse 크기.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Drop")
	float ThrowImpulseStrength = 350.f;

	// 메인 인벤토리 — 종류 무관하게 아무 아이템이나 들어갈 수 있는 일반 저장공간.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Slots, Category = "Inventory")
	TArray<FItemInstance> MainSlots;

	// 벨트(퀵슬롯) — 메인과 완전히 독립된 저장공간. 아이템 종류 제한 없음.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Slots, Category = "Inventory")
	TArray<FItemInstance> BeltSlots;

	// 장비(착용) 슬롯 — 크기 = EquipmentSlotCount. 부위(EquipSlot)가 특정 인덱스에 고정되지
	// 않는다 — 같은 부위는 한 벌만 착용 가능(EquipToSlot이 검사)하고, 빈 칸 아무데나 들어간다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Slots, Category = "Inventory")
	TArray<FItemInstance> EquipmentSlots;

	// 현재 손에 든 벨트 슬롯 인덱스(Weapon/Tool 아이템에만 의미가 있음). 없으면 INDEX_NONE.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Slots, Category = "Inventory")
	int32 HeldBeltIndex = INDEX_NONE;

	// 현재 선택(클릭)된 슬롯이 있는지. SelectSlot/ClearSelection으로만 바뀐다.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bHasSelection = false;

	// 선택된 슬롯 참조. bHasSelection이 false면 의미 없음.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventorySlotRef SelectedSlotRef;

	// 슬롯 변경 감지 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	// 선택 변경 감지 델리게이트(슬롯 내용물이 아니라 "무엇을 선택했는지"만 바뀔 때).
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventorySelectionChanged OnSelectionChanged;

};
