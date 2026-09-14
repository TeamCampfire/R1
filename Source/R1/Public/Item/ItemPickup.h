/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 레벨에 배치되거나 드랍으로 스폰되는 "월드 픽업" 액터.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractableInterface.h"
#include "Interface/HarvestPoolable.h"
#include "ItemPickup.generated.h"

class UItemDataBase;
class USphereComponent;
class UTexture2D;

UCLASS()
class R1_API AItemPickup : public AActor, public IInteractableInterface, public IHarvestPoolable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItemPickup();

	// 드랍으로 스폰될 때 사용 — ItemData/Count를 지정하고 시각적 표현을 즉시 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeFromItem(UItemDataBase* InItemData, int32 InCount);

	// 스폰 직후 던지는 연출용 — Mesh(물리 시뮬레이션 켜져있음)에 impulse를 준다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void AddThrowImpulse(const FVector& Impulse);
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//~ Begin AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	//~ Begin IInteractable Interface
	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual TSoftObjectPtr<UTexture2D> GetInteractionIcon_Implementation() const override;
	//~ End IInteractable Interface

	//~ Begin IHarvestPoolable Interface
	virtual void OnTakenFromHarvestPool_Implementation() override;
	virtual void OnReturnedToHarvestPool_Implementation() override;
	//~ End IHarvestPoolable Interface

	/** Applies pooled visibility/collision state on the server and every client. */
	void SetPoolActive(bool bActive);
	bool IsPoolActive() const { return bIsPoolActive; }

	void SetIsFromDropPool(bool bFromPool) { bIsFromDropPool = bFromPool; }
	bool IsFromDropPool() const { return bIsFromDropPool; }

	/** Bound internally by AHarvestSpawner. If unbound, depletion keeps the legacy destroy behavior. */
	UPROPERTY()
	FOnHarvestableDepleted OnHarvestableDepleted;

protected:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetPoolActive(bool bActive);

	UFUNCTION()
	void OnRep_PoolActive();

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

	// ItemData가 리플리케이트되어 도착했을 때(드랍으로 새로 스폰된 픽업이 클라이언트에
	// 처음 동기화되는 시점) 메시를 다시 갱신한다 — 그 전까지는 클라이언트의 ItemData가
	// null이라 RefreshVisual()이 메시를 지운 상태로 남아있기 때문.
	UFUNCTION()
	void OnRep_ItemData();

public:
	// 이 픽업을 주웠을 때 인벤토리에 들어갈 아이템 정의.
	// 레벨에 배치할 때 디테일 패널에서 직접 지정하거나, 드랍 로직에서
	// 스폰 직후 InitializeFromItem으로 지정한다. 드랍으로 동적 스폰되는 픽업은
	// 클라이언트가 서버로부터 이 값을 리플리케이션으로 받아야 시각적으로 보이므로
	// ReplicatedUsing으로 걸어둔다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ItemData, Category = "Item")
	TObjectPtr<UItemDataBase> ItemData;

	// 스택형 아이템의 수량. 장비 아이템은 항상 1로 취급.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Item", meta = (ClampMin = "1"))
	int32 Count = 1;

	// 조준 시 표시할 아이콘. 아이템별 아이콘(ItemData->Icon)이 아니라 "픽업 가능한 대상"이라는
	// 것 자체를 나타내는 공통 아이콘 하나를 모든 픽업이 공유한다 — 무엇을 주울 수 있는지는
	// 이름 텍스트(GetInteractionDisplayName)로 이미 구분되므로, 아이콘은 종류 상관없이
	// "픽업 가능"을 뜻하는 통일된 이미지로 고정한다(PickupNotificationWidget::CommonPickupIcon과
	// 동일한 이유). BP_ItemPickup 계열의 클래스 디폴트에서 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	TSoftObjectPtr<UTexture2D> CommonPickupIcon;

protected:
	// 표시할 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 인터랙션 용 스피어
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

private:
	UPROPERTY(ReplicatedUsing = OnRep_PoolActive)
	bool bIsPoolActive = true;

	bool bIsFromDropPool = false;
};
