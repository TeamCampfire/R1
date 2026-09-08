#pragma once

#include "CoreMinimal.h"
#include "Item/PlaceableItemBase.h"
#include "Interface/InteractableInterface.h"
#include "Campfire.generated.h"


class UCampfireComponent;
class UParticleSystemComponent;
class UAudioComponent;
class USoundBase;
class USoundAttenuation;

UCLASS()
class R1_API ACampfire : public APlaceableItemBase, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ACampfire();
	// 설치 아이템 소비까지 성공한 서버에서만 호출한다.
	void NotifyPlacementSucceeded();

	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "Campfire")
	UCampfireComponent* GetCampfireComponent() const { return CampfireComponent; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayPlacementSound();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire|Audio")
	TObjectPtr<USoundBase> PlacementSound;

	// Sound Wave를 직접 사용하면 해당 에셋의 Looping을 켜야 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire|Audio")
	TObjectPtr<USoundBase> BurningSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire|Audio")
	TObjectPtr<USoundBase> ExtinguishSound;

	// 설치, 연소, 꺼짐 소리 적용에 공통으로 사용할 사운드 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire|Audio")
	TObjectPtr<USoundAttenuation> WorldSoundAttenuation;

	UFUNCTION()
	void HandleCampfireStateChanged();

	// 사운드 재생용 Audio Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|FX")
	TObjectPtr<UAudioComponent> FireAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UCampfireComponent> CampfireComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire")
	FText InteractionName = NSLOCTEXT("Campfire", "InteractionName", "모닥불");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "0"))
	float InteractionDistance = 350.f;

private:
	bool bAudioStateInitialized = false;
	bool bWasLitForAudio = false;
	bool bPlacementSoundSent = false;
};
