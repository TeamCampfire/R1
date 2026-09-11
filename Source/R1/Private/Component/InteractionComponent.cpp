// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/InteractionComponent.h"
#include "Interface/InteractableInterface.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Item/PlaceableItem/Campfire.h"
#include "CollisionShape.h"

// Sets default values for this component's properties
UInteractionComponent::UInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// 리플리케이트 되는 컴포넌트 설정
	SetIsReplicatedByDefault(true);
}

// 현재 상호작용 중인 모닥불 UI의 대상 액터를 저장
void UInteractionComponent::SetActiveCampfire(ACampfire* Campfire)
{
	ActiveCampfire = Campfire;
}

ACampfire* UInteractionComponent::GetActiveCampfire() const
{
	return ActiveCampfire.Get();
}

void UInteractionComponent::TryInteract()
{
	UE_LOG(LogTemp, Log, TEXT("[PIE %d][%s] TryInteract 호출됨"),
		UE::GetPlayInEditorID(),
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	if (!CurrentTarget || !CurrentTarget->Implements<UInteractableInterface>())
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (IInteractableInterface::Execute_CanInteract(CurrentTarget, OwnerPawn))
	{
		Server_TryInteract(CurrentTarget);	// <- 클라이언트는 "이거 상호작용 하고 싶다"는 요청만 서버로 보내고, 실제 상호작용은 서버에서 실행한다.
	}
}

bool UInteractionComponent::Server_TryInteract_Validate(AActor* Target)
{
	return Target != nullptr;
}

void UInteractionComponent::Server_TryInteract_Implementation(AActor* Target)
{
	if (!Target || !Target->Implements<UInteractableInterface>())
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (IInteractableInterface::Execute_CanInteract(Target, OwnerPawn))
	{
		IInteractableInterface::Execute_Interact(Target, OwnerPawn);	// <- 실제 상호작용은 서버에서 실행한다.
	}
}

void UInteractionComponent::ClearTarget()
{
	if (!CurrentTarget)
	{
		return;
	}

	SetActorHighlight(CurrentTarget, false);
	CurrentTarget = nullptr;

	OnInteractableTargetChanged.Broadcast(nullptr, FText::GetEmpty(), nullptr);
}

void UInteractionComponent::SetActorHighlight(AActor* Actor, bool bEnable)
{
	if (!IsValid(Actor))
	{
		return;
	}

	/// 아웃라인 하이라이트용 코드
	// 복제 메시로 덧씌우는 대신, 대상의 모든 프리미티브 컴포넌트에 CustomDepth
	// 렌더링을 직접 켜고 끈다 — 레벨의 PostProcessVolume에 등록된
	// M_InteractionOutline_PostProcess가 CustomDepth 스텐실 값을 읽어 외곽선을
	// 그린다. 메시 개수/모양이나 카메라 각도와 무관하게 실제 실루엣을 그대로
	// 따라가므로, 인버티드 헐 복제 방식과 달리 시야각에 따라 테두리가 빠지거나
	// 면이 통째로 덮이는 문제가 생기지 않는다.
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimComp : PrimitiveComponents)
	{
		if (!PrimComp)
		{
			continue;
		}

		// SetRenderCustomDepth()는 값이 바뀔 때 내부적으로 MarkRenderStateDirty()를
		// 호출해 씬 프록시를 다시 만든다 — 조준할 때마다 껐다 켰다 하면 그 순간
		// TSR 히스토리 재투영이 깨지면서 한두 프레임 동안 노이즈/줄무늬가 튄다.
		// 그래서 CustomDepth 자체는 한 번 켜면 계속 켜둔 채로 두고, on/off는 프록시
		// 재생성이 필요 없는 SetCustomDepthStencilValue만으로 처리한다 — 스텐실 값
		// 0은 배경과 같아서 외곽선 머티리얼이 그리지 않으므로 시각적으로는 동일하다.
		if (bEnable)
		{
			PrimComp->SetRenderCustomDepth(true);
			PrimComp->SetCustomDepthStencilValue(HighlightStencilValue);
		}
		else
		{
			PrimComp->SetCustomDepthStencilValue(0);
		}
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
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
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

	if (!GetWorld())
	{
		return;
	}

	// 1단계: InteractionAssistRadius만큼 구를 스윕해서 후보를 넓게 모은다(반경이 0 이하면 얇은
	// 라인 멀티 트레이스와 동일 — assist 기능을 끈 것과 같다). 이 반경 "안"에 실제로 걸린
	// 액터만 후보가 되므로, 이후 정밀 판정에서도 이 범위 밖의 엉뚱한 대상이 뽑힐 일은 없다.
	TArray<FHitResult> SweepHits;
	if (InteractionAssistRadius > 0.f)
	{
		GetWorld()->SweepMultiByChannel(SweepHits, Start, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(InteractionAssistRadius), Params);
	}
	else
	{
		GetWorld()->LineTraceMultiByChannel(SweepHits, Start, End, TraceChannel, Params);
	}

	// 인터랙터블만 추려 중복 제거(같은 액터가 여러 폴리곤에 걸릴 수 있음). Sweep/LineTraceMulti
	// 결과는 거리순 정렬이라 Candidates도 가까운 순서 그대로 유지된다.
	TArray<AActor*> Candidates;
	for (const FHitResult& EachHit : SweepHits)
	{
		// Multi 트레이스는 블로킹 히트뿐 아니라 오버랩 히트도 함께 돌려준다. AItemPickup의
		// InteractionSphere(반경 120cm, AutoOnOverlap 자동 획득용 — OverlapAllDynamic 프로파일)가
		// 여기 걸리면 조준과 무관하게 100cm 넘게 떨어진 아이템까지 후보로 잡혀버리므로, 실제로
		// 콜리전을 막아선(블로킹) 히트만 후보로 인정한다.
		if (!EachHit.bBlockingHit)
		{
			continue;
		}

		AActor* HitActor = EachHit.GetActor();
		if (HitActor && HitActor->Implements<UInteractableInterface>()
			&& IInteractableInterface::Execute_CanInteract(HitActor, OwnerPawn))
		{
			Candidates.AddUnique(HitActor);
		}
	}

	AActor* NewTarget = nullptr;
	if (Candidates.Num() == 1)
	{
		NewTarget = Candidates[0];
	}
	else if (Candidates.Num() > 1)
	{
		// 2단계: 후보가 2개 이상 겹쳤을 때만 Complex Collision(심플 콜리전이 아니라 실제 렌더
		// 메시 삼각형 기준) 정밀 라인트레이스로 크로스헤어가 진짜 가리키는 하나를 고른다.
		FCollisionQueryParams PreciseParams = Params;
		PreciseParams.bTraceComplex = true;

		FHitResult PreciseHit;
		if (GetWorld()->LineTraceSingleByChannel(PreciseHit, Start, End, TraceChannel, PreciseParams))
		{
			AActor* HitActor = PreciseHit.GetActor();
			if (HitActor && Candidates.Contains(HitActor))
			{
				NewTarget = HitActor;
			}
		}

		// 정밀 트레이스가 후보 중 누구도 못 맞히면(다들 가장자리에 애매하게 걸친 경우) 가장
		// 가까운 후보로 폴백한다.
		if (!NewTarget)
		{
			NewTarget = Candidates[0];
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
		// 이전 대상이 이미 Destroy()돼서 더 이상 유효하지 않다면(예: 인벤토리로
		// 획득되며 즉시 파괴) CustomDepth를 끌 대상 자체가 사라진 것이므로 굳이 끌
		// 필요가 없다 — SetActorHighlight 내부에서 IsValid(Actor) 체크로 안전하게
		// 걸러진다.
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

