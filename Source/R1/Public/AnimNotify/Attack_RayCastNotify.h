// 08/28 주형진

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Attack_RayCastNotify.generated.h"

/**
 * 
 */
UCLASS()
class R1_API UAttack_RayCastNotify : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp, 
		UAnimSequenceBase* Animation, 
		const FAnimNotifyEventReference& EventReference) override;
	
};
