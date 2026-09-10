/// 최초작성 : 2026.08.31
/// 작 성 자 : 강 진 구
/// 침낭 테스트

#pragma once

#include "CoreMinimal.h"
#include "Item/PlaceableItemBase.h"
#include "Interface/InteractableInterface.h"
#include "Interface/RespawnPointInterface.h"
#include "Character/ActionCharacter.h"
#include "SleepingBag.generated.h"

UCLASS()
class R1_API ASleepingBag : public APlaceableItemBase, public IRespawnPointInterface, public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASleepingBag();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual FTransform GetRespawnTransform_Implementation() const override;

	//~ Begin IInteractableInterface
	virtual FText GetInteractionDisplayName_Implementation() const override { return FText::FromString(TEXT("잠자기 (리스폰 등록)")); }
	//누군가 잠자고 있으면 상호작용 불가
	virtual bool CanInteract_Implementation(APawn* Interactor) const override { return OccupantCharacter == nullptr; }
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual TSoftObjectPtr<UTexture2D> GetInteractionIcon_Implementation() const override;
	//~ End IInteractableInterface

	void ClearOccupant(AActionCharacter* Character);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;


protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SleepingBag")
	TSoftObjectPtr<UTexture2D> InteractionIcon;

	// ★ 현재 이 침낭에서 자고 있는 플레이어 (멀티플레이 동기화)
	UPROPERTY(Replicated)
	TObjectPtr<AActionCharacter> OccupantCharacter;

};
