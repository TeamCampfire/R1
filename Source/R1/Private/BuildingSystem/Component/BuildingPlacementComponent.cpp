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

	// 컨트롤러에서 플레이어 카메라가 보고 있는 현재 위치와 회전을 가져와요
	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FVector ViewForward = ViewRotation.Vector();

	// 캐릭터와 PreiveActor는 라인트레이스에서 제외!
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.bTraceComplex = false;
	if (APawn* ControlledPawn = PlayerController->GetPawn())
		CollisionQueryParams.AddIgnoredActor(ControlledPawn);

	CollisionQueryParams.AddIgnoredActor(PreviewActor);

	// 정면 벡터의 수평 크기 : 수평이면 1에 가까워지고, 수직 위아래는 0에 가까워짐.
	float HorizontalForwardSize = FVector2D(ViewForward.X, ViewForward.Y).Size();

	// 0에 가까울 때(거의 수직 아래를 바라볼 땐)
	// 카메라 방향으로 걍 trace 넘겨요. 무조건 최대 건축 지점 거리 안에 있을테니까!!
	if (HorizontalForwardSize <= KINDA_SMALL_NUMBER)
	{
		FVector VerticalTraceEnd = ViewLocation + ViewForward * GroundSearchDistance;
		
		// 카메라 수작 아래로 LineTrace 꽂아버려요
		FHitResult VerticalHitResult;
		bool bVerticalHit = GetWorld()->LineTraceSingleByChannel(
			VerticalHitResult, ViewLocation, VerticalTraceEnd, ECC_Visibility, CollisionQueryParams);

		if (true == bVerticalHit)
			PreviewActor->SetActorHiddenInGame(false);
		else
			PreviewActor->SetActorHiddenInGame(true);

		return;
	}

	// 카메라 선을 따라 얼만큼 이동해야 수평 거리가 MaxPlacementDistance가 되는지 계산해요
	// MaxPlacementDistance는 카메라와 프리뷰 액터 사이의 3차원 직선 거리가 아니라 수평 거리 계산임
	float ViewRayDistance = MaxPlacementDistance / HorizontalForwardSize;
	FVector MaxDistancePoint = ViewLocation + ViewForward * ViewRayDistance;

	// 먼저 카메라에서 최대 거리 지점(MaxDistancePoint)까지 Trace를 진행
	// 여기서 지면을 맞으면 최대 거리 이전이기 때문에 일반적인 지면 배치 상태 ㅇㅇ (=높이는 고려 안 함)
	FHitResult ForwardHitResult;
	bool bForwardHit = GetWorld()->LineTraceSingleByChannel(
			ForwardHitResult, ViewLocation, MaxDistancePoint, ECC_Visibility, CollisionQueryParams);

	if (true == bForwardHit)
	{
		PreviewActor->SetActorLocation(ForwardHitResult.ImpactPoint); // 높이 고려 안 해요
		PreviewActor->SetActorHiddenInGame(false);
		return;
	}

	// 최대 지면까지 지면을 맞추지 못 한 경우 (= 최대 건축 지점보다 멀리 보고 있음)
	// 이제 X,Y 좌표는 MaxDistancePoint에 고정하고, 카메라 조준으로 높이만 결정해요
	// MaxDistancePoint의 아래쪽으로 LineTrace를 쏴서 해당 위치의 실제 지면 높이를 찾아요
	FVector GroundTraceStart = MaxDistancePoint + FVector::UpVector * GroundSearchDistance;
	FVector GroundTraceEnd = MaxDistancePoint - FVector::UpVector * GroundSearchDistance;

	FHitResult GroundHitResult;
	bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
			GroundHitResult, GroundTraceStart, GroundTraceEnd, ECC_Visibility, CollisionQueryParams);

	if (false == bGroundHit)
	{
		PreviewActor->SetActorHiddenInGame(false);
		return;
	}

	// 최대 거리 지점의 카메라 LineTrace가 실제 지면에서 얼마나 떠있는지 계산
	float GroundZ = GroundHitResult.ImpactPoint.Z;
	float DesiredHeight = MaxDistancePoint.Z - GroundZ;

	// Foundation이 지면 아래로 내려가거나, 
	// Foundation의 다리기둥 길이보다 높게 뜨지 않도록 Clamp해버리기
	float FinalHeight = FMath::Clamp(DesiredHeight, 0.f, CurFoundationLegLength);

	FVector PreviewLocation(MaxDistancePoint.X, MaxDistancePoint.Y, GroundZ + FinalHeight);
	PreviewActor->SetActorLocation(PreviewLocation);

	PreviewActor->SetActorHiddenInGame(false);
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
	
	// Foundation 전용 : 메시 피벗 아래쪽으로 내려간 다리기둥 길이 계산
	FBoxSphereBounds MeshBounds = SelectedDefinition->PartMesh->GetBounds(); 

	// Foundation 피벗은 위에 존재해서 Foundation의 끝을 빼면 음수가 나옴, 
	// 추가적으로 (-)를 적용해 다리기둥 길이를 구했어요
	float LowestLocalZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
	CurFoundationLegLength = FMath::Max(0.f, -LowestLocalZ);

	// PreviewActor가 한 번도 생성된 적 없다면 하나 만들어주기
	if (false == IsValid(PreviewActor))
		PreviewActor = GetWorld()->SpawnActor<ABuildingPreviewActor>(PreviewActorClass, FVector::ZeroVector, FRotator::ZeroRotator);

	// 만들었는데도 없다? 리턴
	if (false == IsValid(PreviewActor))
	{
		SelectedDefinition = nullptr;
		CurFoundationLegLength = 0.f;
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

