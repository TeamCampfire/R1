

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractableInterface.h"
#include "Interface/HarvestPoolable.h"
#include "Data/Item/ItemDataBase.h"
#include "Hemp.generated.h"

class FLifetimeProperty;

UCLASS()
class R1_API AHemp : public AActor, public IInteractableInterface, public IHarvestPoolable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHemp();

	//~ Begin IInteractableInterface
	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual TSoftObjectPtr<UTexture2D> GetInteractionIcon_Implementation() const override;
	//~ End IInteractableInterface

	//~ Begin IHarvestPoolable Interface
	virtual void OnTakenFromHarvestPool_Implementation() override;
	virtual void OnReturnedToHarvestPool_Implementation() override;
	//~ End IHarvestPoolable Interface

	/** Applies pooled visibility/collision state on the server and every client. */
	void SetPoolActive(bool bActive);
	bool IsPoolActive() const { return bIsPoolActive; }

	/** Bound internally by AHarvestSpawner. If unbound, depletion keeps the legacy destroy behavior. */
	UPROPERTY()
	FOnHarvestableDepleted OnHarvestableDepleted;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetPoolActive(bool bActive);

	UFUNCTION()
	void OnRep_PoolActive();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest")
	FText DisplayName = FText::FromString(TEXT("대마풀"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest")
	TObjectPtr<UItemDataBase> YieldItemData; // DA_Item_Misc_Cloth 지정

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest")
	int32 YieldCount = 10; // 획득할 천 개수

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest")
	TSoftObjectPtr<UTexture2D> InteractionIcon;


private:
	bool bHarvested = false;

	UPROPERTY(ReplicatedUsing = OnRep_PoolActive)
	bool bIsPoolActive = true;
};
