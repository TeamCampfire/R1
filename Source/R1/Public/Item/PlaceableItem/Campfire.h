#pragma once

#include "CoreMinimal.h"
#include "Item/PlaceableItemBase.h"
#include "Interface/InteractableInterface.h"
#include "Campfire.generated.h"


class UCampfireComponent;
class UAudioComponent;
class USoundBase;
class USoundAttenuation;

UCLASS()
class R1_API ACampfire : public APlaceableItemBase, public IInteractableInterface
{
	GENERATED_BODY()

public:

	ACampfire();

	// PlaceableItem으로 이주한 모닥불의 설치 확정 알림
	// 서버에서 설치음을 한 번 전송 (설치 아이템 소비까지 성공한 서버에서만 호출)
	void NotifyPlacementSucceeded();

	// 상호작용 인터페이스 관련 함수
	virtual FText GetInteractionDisplayName_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "Campfire")
	UCampfireComponent* GetCampfireComponent() const { return CampfireComponent; }

protected:

	virtual void BeginPlay() override;

	// 모닥불 점화 상태 변화에 대한 사운드 조절 함수
	UFUNCTION()
	void HandleCampfireStateChanged();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayPlacementSound();

protected :

	/* 재생할 사운드 소스 - 블루프린트에서 설정 */

	// 설치음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire|Audio")
	TObjectPtr<USoundBase> PlacementSound = nullptr;

	// 연소음 - 사운드 에셋에서 Looping 활성화
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire|Audio")
	TObjectPtr<USoundBase> BurningSound = nullptr;

	// 소화음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire|Audio")
	TObjectPtr<USoundBase> ExtinguishSound = nullptr;

	// 설치, 연소, 꺼짐 소리 적용에 공통으로 사용할 사운드 설정 (거리 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire|Audio")
	TObjectPtr<USoundAttenuation> WorldSoundAttenuation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|FX")
	TObjectPtr<UAudioComponent> FireAudio;	// 연소음 재생용 Audio Component

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UCampfireComponent> CampfireComponent;	// 모닥불 기능을 위한 Campfire Component

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "0"))
	float InteractionDistance = 350.f;	// 상호작용 가능 거리

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campfire|FX")
	TObjectPtr<class UNiagaraComponent> FireVFX; // 연소 나이아가라
	
private:

	bool bWasLitForAudio = false;			// 마지막으로 오디오를 처리한 모닥불 점화 상태 (재생/정지 반복 호출 방지)
	bool bPlacementSoundSent = false;		// 설치음 멀티캐스트 요청을 했는지 (설치음이 여러번 재생되는 것 방지)
};
