// 작업 시작일 : 9/7
// 작업자 : 우진


#include "Item/PlaceableItem/Workbench.h"
#include "Character/ActionPlayerController.h"
#include "GameFramework/Pawn.h"

AWorkbench::AWorkbench()
{
	PrimaryActorTick.bCanEverTick = false;
}

FText AWorkbench::GetInteractionDisplayName_Implementation() const
{
	return FText::FromString(TEXT("작업대"));
}

bool AWorkbench::CanInteract_Implementation(APawn* Interactor) const
{
	// UI 개방과 서버 제작 요청이 동일한 거리 제한을 사용한다(언리얼 단위: cm).
	constexpr float InteractionDistance = 300.f;
	return IsValid(Interactor)
		&& FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionDistance);
}

void AWorkbench::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || !CanInteract_Implementation(Interactor))
	{
		return;
	}
	if (AActionPlayerController* Controller = Cast<AActionPlayerController>(Interactor->GetController()))
	{
		Controller->Client_OpenCrafting(this);
	}
}
