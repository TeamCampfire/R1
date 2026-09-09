#pragma once

#include "CoreMinimal.h"
#include "Item/PlaceableItemBase.h"
#include "Interface/InteractableInterface.h"
#include "Workbench.generated.h"

class UCraftingComponent;

// [wdk59] 설치 가능한 작업대에 공유 제작 컴포넌트와 거리 기반 상호작용을 연결한다.
UCLASS()
class R1_API AWorkbench : public APlaceableItemBase, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AWorkbench();
	virtual void Destroyed() override;
	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	UCraftingComponent* GetCraftingComponent() const { return CraftingComponent; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<UCraftingComponent> CraftingComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting", meta = (ClampMin = "0"))
	float InteractionDistance = 300.f;
};
