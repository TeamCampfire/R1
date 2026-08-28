#include "BuildingSystem/Component/BuildingPlacementComponent.h"
#include "Data/Building/BuildingPartDefinition.h"
#include "BuildingSystem/BuildingPreviewActor.h"

UBuildingPlacementComponent::UBuildingPlacementComponent()
{
	// 평소에는 tick을 꺼두지만, 배치중일 땐 tick을 켜요
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UBuildingPlacementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuildingPlacementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if ((false == bIsPlacing) || (nullptr == SelectedDefinition))
		return;

	// 컴포넌트가 부착된 컨트롤러 찾아요
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (nullptr == PlayerController) return;

	// 컨트롤러에서 플레이어가 보고 있는 현재 방향과 위치를 가져와서
	// 라인트레이서 시작,끝 좌표 계산에 사용해요
	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FVector TraceStartLocation = ViewLocation;
	FVector TraceEndLocation = TraceStartLocation + ViewRotation.Vector() * MaxPlacementDistance;

	FVector ViewForward = ViewRotation.Vector();
	float HeightAlpha = FMath::Clamp(1.f + ViewForward.Z, 0.f, 1.f);

	// 캐릭터와 PreiveActor는 라인트레이스에서 제외!
	FCollisionQueryParams CollisionQueryParams;
	if (APawn* ControlledPawn = PlayerController->GetPawn())
		CollisionQueryParams.AddIgnoredActor(ControlledPawn);

	CollisionQueryParams.AddIgnoredActor(PreviewActor);

	// 진짜 라인트레이서 쏘기
	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult, TraceStartLocation, TraceEndLocation, ECC_Visibility, CollisionQueryParams);

	if (true == bHit)
	{
		// Hit 지점으로 PreviewActor 이동시킵니다
		PreviewActor->SetActorLocation(HitResult.ImpactPoint);
		PreviewActor->SetActorHiddenInGame(false);
	}
	else
		PreviewActor->SetActorHiddenInGame(true); // Hit가 없었으면 안 보여줍니다

}

void UBuildingPlacementComponent::StartPlacement(UBuildingPartDefinition* Definition)
{
	// 터짐 방지
	if (false == IsValid(Definition))
	{
		UE_LOG(LogTemp, Log, TEXT("[UBuildingPlacementComponent::StartPlacement] : Definition 안 넘어왔다"));
		return;
	}

	if (false == IsValid(PreviewActorClass))
	{
		UE_LOG(LogTemp, Log, TEXT("[UBuildingPlacementComponent::StartPlacement] : PreviewActorClass 세팅 안 된 듯"));
		return;
	}

	SelectedDefinition = Definition;
	
	// PreviewActor가 한 번도 생성된 적 없다면 하나 만들어주기
	if (false == IsValid(PreviewActor))
		PreviewActor = GetWorld()->SpawnActor<ABuildingPreviewActor>(PreviewActorClass, FVector::ZeroVector, FRotator::ZeroRotator);

	// 만들었는데도 없다? 리턴
	if (false == IsValid(PreviewActor))
	{
		SelectedDefinition = nullptr;
		return;
	}

	// PreviewActor 세팅
	PreviewActor->SetDefinition(SelectedDefinition);

	bIsPlacing = true; // 배치 ing...
	SetComponentTickEnabled(true); // 틱 돌아요
}

void UBuildingPlacementComponent::StopPlacement()
{
	// 배치 끝냈으니, 값들 리셋
	bIsPlacing = false;
	SelectedDefinition = nullptr;

	if (true == IsValid(PreviewActor))
		PreviewActor->SetActorHiddenInGame(true); // 화면에서 사라져요
}

