/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 레벨에 배치되거나 드랍으로 스폰되는 "월드 픽업" 액터.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractableInterface.h"
#include "ItemPickup.generated.h"

/**
 * 레벨에 배치되거나 드랍으로 스폰되는 "월드 픽업" 액터.
 *
 * 인벤토리 슬롯에 들어가는 FItemInstance(런타임 상태, ItemInstance.h)와는
 * 다른 층위다. 이 액터는 그저 "월드에 놓인 아이템의 시각적 표현 + 상호작용
 * 트리거" 역할만 하고, 인벤토리 슬롯 인덱스나 장비 여부 같은 상태는 전혀 모른다. 
 * 실제로 주울 때는 이 액터가 들고 있는 ItemData/Count로 새 FItemInstance를 
 * 만들어 인벤토리 배열에 추가하고, 이 액터 자신은 파괴.
 * 반대로 드랍할 때는 인벤토리에서 FItemInstance를 제거하면서 그 자리에
 * AItemPickup을 스폰해 ItemData/Count를 그대로 넘겨준다.
 *
 * 획득 방식은 두 가지를 아이템 정의(UItemDataBase::DefaultPickupMode) 기준으로
 * 분기한다.
 * - LookAndPress: IInteractable을 구현해서, 캐릭터의 UInteractionComponent가
 *   조준 중 감지 → 이름 표시 → 단축키 입력 시 Interact() 호출.
 * - AutoOnOverlap: InteractionSphere 오버랩 즉시 자동 획득 (조준/입력 불필요).
 *   여러 개가 무더기로 흩어지는 광석·제작 재료 등에 적합.
 *
 * IInteractable을 구현해두는 건 AutoOnOverlap 아이템에도 해가 되지 않는다 —
 * 조준하면 이름 정도는 뜨고, 어차피 오버랩으로 먼저 자동 획득되니 실질적으로는
 * 안 쓰일 뿐이다.
 *
 */

class UItemDataBase;
class USphereComponent;

UCLASS()
class R1_API AItemPickup : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItemPickup();

	// 드랍으로 스폰될 때 사용 — ItemData/Count를 지정하고 시각적 표현을 즉시 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeFromItem(UItemDataBase* InItemData, int32 InCount);
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//~ Begin IInteractable Interface
	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual TSoftObjectPtr<UTexture2D> GetInteractionIcon_Implementation() const override;
	//~ End IInteractable Interface

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	// ItemData->PickupMesh를 읽어 Mesh 컴포넌트에 반영. 생성자/OnConstruction에서
	// 호출되므로 레벨에 배치한 뒤 디테일 패널에서 ItemData를 바꿀 때마다
	// 에디터 뷰포트에서 바로 메시가 갱신된다.
	void RefreshVisual();

	// ItemData/Count를 Interactor의 UInventoryComponent에 실제로 넘기는 공용 처리.
	// LookAndPress(Interact_Implementation)와 AutoOnOverlap(오버랩 이벤트) 양쪽에서
	// 공유한다. 전부 들어갔으면 액터를 파괴하고, 일부만 들어갔으면 남은 수량만큼
	// Count를 줄인 채 액터를 그대로 남긴다(인벤토리가 꽉 찬 경우 등).
	void TryGrantToInventory(APawn* Interactor);

public:
	// 이 픽업을 주웠을 때 인벤토리에 들어갈 아이템 정의.
	// 레벨에 배치할 때 디테일 패널에서 직접 지정하거나, 드랍 로직에서
	// 스폰 직후 InitializeFromItem으로 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemDataBase> ItemData;

	// 스택형 아이템의 수량. 장비 아이템은 항상 1로 취급.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 Count = 1;

protected:
	// 표시할 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 인터랙션 용 스피어
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;



};
