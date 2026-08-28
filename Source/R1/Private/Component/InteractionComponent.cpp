// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/InteractionComponent.h"
#include "Interface/InteractableInterface.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"

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

void UInteractionComponent::SetActorHighlight(AActor* Actor, bool bEnable)
{
	if (!bEnable)
	{
		if (HighlightCloneMesh)
		{
			HighlightCloneMesh->SetVisibility(false);
		}
		return;
	}

	if (!IsValid(Actor))
	{
		return;
	}

	/// 아웃라인 하이라이트용 코드
	// 지금은 아이템 픽업처럼 스태틱 메시 하나로 이루어진 액터만 지원한다.
	UStaticMeshComponent* TargetMesh = Actor->FindComponentByClass<UStaticMeshComponent>();
	if (!TargetMesh || !TargetMesh->GetStaticMesh())
	{
		return;
	}

	if (!HighlightCloneMesh)
	{
		// 대상이 바뀔 때마다 새로 만들고 지우는 대신, 하나만 만들어 재사용한다
		// (한 번에 대상 하나만 하이라이트되므로 충분함).
		HighlightCloneMesh = NewObject<UStaticMeshComponent>(GetOwner());
		HighlightCloneMesh->RegisterComponentWithWorld(GetWorld());
		HighlightCloneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HighlightCloneMesh->SetCastShadow(false);
		HighlightCloneMesh->SetReceivesDecals(false);
	}
	
	HighlightCloneMesh->SetStaticMesh(TargetMesh->GetStaticMesh());
	HighlightCloneMesh->SetMaterial(0, HighlightOverlayMaterial);
	HighlightCloneMesh->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	HighlightCloneMesh->SetRelativeLocation(FVector::ZeroVector);
	HighlightCloneMesh->SetRelativeRotation(FRotator::ZeroRotator);
	HighlightCloneMesh->SetRelativeScale3D(FVector(HighlightScaleMultiplier));
	HighlightCloneMesh->SetVisibility(true);
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

	// IsValid()로 비교하는 이유: 같은 프레임 안에서 대상이 Destroy()된 경우(예:
	// TryGrantToInventory에서 획득 즉시 파괴) UPROPERTY는 실제 GC가 돌기 전까지는
	// 아직 non-null일 수 있다 — 포인터 자체 비교(!=)만으로는 이미 Pending Kill인
	// 액터를 "그대로 조준 중"으로 오인해 Execute_ 호출 시 어서션을 만날 수 있다.
	const bool bCurrentTargetStillValid = IsValid(CurrentTarget);

	// 대상이 실제로 바뀔 때만 브로드캐스트 — 매 틱 쏘는 트레이스라 여기서 걸러주지
	// 않으면 UI 쪽에서 매 프레임 갱신 이벤트를 처리해야 한다.
	if (NewTarget != CurrentTarget || !bCurrentTargetStillValid)
	{
		// 이전 대상이 이미 Destroy()돼서 더 이상 유효하지 않아도(예: 인벤토리로
		// 획득되며 즉시 파괴) 하이라이트는 반드시 꺼야 한다 — 안 그러면 HighlightCloneMesh가
		// 파괴된 대상에 붙어 있던 자리에 그대로 남아(부착 대상 컴포넌트가 없어지며 고아
		// 상태로 남음) 다음에 새 대상을 조준하기 전까지 화면에 계속 떠 있게 된다.
		// SetActorHighlight(..., false)는 Actor 인자를 쓰지 않으므로 CurrentTarget이
		// 이미 무효(Pending Kill/nullptr)여도 안전하게 호출할 수 있다.
		SetActorHighlight(CurrentTarget, false);

		CurrentTarget = IsValid(NewTarget) ? NewTarget : nullptr;

		FText DisplayName = FText::GetEmpty();
		UTexture2D* Icon = nullptr;

		if (CurrentTarget)
		{
			// outline 하이라이트 
			SetActorHighlight(CurrentTarget, true);
			
			// 이름, 아이콘 표시
			DisplayName = IInteractableInterface::Execute_GetInteractionDisplayName(CurrentTarget);
			Icon = IInteractableInterface::Execute_GetInteractionIcon(CurrentTarget).LoadSynchronous();
		}
	
		OnInteractableTargetChanged.Broadcast(CurrentTarget, DisplayName, Icon);
	}
}

