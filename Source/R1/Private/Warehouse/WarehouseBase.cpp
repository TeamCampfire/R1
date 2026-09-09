// Fill out your copyright notice in the Description page of Project Settings.


#include "Warehouse/WarehouseBase.h"
#include "Component/WarehouseInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Character/ActionPlayerController.h"
#include "GameFramework/Pawn.h"

AWarehouseBase::AWarehouseBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// UInteractionComponent가 조준 시 스텐실 값만 바꿔 하이라이트를 켜고 끌 수 있도록,
	// AItemPickup과 동일하게 등록 전에 미리 켜둔다.
	Mesh->SetRenderCustomDepth(true);

	StorageComponent = CreateDefaultSubobject<UWarehouseInventoryComponent>(TEXT("StorageComponent"));
}

FText AWarehouseBase::GetInteractionDisplayName_Implementation() const
{
	return DisplayName;
}

bool AWarehouseBase::CanInteract_Implementation(APawn* Interactor) const
{
	return Interactor != nullptr;
}

void AWarehouseBase::Interact_Implementation(APawn* Interactor)
{
	// InteractionComponent::Server_TryInteract를 통해 서버에서 실행된다. 상호작용을 요청한
	// 클라이언트에게만 UI를 열라고 알려야 하므로, 그 폰의 컨트롤러로 Client RPC를 보낸다.
	if (!Interactor)
	{
		return;
	}

	if (AActionPlayerController* PC = Cast<AActionPlayerController>(Interactor->GetController()))
	{
		PC->Client_OpenWarehouse(StorageComponent);
	}
}

TSoftObjectPtr<UTexture2D> AWarehouseBase::GetInteractionIcon_Implementation() const
{
	return InteractionIcon;
}
