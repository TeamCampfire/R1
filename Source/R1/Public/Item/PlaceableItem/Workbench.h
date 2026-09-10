#pragma once

#include "CoreMinimal.h"
#include "Item/PlaceableItemBase.h"
#include "Interface/InteractableInterface.h"
#include "Workbench.generated.h"

class UCraftingComponent;

// 설치 가능한 작업대에 공유 제작 컴포넌트와 거리 기반 상호작용을 연결
UCLASS()
class R1_API AWorkbench : public APlaceableItemBase, public IInteractableInterface
{
	GENERATED_BODY()

public:

	AWorkbench();

	virtual void Destroyed() override;

	/* 상호작용 관련 상속 함수 */
	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;

	// Crafting Component의 Getter
	UFUNCTION(BlueprintPure, Category = "Crafting")
	UCraftingComponent* GetCraftingComponent() const { return CraftingComponent; }

protected:

	// Crafting Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<UCraftingComponent> CraftingComponent;

	// UI 유지 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting", meta = (ClampMin = "0"))
	float InteractionDistance = 300.f;
};
