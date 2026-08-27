// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/InteractionComponent.h"
#include "Interface/InteractableInterface.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"

// Sets default values for this component's properties
UInteractionComponent::UInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UInteractionComponent::TryInteract()
{
	if (!CurrentTarget || !CurrentTarget->Implements<UInteractableInterface>())
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (IInteractableInterface::Execute_CanInteract(CurrentTarget, OwnerPawn))
	{
		IInteractableInterface::Execute_Interact(CurrentTarget, OwnerPawn);
	}
}


// Called when the game starts
void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateTargeting();
}

void UInteractionComponent::UpdateTargeting()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	// 1인칭 캐릭터 기준 눈높이 카메라를 찾아 그 위치/방향으로 트레이스한다.
	UCameraComponent* Camera = OwnerPawn->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return;
	}

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * TraceDistance;

	// Owner는 감지 대상에서 제외
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerPawn);

	FHitResult Hit;
	AActor* NewTarget = nullptr;
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params))
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (HitActor->Implements<UInteractableInterface>())
			{
				NewTarget = HitActor;
			}
		}
	}

	// 대상이 실제로 바뀔 때만 브로드캐스트 — 매 틱 쏘는 트레이스라 여기서 걸러주지
	// 않으면 UI 쪽에서 매 프레임 갱신 이벤트를 처리해야 한다.
	if (NewTarget != CurrentTarget)
	{
		CurrentTarget = NewTarget;

		const FText DisplayName = CurrentTarget
			? IInteractableInterface::Execute_GetInteractionDisplayName(CurrentTarget)
			: FText::GetEmpty();

		OnInteractableTargetChanged.Broadcast(CurrentTarget, DisplayName);
	}
}

