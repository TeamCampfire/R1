// 작업 시작일 : 9/7
// 작업자 : 우진

#pragma once

#include "CoreMinimal.h"
#include "Item/PlaceableItemBase.h"
#include "Interface/InteractableInterface.h"
#include "Workbench.generated.h"

/**
 * 
 */
UCLASS()
class R1_API AWorkbench : public APlaceableItemBase, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AWorkbench();
	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;
};
