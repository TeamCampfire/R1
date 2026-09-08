
#include "Framework/AnimNotify/BuildingHammerHitNotify.h"
#include "Character/ActionCharacter.h"
#include "Component/HeldItemComponent.h"
#include "Item/HeldItem/BuildingHammer.h"

void UBuildingHammerHitNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (false == IsValid(MeshComp)) return;

	AActionCharacter * ActionCharacter = Cast<AActionCharacter>(MeshComp->GetOwner());
	if (false == IsValid(ActionCharacter)) return;

	UHeldItemComponent* HeldItemComponent = ActionCharacter->GetHeldItemComponent();
	if (false == IsValid(HeldItemComponent)) return;

	ABuildingHammer* Hammer = Cast<ABuildingHammer>(HeldItemComponent->GetCurrentHeldItem());
	if (false == IsValid(Hammer)) return;

	// 라인트레이스, 데미지 적용
	Hammer->PerformBuildingHit();
}
