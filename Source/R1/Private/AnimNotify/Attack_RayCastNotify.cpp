// 08/28 주형진


#include "AnimNotify/Attack_RayCastNotify.h"
#include "Character/ActionCharacter.h"

void UAttack_RayCastNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	// 해당 노티파이가 들어오면 공격 실행
	if(AActionCharacter* AC = Cast<AActionCharacter>(MeshComp->GetOwner()))
	{
		AC->ProcessAttack();
	}
}
