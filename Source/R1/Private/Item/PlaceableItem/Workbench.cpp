#include "Item/PlaceableItem/Workbench.h"

#include "Data/Item/PlaceableItemData.h"
#include "Character/ActionPlayerController.h"
#include "Component/CraftingComponent.h"
#include "GameFramework/Pawn.h"

AWorkbench::AWorkbench()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));
}

// 파괴되기 전에 서버에서 제작 결과물과 남은 재료를 반환
void AWorkbench::Destroyed()
{
	if (HasAuthority())
		CraftingComponent->DropContents();

	Super::Destroyed();
}

FText AWorkbench::GetInteractionDisplayName_Implementation() const
{
	return PlaceableItemData ? PlaceableItemData->DisplayName : FText::GetEmpty();
}

bool AWorkbench::CanInteract_Implementation(APawn* Interactor) const
{
	return IsValid(Interactor)
		&& FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionDistance);
}

// \서버의 거리 검사 후 해당 작업대의 제작 화면을 요청자에게 엶
void AWorkbench::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || !CanInteract_Implementation(Interactor)) return;
	if (AActionPlayerController* Controller = Cast<AActionPlayerController>(Interactor->GetController()))
	{
		Controller->Client_OpenCrafting(this);
	}
}
