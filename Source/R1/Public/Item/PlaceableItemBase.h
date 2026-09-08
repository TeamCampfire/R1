// 작업 시작일 : 9/6
// 작업자 : 우진

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlaceableItemBase.generated.h"

/**
 * 월드에 설치된 Placeable 아이템의 공통 기반 액터.
 * 서버에서 아이템 데이터를 초기화하고
 * 빌딩 파츠 정의 데이터를 통해 복제된 데이터를 통해 클라이언트에도 동일한 메시를 적용해요
 */
UCLASS()
class R1_API APlaceableItemBase : public AActor
{
	GENERATED_BODY()
	
public:	
	APlaceableItemBase();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	// 서버에서 Placeable 액터가 생성된 직후 원본 아이템 데이터를 적용해요
	void InitializePlaceable(class UPlaceableItemData* InPlaceableItemData);

	UFUNCTION(BlueprintPure, Category = "Placeable|Durability")
	float GetCurrentDurability() { return CurrentDurability; }

	UFUNCTION(BlueprintPure, Category = "Placeable|Durability")
	float GetMaxDurability();

	// Placeable 아이템에 데미지를 적용하는 함수
	UFUNCTION(BlueprintCallable, Category = "Placeable|Durability")
	bool ApplyPlaceableDamage(float DamageAmount);

protected:
	// PlaceableItemData가 클라이언트에 복제된 뒤 메시를 적용해요
	UFUNCTION()
	void OnRep_PlaceableItemData();

	// 아이템 데이터가 가리키는 건축 파츠 정의의 메시를 적용해요
	void ApplyPlaceableData();

	//  ===================================================================================
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placeable")
	TObjectPtr<class USceneComponent> SceneRoot; // 루트 컴포넌트

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placeable")
	TObjectPtr<class UStaticMeshComponent> MeshComponent; // BuildingPartDefinition에 설정된 설치 메시를 표시하는 컴포넌트

	UPROPERTY(ReplicatedUsing = OnRep_PlaceableItemData, VisibleInstanceOnly, BlueprintReadOnly, Category = "Placeable")
	TObjectPtr<class UPlaceableItemData> PlaceableItemData; // 설치에 사용된 원본 아이템 데이터
	// OnRep을 통해 메시 컴포넌트에 적용됨

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Placeable|Durability")
	float CurrentDurability = 0.f; // 현재 아이템의 내구도
};
