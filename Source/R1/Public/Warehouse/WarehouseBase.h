/// 최초작성 : 2026.09.06
/// 작 성 자 : 최 요 환
/// 간단설명 : 레벨에 배치되는 창고 액터

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractableInterface.h"
#include "WarehouseBase.generated.h"

class UStaticMeshComponent;
class UWarehouseInventoryComponent;
class UTexture2D;

/**
 * 레벨에 배치되는 창고 액터.
 *
 * AItemPickup과 마찬가지로 IInteractableInterface만 구현하면 UInteractionComponent가 조준/입력을
 * 전담해준다. 다른 점은 상호작용 결과가 "즉시 인벤토리에 흡수"가 아니라 "UI를 연다"는 것 —
 * Interact_Implementation은 InteractionComponent::Server_TryInteract를 통해 서버에서 실행되므로,
 * 상호작용한 클라이언트에게 UI를 열라고 알리려면 그 클라이언트의 PlayerController로 Client RPC를
 * 보내야 한다(AActionPlayerController::Client_OpenWarehouse 참고).
 */
UCLASS()
class R1_API AWarehouseBase : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AWarehouseBase();

	//~ Begin IInteractableInterface
	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual TSoftObjectPtr<UTexture2D> GetInteractionIcon_Implementation() const override;
	//~ End IInteractableInterface

	UFUNCTION(BlueprintPure, Category = "Warehouse")
	UWarehouseInventoryComponent* GetStorageComponent() const { return StorageComponent; }

protected:
	// 조준 시 표시할 이름(예: "창고", "나무 창고").
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warehouse")
	FText DisplayName = FText::FromString(TEXT("창고"));

	// 조준 시 표시할 아이콘 — 픽업(AItemPickup::CommonPickupIcon)과 달리 창고는 "픽업"이 아니라
	// "여는 상호작용"이므로 그에 맞는 별도 아이콘을 쓴다. 창고 종류별로 다른 아이콘을 쓰고
	// 싶으면 서브클래스/BP 인스턴스마다 이 값만 다르게 지정하면 된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warehouse")
	TSoftObjectPtr<UTexture2D> InteractionIcon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWarehouseInventoryComponent> StorageComponent;
};
