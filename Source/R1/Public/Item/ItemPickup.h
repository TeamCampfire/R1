/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 레벨에 배치되거나 드랍으로 스폰되는 "월드 픽업" 액터.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
 * 그래서 인벤토리에 몇백 개의 아이템이 있어도 액터가 몇백 개 존재할 필요가
 * 없다 — 액터는 "지금 월드에 실제로 놓여있는 것"에만 대응한다.
 *
 * 실제 획득 처리(오버랩 즉시 자동 획득 vs 프롬프트+단축키)는 인벤토리
 * 컴포넌트 단계에서 InteractionSphere의 오버랩 이벤트를 구독해 붙일 예정
 *
 */

class UItemDataBase;
class USphereComponent;

UCLASS()
class R1_API AItemPickup : public AActor
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
	
protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// ItemData->PickupMesh를 읽어 Mesh 컴포넌트에 반영. 생성자/OnConstruction에서
	// 호출되므로 레벨에 배치한 뒤 디테일 패널에서 ItemData를 바꿀 때마다
	// 에디터 뷰포트에서 바로 메시가 갱신된다.
	void RefreshVisual();


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
