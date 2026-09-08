#include "Item/PlaceableItem/Workbench.h"
#include "Character/ActionPlayerController.h"
#include "Component/CraftingComponent.h"
#include "GameFramework/Pawn.h"

AWorkbench::AWorkbench()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));
}

void AWorkbench::Destroyed()
{
	if (HasAuthority()) CraftingComponent->DropContents();
	Super::Destroyed();
}

FText AWorkbench::GetInteractionDisplayName_Implementation() const
{
	return NSLOCTEXT("Crafting", "Workbench", "작업대");
}

bool AWorkbench::CanInteract_Implementation(APawn* Interactor) const
{
	return IsValid(Interactor)
		&& FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionDistance);
}

void AWorkbench::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || !CanInteract_Implementation(Interactor)) return;
	if (AActionPlayerController* Controller = Cast<AActionPlayerController>(Interactor->GetController()))
	{
		Controller->Client_OpenCrafting(this);
	}
}
