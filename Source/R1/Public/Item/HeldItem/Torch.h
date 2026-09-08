

#pragma once

#include "CoreMinimal.h"
#include "Item/HeldItemBase.h"
#include "Torch.generated.h"

/**
 * 
 */
UCLASS()
class R1_API ATorch : public AHeldItemBase
{
	GENERATED_BODY()


public:
	virtual void OnPrimaryActionStarted() override;
	virtual void OnSecondaryActionStarted() override;

protected:
	UFUNCTION(Server, Reliable)
	void Server_PlayMontage(UAnimMontage* TargetMontage);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayMontage(UAnimMontage* TargetMontage);

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


};
