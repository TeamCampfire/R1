

#pragma once

#include "CoreMinimal.h"
#include "Item/HeldItemBase.h"
#include "Torch.generated.h"

class UNiagaraComponent;
class UPointLightComponent;

/**
 * 
 */
UCLASS()
class R1_API ATorch : public AHeldItemBase
{
	GENERATED_BODY()

public:
	ATorch();
	virtual void OnPrimaryActionStarted() override;
	virtual void OnSecondaryActionStarted() override;
	virtual void InitItemVisual(UHeldItemData* InItemData) override;

protected:
	UFUNCTION(Server, Reliable)
	void Server_PlayMontage(UAnimMontage* TargetMontage);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayMontage(UAnimMontage* TargetMontage);

	UFUNCTION(Server, Reliable)
	void Server_ToggleState();

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	virtual void OnItemStateChanged(bool bNewState) override;

protected:
	// 전용 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Torch|Animation")
	TObjectPtr<UAnimMontage> LitAttackMontage;

	// 전용 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Torch|Animation")
	TObjectPtr<UAnimMontage> UnlitAttackMontage;

	// 전용 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Torch|Animation")
	TObjectPtr<UAnimMontage> LitMontage;

	// 전용 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Torch|Animation")
	TObjectPtr<UAnimMontage> UnlitMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Torch|Fx")
	TObjectPtr<UNiagaraComponent> FlameFxComponent3P;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Torch|Fx")
	TObjectPtr<UNiagaraComponent> FlameFxComponent1P;

	// 주변을 밝히는 조명
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torch|FX")
	TObjectPtr<UPointLightComponent> TorchFireLight;

};
