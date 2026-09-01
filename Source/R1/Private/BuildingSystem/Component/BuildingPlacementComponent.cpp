#include "BuildingSystem/Component/BuildingPlacementComponent.h"

#include "Engine/OverlapResult.h"
#include "Components/StaticMeshComponent.h"
#include "../R1.h"

#include "Data/Building/BuildingPartDefinition.h"
#include "BuildingSystem/BuildingPreviewActor.h"
#include "BuildingSystem/BuildingActor.h"

UBuildingPlacementComponent::UBuildingPlacementComponent()
{
	// 평소에는 tick을 꺼두지만, 배치중일 땐 tick을 켜요
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// PlayerController를 통해 서버 RPC를 호출하기 위해 복제 활성화
	SetIsReplicatedByDefault(true);
}

void UBuildingPlacementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuildingPlacementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if ((false == bIsPlacing) || (nullptr == SelectedDefinition) || (false == IsValid(PreviewActor)))
		return;

	// 서버가 가진 원격 PlayerController 사본은 로컬 프리뷰 연산도 하면 안됩니다
	if (false == IsLocalPlacementController()) return;

	// 컴포넌트가 부착된 컨트롤러 찾아요
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (nullptr == PlayerController) 
	{
		HidePlacementPreview();
		return;
	}

	// 배치되는 방식에 따라 프리뷰 갱신이 달라져요
	switch (SelectedDefinition->PlacementType)
	{
	case EBuildingPlacementType::FOUNDATION:
	{
		// 기존 Foundation에서 스냅 될 수 있는 소켓을 찾았다면 스냅 프리뷰를 사용하고.  찾지 못했을 때만 기존의 지면 자유 배치를 실행합니다
		if(false == UpdateStructureSnapPreview(PlayerController))
			UpdateFoundationPreview(PlayerController);
	}
		break;

	case EBuildingPlacementType::STRUCTURE_SNAP:
		UpdateStructureSnapPreview(PlayerController);
		break;

	case EBuildingPlacementType::SURFACE:
	case EBuildingPlacementType::ATTACHMENT:
	default:
		HidePlacementPreview();
		break;
	}
}

void UBuildingPlacementComponent::StartPlacement(UBuildingPartDefinition* Definition)
{
	// 서버가 가진 원격 PlayerController 사본은 로컬 프리뷰를 생성하면 안됩니다ㅏ
	if (false == IsLocalPlacementController()) return;

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
	CurFoundationLegLength = 0.f;
	CurSnapYawOffsetIdx = 0;

	if (SelectedDefinition->PlacementType == EBuildingPlacementType::FOUNDATION)
	{
		// Foundation 전용 : 메시 피벗 아래쪽으로 내려간 다리기둥 길이 계산
		FBoxSphereBounds MeshBounds = SelectedDefinition->PartMesh->GetBounds(); 

		// Foundation 피벗은 위에 존재해서 Foundation의 끝을 빼면 음수가 나옴, 
		// 추가적으로 (-)를 적용해 다리기둥 길이를 구했어요
		float LowestLocalZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
		CurFoundationLegLength = FMath::Max(0.f, -LowestLocalZ);
	}

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
	ClearCurrentSnapTarget();
	bIsPlacing = false;
	SelectedDefinition = nullptr;
	CurFoundationLegLength = 0.f;
	if (true == IsValid(PreviewActor))
		PreviewActor->SetActorHiddenInGame(true); // 화면에서 사라져요
}

ABuildingPreviewActor* UBuildingPlacementComponent::GetPreviewActor()
{
	return PreviewActor;
}

void UBuildingPlacementComponent::ConfirmPlacement()
{
	if (false == bIsPlacing)
	{
		UE_LOG(LogTemp, Log, TEXT("[UBuildingPlacementComponent::ConfirmPlacement()] : 현재 건축 모드가 아닙니다."));
		return;
	}

	if (false == IsValid(SelectedDefinition) || false == IsValid(PreviewActor))
	{
		UE_LOG(LogTemp, Log, TEXT("[UBuildingPlacementComponent::ConfirmPlacement()] : 건축 파츠 또는 프리뷰가 없습니다."));
		return;
	}

	if (false == bCanPlace)
	{
		UE_LOG(LogTemp, Log, TEXT("[UBuildingPlacementComponent::ConfirmPlacement()] : 현재 위치에는 설치할 수 없습니다."));
		return;
	}

	// 배치되는 방식(EBuildingPlacementType)에 따라 서버에 요청하는 방식이 달라요
	switch (SelectedDefinition->PlacementType)
	{
	case EBuildingPlacementType::FOUNDATION:
	{
		const bool bHasFoundationSnapTarget = CurrentSnapBuilding.IsValid() && CurrentSnapTargetPartID.IsValid() && false == CurrentSnapSocketName.IsNone();
		if (true == bHasFoundationSnapTarget)
		{
			// 기존 Foundation에 스냅하는 경우
			ServerPlaceSnappedPart(SelectedDefinition.Get(), CurrentSnapBuilding.Get(),
				CurrentSnapTargetPartID, CurrentSnapSocketName, CurSnapYawOffsetIdx);

			break;
		}

		// 근처에 스냅될 수 있는 소켓이 없다면 기존처럼 지면에 새로운 BuildingActor를 생성
		if (nullptr == BuildingActorClass)
		{
			UE_LOG(LogTemp, Log, TEXT("[UBuildingPlacementComponent::ConfirmPlacement()] : BuildingActorClass가 설정되지 않았습니다."));
			return;
		}

		const FTransform PlacementTransform = PreviewActor->GetActorTransform(); // 프리뷰의 트랜스폼 가져오기
		ServerPlaceNewBuilding(SelectedDefinition.Get(), PlacementTransform); // 실제 건축물 생성은 서버에 요청해요

		break;
	}
	case EBuildingPlacementType::STRUCTURE_SNAP:
	{
		if (false == CurrentSnapBuilding.IsValid() || false == CurrentSnapTargetPartID.IsValid() || true == CurrentSnapSocketName.IsNone())
		{
			UE_LOG(LogTemp, Log, TEXT("[UBuildingPlacementComponent::ConfirmPlacement()] : 유효한 스냅 대상이 없습니다."));
			return;
		}

		// 실제 건축 파츠 스냅은 서버에 요청해요
		ServerPlaceSnappedPart(SelectedDefinition.Get(), CurrentSnapBuilding.Get(), CurrentSnapTargetPartID, CurrentSnapSocketName, CurSnapYawOffsetIdx);

		break;
	}
	case EBuildingPlacementType::SURFACE:
	case EBuildingPlacementType::ATTACHMENT:
	default:
	{
		UE_LOG(LogTemp, Log, TEXT("[UBuildingPlacementComponent::ConfirmPlacement()] : 아직 지원하지 않는 배치 방식입니다..."));
		break;
	}
	}
}

void UBuildingPlacementComponent::RotateBuildingPart()
{
	CycleSnapYawOffset();
	UE_LOG(LogTemp, Log, TEXT("현재 스냅 Yaw Offset: %.1f"), GetCurSnapYawOffset());
}

void UBuildingPlacementComponent::ServerPlaceSnappedPart_Implementation(UBuildingPartDefinition* Definition, ABuildingActor* TargetBuilding, FGuid TargetPartID, FName SocketName, int32 _SnapYawOffsetIdx)
{
	if (false == IsValid(Definition) || false == IsValid(Definition->PartMesh) ||
		false == IsValid(TargetBuilding) || false == TargetPartID.IsValid() || true == SocketName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 유효하지 않은 요청 데이터입니다."));
		return;
	}

	bool bSupportsSnapPlacement =
		Definition->PlacementType == EBuildingPlacementType::STRUCTURE_SNAP ||
		Definition->PlacementType == EBuildingPlacementType::FOUNDATION;

	if (false == bSupportsSnapPlacement)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 스냅을 지원하지 않는 파츠입니다."));
		return;
	}

	// 서버 기준의 건축 파츠 배열에서 대상 파츠를 PartID로 다시 검색
	FPlacedBuildingPart* TargetPart = TargetBuilding->FindPlacedPartByID(TargetPartID);
	if (nullptr == TargetPart || false == IsValid(TargetPart->Definition) || false == IsValid(TargetPart->MeshComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 대상 파츠를 찾을 수 없습니다."));
		return;
	}

	// 이미 사용된 소켓인지 검사
	if (TargetPart->OccupiedSnapPoints.Contains(SocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 이미 점유된 소켓입니다. Socket=%s"), *SocketName.ToString());
		return;
	}

	//서버가 Definition과 실제 메시 소켓을 이용해 직!접! 최종 월드 Transform을 계산해요
	FTransform SocketWorldTransform;
	if (false == TargetBuilding->TryGetSnapPointWorldTransform( 
		TargetPart->MeshComponent.Get(), Definition, SocketName, SocketWorldTransform))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 호환되는 스냅 소켓이 아닙니다."));
		return;
	}

	//스냅 Yaw 회전값을 Definition에서 직접 받아와 Trasform에 적용해요
	float SnapYawOffset = 0.f;
	if (Definition->AllowedSnapYawOffsets.Num() > 0)
	{
		if (false == Definition->AllowedSnapYawOffsets.IsValidIndex(_SnapYawOffsetIdx))
		{
			UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 허용되지 않은 회전 인덱스입니다. Index=%d"), _SnapYawOffsetIdx);
			return;
		}

		SnapYawOffset = Definition->AllowedSnapYawOffsets[_SnapYawOffsetIdx];
	}
	else if (_SnapYawOffsetIdx != 0)
		return;

	FQuat YawOffsetRotation = FRotator(0.f, SnapYawOffset, 0.f).Quaternion(); // 진짜 YawOffset만 든 값
	FQuat FinalSnapRotation = SocketWorldTransform.GetRotation() * YawOffsetRotation; // 을 적용시켜요
	FinalSnapRotation.Normalize();

	FTransform SafeSnapTransform(FinalSnapRotation, SocketWorldTransform.GetLocation(), FVector::OneVector);
	if (SafeSnapTransform.ContainsNaN())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 소켓 Transform이 유효하지 않습니다."));
		return;
	}

	// 서버가 알고 있는 플레이어 위치를 기준으로 거리 검사
	if (false == IsWithinServerPlacementDistance(SafeSnapTransform.GetLocation()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 허용 설치 거리를 벗어났습니다."));
		return;
	}

	// 설치Target Foundation은 의도적으로 맞닿으므로 제외하고(마지막 인자) 다른 건축 파츠 및 장애물과의 겹침을 검사해요
	if (true == HasServerPlacementOverlap(Definition, SafeSnapTransform, TargetPart->MeshComponent.Get(), TargetBuilding))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 외부 장애물과 겹치는 위치입니다."));
		return;
	}

	// 소켓의 월드 Transform을 -> 기존 BuildingActor 기준 상대 Transform으로 변환시켜요
	const FTransform RelativeTransform = SafeSnapTransform.GetRelativeTransform(TargetBuilding->GetActorTransform());
	if (RelativeTransform.ContainsNaN()) 
		return;

	// 새 BuildingActor를 만들지 않고 기존 건물에 해당 스냅 파츠를 추가해요
	UStaticMeshComponent* NewPart = TargetBuilding->AddPart(Definition, RelativeTransform);
	if (false == IsValid(NewPart))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 파츠 추가에 실패했습니다."));
		return;
	}

	// AddPart()가 PlacedParts 배열에 새 요소를 추가하면서 배열 메모리가 재할당될 수 있으므로,
	// 기존 TargetPart 포인터를 사용하지 않고 PartID로 다시 찾아요
	FPlacedBuildingPart* UpdatedTargetPart = TargetBuilding->FindPlacedPartByID(TargetPartID);
	if (nullptr == UpdatedTargetPart)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBuildingPlacementComponent::ServerPlaceSnappedPart] : 설치 후 대상 파츠 재검색에 실패했습니다."));
		return;
	}

	// 설치에 사용한 소켓을 사용된 상태로 기록
	UpdatedTargetPart->OccupiedSnapPoints.AddUnique(SocketName);

	// PlacedParts 변경을 클라이언트에 빠르게 전달
	TargetBuilding->ForceNetUpdate();

	UE_LOG(LogTemp, Log, TEXT(" [UBuildingPlacementComponent::ServerPlaceSnappedPart] : 스냅 파츠 설치 완료. Socket=%s"), *SocketName.ToString());
}

void UBuildingPlacementComponent::UpdateFoundationPreview(APlayerController* PlayerController)
{
	if (false == IsValid(PlayerController) || false == IsValid(PreviewActor))
	{
		HidePlacementPreview();
		return;
	}

	ClearCurrentSnapTarget();

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

		if (false == bVerticalHit)
		{
			HidePlacementPreview();
			return;
		}

		ShowPreviewAtLocation(VerticalHitResult.ImpactPoint, VerticalHitResult.ImpactNormal, VerticalHitResult.GetComponent());
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
		// 높이 고려 안 해요
		ShowPreviewAtLocation(ForwardHitResult.ImpactPoint, ForwardHitResult.ImpactNormal, ForwardHitResult.GetComponent());
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
		HidePlacementPreview();
		return;
	}

	// 최대 거리 지점의 카메라 LineTrace가 실제 지면에서 얼마나 떠있는지 계산
	float GroundZ = GroundHitResult.ImpactPoint.Z;
	float DesiredHeight = MaxDistancePoint.Z - GroundZ;

	// Foundation이 지면 아래로 내려가거나, 
	// Foundation의 다리기둥 길이보다 높게 뜨지 않도록 Clamp해버리기
	float FinalHeight = FMath::Clamp(DesiredHeight, 0.f, CurFoundationLegLength);

	FVector PreviewLocation(MaxDistancePoint.X, MaxDistancePoint.Y, GroundZ + FinalHeight);

	ShowPreviewAtLocation(PreviewLocation, GroundHitResult.ImpactNormal, GroundHitResult.GetComponent());

	PreviewActor->SetActorHiddenInGame(false);
}

bool UBuildingPlacementComponent::UpdateStructureSnapPreview(APlayerController* PlayerController)
{
	ClearCurrentSnapTarget();

	if (false == IsValid(PlayerController) || false == IsValid(SelectedDefinition) || false == IsValid(PreviewActor))
	{
		HidePlacementPreview();
		return false;
	}

	// 컨트롤러에서 플레이어 카메라가 보고 있는 현재 위치와 회전을 가져와요
	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FVector TraceEnd = ViewLocation + ViewRotation.Vector() * StructureSnapTraceDistance;

	// 플레이어와 프리뷰는 라인트레이스에서 제외시켜요
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;

	if (APawn* ControlledPawn = PlayerController->GetPawn())
		QueryParams.AddIgnoredActor(ControlledPawn);

	QueryParams.AddIgnoredActor(PreviewActor);

	// 라인트레이스 해서 플레이어가 바라보는 기존 건축 파츠를 찾아요
	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		ViewLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams);

	if (false == bHit)
	{
		HidePlacementPreview();
		return false;
	}

	// 트레이스 맞힌 액터가 BuildingActor인지 확인합니다
	ABuildingActor* HitBuilding = Cast<ABuildingActor>(HitResult.GetActor());
	if (false == IsValid(HitBuilding) || false == IsValid(HitResult.GetComponent()))
	{
		HidePlacementPreview();
		return false;
	}

	// 트레이스 맞힌 메시 컴포넌트의 건축 파츠가 있는지 확인
	const FPlacedBuildingPart* TargetPart = HitBuilding->FindPlacedPartByComponent(HitResult.GetComponent());
	if (nullptr == TargetPart || false == IsValid(TargetPart->Definition) || false == IsValid(TargetPart->MeshComponent))
	{
		HidePlacementPreview();
		return false;
	}

	// 조준 지점에서 가장 가까운 호환(AllowedPartTypes) 소켓을 찾아요
	bool bFoundSnapPoint = false;

	const float CurrentSnapPointSearchRadius =
		SelectedDefinition->PlacementType == EBuildingPlacementType::FOUNDATION ?
		FoundationSnapPointSearchRadius: SnapPointSearchRadius;

	float BestDistanceSquared = FMath::Square(SnapPointSearchRadius);
	FName BestSocketName = NAME_None;
	FTransform BestSocketTransform = FTransform::Identity;

	for (const FBuildingSnapPointDefinition& SnapPoint : TargetPart->Definition->SnapPoints)
	{
		// 이미 사용된 소켓이면 제외
		if (TargetPart->OccupiedSnapPoints.Contains(SnapPoint.SocketName))
			continue;

		FTransform CandidateTransform;

		// 소켓 존재 여부와 파츠 타입 호환성을 함께 검사
		if (false == HitBuilding->TryGetSnapPointWorldTransform(
				HitResult.GetComponent(), SelectedDefinition.Get(),
				SnapPoint.SocketName, CandidateTransform))
			continue;

		float DistanceSquared = FVector::DistSquared(HitResult.ImpactPoint, CandidateTransform.GetLocation());

		// 소켓 탐지 거리보다 크다면.. 제외
		if (DistanceSquared > BestDistanceSquared)
			continue;

		// 뭐라도 찾아냈어요
		bFoundSnapPoint = true;
		BestDistanceSquared = DistanceSquared;
		BestSocketName = SnapPoint.SocketName;
		BestSocketTransform = CandidateTransform;
	}

	if (false == bFoundSnapPoint) // 사실 못 찾았습니다
	{
		HidePlacementPreview();
		return false;
	}

	// 클라이언트에서도 최대 설치 수평거리 확인
	float PlacementDistanceSquared = FVector::DistSquared2D( ViewLocation, BestSocketTransform.GetLocation());
	if (PlacementDistanceSquared > FMath::Square(MaxPlacementDistance))
	{
		HidePlacementPreview();
		return false;
	}

	// 적용할 Yaw 회전값을 가져와 세팅된 값만이 담긴 회전 쿼터니언으로 만들었어요
	FTransform RotatedSnapTransform = BestSocketTransform;
	float SnapYawOffset = GetCurSnapYawOffset();
	FQuat YawOffsetRotation = FRotator(0.f, SnapYawOffset, 0.f).Quaternion();

	// BestSocketTransform의 회전에 적용시켜요
	FQuat FinalRotation = BestSocketTransform.GetRotation() * YawOffsetRotation;

	FinalRotation.Normalize();
	RotatedSnapTransform.SetRotation(FinalRotation);

	// 회전값까지 적용된 Transform을 프리뷰에 적용해 배치 !!
	PreviewActor->SetActorTransform(RotatedSnapTransform);

	// 대상 Foundation 컴포넌트는 이미 의도적으로 겹친 상태니까 겹침 검사 대상에서 제외시켜요
	const bool bHasOverlap = HasPlacementOverlap(HitResult.GetComponent(), HitBuilding);

	bCanPlace = false == bHasOverlap;

	PreviewActor->SetPlacementValid(bCanPlace);
	PreviewActor->SetActorHiddenInGame(false);

	// 설치 확정 됐으니 서버에 보낼 대상 정보 저장합니다ㅏ
	CurrentSnapBuilding = HitBuilding;
	CurrentSnapTargetPartID = TargetPart->PartID;
	CurrentSnapSocketName = BestSocketName;

	return true;
}

void UBuildingPlacementComponent::ClearCurrentSnapTarget()
{
	CurrentSnapBuilding.Reset();
	CurrentSnapTargetPartID.Invalidate();
	CurrentSnapSocketName = NAME_None;
}

void UBuildingPlacementComponent::HidePlacementPreview()
{
	bCanPlace = false;
	ClearCurrentSnapTarget();

	if (false == IsValid(PreviewActor)) return;

	PreviewActor->SetPlacementValid(false);
	PreviewActor->SetActorHiddenInGame(true);
}

void UBuildingPlacementComponent::ServerPlaceNewBuilding_Implementation(UBuildingPartDefinition* Definition, const FTransform& InPlacementTransform)
{
	if (false == IsValid(Definition))
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerPlaceNewBuilding : Definition이 유효하지 않습니다."));
		return;
	}

	if (nullptr == BuildingActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerPlaceNewBuilding : BuildingActorClass가 설정되지 않았습니다."));
		return;
	}

	APlayerController* OwnerController = Cast<APlayerController>(GetOwner());
	APawn* OwnerPawn = (true == IsValid(OwnerController)) ? OwnerController->GetPawn() : nullptr;

	// 비정상적인 Transform이 사용되는 걸 방지해요
	if (InPlacementTransform.ContainsNaN())
	{
		UE_LOG(LogTemp, Warning,TEXT("ServerPlaceNewBuilding : 유효하지 않은 Transform 요청입니다."));
		return;
	}

	// 건축 위치가 최대 설치 거리(수평 기준) 안인지 검사해요
	const FVector PlacementLocation = InPlacementTransform.GetLocation();
	if (false == IsWithinServerPlacementDistance(PlacementLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerPlaceNewBuilding : 허용 거리를 벗어난 설치 요청입니다. Location=%s"),*PlacementLocation.ToString());
		return;
	}

	// 클라이언트가 전달한 임의의 Scale은 사용하지 않습니다
	const FTransform SafePlacementTransform(InPlacementTransform.GetRotation(), InPlacementTransform.GetLocation(), FVector::OneVector);
	
	// PlacementType에 해당하는 서버 검증
	if (false == IsServerPlacementRuleValid(Definition, SafePlacementTransform)) return;
	
	if (true == HasServerPlacementOverlap(Definition, SafePlacementTransform))
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerPlaceNewBuilding: 장애물과 겹치는 설치 요청입니다."));
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerController; // 이 건물을 설치한 PlayerController
	SpawnParameters.Instigator = OwnerPawn; // 이 건물을 설치한 Pawn
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 클라이언트가 보낸 Scale은 사용하지 않음
	//const FTransform SafePlacementTransform(
	//	InPlacementTransform.GetRotation(), InPlacementTransform.GetLocation(), FVector::OneVector);

	ABuildingActor* NewBuilding = GetWorld()->SpawnActor<ABuildingActor>(
			BuildingActorClass, SafePlacementTransform, SpawnParameters);

	if (false == IsValid(NewBuilding))
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerPlaceNewBuilding : BuildingActor 생성에 실패했습니다."));
		return;
	}

	// BuildingActor 자체가 프리뷰 위치에 있으므로 첫 파츠는 건축 껍데기 액터의 원점에 배치
	UStaticMeshComponent* NewPart = NewBuilding->AddPart(Definition, FTransform::Identity);

	if (false == IsValid(NewPart))
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerPlaceNewBuilding : 첫 건축 파츠 추가에 실패했습니다."));

		NewBuilding->Destroy(); // 파츠가 없는 빈껍데기 액터를 남기지 않음
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ServerPlaceNewBuilding: 건축물 설치 완료. Location=%s"), *SafePlacementTransform.GetLocation().ToString());
}

bool UBuildingPlacementComponent::HasPlacementOverlap( const UPrimitiveComponent* SupportingComponent, const ABuildingActor* IgnoredBuilding) const
{
	if ((false == IsValid(SelectedDefinition)) || false == IsValid(SelectedDefinition->PartMesh)) return true;

	const UStaticMeshComponent* PreviewMeshComponent = PreviewActor->GetPreviewMeshComponent();

	if ((false == IsValid(PreviewMeshComponent)) || (false == IsValid(PreviewMeshComponent->GetStaticMesh()))) return true;

	// 이러한 종류의 물체들과 충돌 검사를 합니다
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	// Preview Actor는 충돌 검사에서 제외시켜요
	FComponentQueryParams QueryParams(SCENE_QUERY_STAT(BuildingPlacementOverlap), PreviewActor);

	// 자기 플레이어도 충돌 검사에서 제외
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetOwner()))
	{
		if (const APawn* ControlledPawn = PlayerController->GetPawn())
			QueryParams.AddIgnoredActor(ControlledPawn);
	}

	// Foundation을 받치는 지면 컴포넌트도 검사 대상에서 제외!
	// 왜냐면요 Foundation 다리는 지면과 겹칠 수 있기 때문에..
	if (true == IsValid(SupportingComponent))
		QueryParams.AddIgnoredComponent(SupportingComponent);

	// 유효한 스냅 소켓이 제공하는 위치는 무!조!건! 신뢰하는 걸로 하기 때문에
	// 해당 소켓을 소유한 BuildingActor 내부 파츠들은 겹침 장애물에서 제외시켜요
	if (true == IsValid(IgnoredBuilding))
		QueryParams.AddIgnoredActor(IgnoredBuilding);

	// 진짜 충돌 검사
	TArray<FOverlapResult> OverlapResults;
	GetWorld()->ComponentOverlapMulti(
		OverlapResults, // 발견된 결과 받는 배열
		PreviewMeshComponent, // 충돌 검사에 사용할 콜리전 모양
		PreviewMeshComponent->GetComponentLocation(),
		PreviewMeshComponent->GetComponentQuat(),
		QueryParams, // 충돌 검사 제외 목록
		ObjectQueryParams // 충돌 검사 Object Type 목록
	);

	return OverlapResults.Num() > 0;
}

void UBuildingPlacementComponent::ShowPreviewAtLocation(const FVector& InPreviewLocation, const FVector& InSurfaceNormal, UPrimitiveComponent* SupportingComponent)
{
	if (false == IsValid(PreviewActor)) return;

	// 프리뷰 액터 위치 변경
	PreviewActor->SetActorLocation(InPreviewLocation);

	// 현재 맞힌 곳이 실제 설치 가능한 지면(옵젝타입:BuildableGround)인지 검사해서 결과를 얻어요
	const bool bIsSurfaceValid = IsBuildableSurface(SelectedDefinition.Get(), SupportingComponent);

	// 지형 경사 각도 검사를 해서 결과를 얻어요
	const bool bIsSlopeValid = IsGroundSlopeValid(SelectedDefinition.Get(), InSurfaceNormal);

	// 오브젝트 겹침 검사해서 결과를 얻어요
	const bool bHasOverlap = HasPlacementOverlap(SupportingComponent);

	bCanPlace = (bIsSurfaceValid) && (bIsSlopeValid) && (!bHasOverlap);
	
	PreviewActor->SetPlacementValid(bCanPlace); // 여기서 PreviewActor Valid/Invalid 머터리얼 세팅해요
	PreviewActor->SetActorHiddenInGame(false);
}

bool UBuildingPlacementComponent::IsBuildableSurface(const UBuildingPartDefinition* Definition, const UPrimitiveComponent* SupportingComponent) const
{
	if (false == IsValid(Definition)) return false;

	// 건축 파츠의 배치 규칙을 가져옴
	const FGroundPlacementRule& GroundRule = Definition->GroundPlacementRule;

	//if (false == GroundRule.bEnabled) return true; // 경사를 신경 안 쓰니 true. 무조건 배치 가능해요

	// 라인트레이스가 유효한 컴포넌트를 맞히지 못했다면 설치 불가!
	if (false == IsValid(SupportingComponent)) return false;

	// 맞힌 컴포넌트가 BuildableGround 옵젝 타입인지 확인하고 그 결과를 리턴해요
	return (SupportingComponent->GetCollisionObjectType() == ECC_BUILDABLEGROUND);
}

bool UBuildingPlacementComponent::IsGroundSlopeValid(const UBuildingPartDefinition* Definition, const FVector& InSurfaceNormal)
{
	// 인자 InSurfaceNormal은 HitResult.ImpactNormal이 들어와요 (ShowPreviewAtLocation()로 넘겨옴)
	// ImpactNormal : 표면을 기준으로 수직으로 뻗어나오는 방향
	// 표면이 기울수록 노말도 옆으로 기울어짐

	if (false == IsValid(Definition)) return false;

	// 건축 파츠의 배치 규칙을 가져옴
	const FGroundPlacementRule& GroundRule = Definition->GroundPlacementRule;

	//if (false == GroundRule.bEnabled) return true; // 경사를 신경 안 쓰니 true. 무조건 배치 가능해요

	if (InSurfaceNormal.IsNearlyZero()) return false; // 0에 가까우면 방향이 없다? 각도 계산 못 해요 배치 불가

	FVector NormalizedSurfaceNormal = InSurfaceNormal.GetSafeNormal();

	// 표면 노멀벡터와 업벡터를 내적시키면 표면 노멀과 월드 위쪽 방향 사이의 각도를 알 수 있어요
	// 표면의 노멀과 업벡터가 가까워질수록 평지에 가까운 것!!! 

	// 내적 결과는 그 사이 각도의 cos값
	// 부동 소수점 오차 때문에 내적 결과에 오차가 생김 --> Acos의 계산 범위 때문에 Clamp로 감쌈
	float NormalDotUp = FMath::Clamp(
		FVector::DotProduct(NormalizedSurfaceNormal, FVector::UpVector), -1.f, 1.f);

	// NormalDotUp은 cos값이라 Acos로 진짜 각도로 추출
	float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(NormalDotUp));

	return SlopeAngle <= GroundRule.MaxSlopeAngle;
}

bool UBuildingPlacementComponent::IsLocalPlacementController() const
{
	APlayerController* OwnerController = Cast<APlayerController>(GetOwner());

	return (true == IsValid(OwnerController)) && OwnerController->IsLocalController();
}

bool UBuildingPlacementComponent::IsWithinServerPlacementDistance(const FVector& InPlacementLocation) const
{
	const APlayerController* OwnerController = Cast<APlayerController>(GetOwner());
	if (false == IsValid(OwnerController)) return false;

	const APawn* OwnerPawn = OwnerController->GetPawn();
	if (false == IsValid(OwnerPawn)) return false;

	const FVector ViewLocation = OwnerPawn->GetPawnViewLocation(); // 서버가 알고 있는 Pawn의 시점 위치

	const float AllowedDistance = MaxPlacementDistance + ServerPlacementDistanceTolerance;

	// 서버가 알고 있는 Pawn의 시점 위치를 기준으로 요청받은 건축 위치가 최대 설치 거리(수평 기준) 안인지 검사해요
	return FVector::DistSquared2D(ViewLocation, InPlacementLocation) <= FMath::Square(AllowedDistance);
}

bool UBuildingPlacementComponent::FindSupportingGround(const UBuildingPartDefinition* Definition, const FVector& InPlacementLocation, FHitResult& OutGroundHit) const
{
	if (false == IsValid(Definition) || false == IsValid(Definition->PartMesh)) return false;

	// 메시의 가장 낮은 Z를 구합니다
	FBoxSphereBounds MeshBounds = Definition->PartMesh->GetBounds();
	float LowestLocalZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
	float GroundReach = FMath::Max(0.f, -LowestLocalZ); // 파츠 기준점에서 다리 끝까지 지면에 도달할 수 있는 순수 길이

	// 파츠의 기준점보다 약간 위에서 시작해
	// 다리 끝보다 약간 아래까지의 지면을 탐색해요
	FVector TraceStart = InPlacementLocation + FVector::UpVector * 10.f; // 10은 지면<->파츠 작은 높이 오차값? 변수로 변경을 해야 하는데..
	FVector TraceEnd = InPlacementLocation - FVector::UpVector * (GroundReach + 10.f);

	// 옵젝타입 BuildableGround만 지면으로 인정하기로 해요..
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_BUILDABLEGROUND);
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ServerPlacementGroundTrace), false);

	if (APlayerController* OwnerController = Cast<APlayerController>(GetOwner()))
	{
		if (APawn* OwnerPawn = OwnerController->GetPawn())
			QueryParams.AddIgnoredActor(OwnerPawn);
	}

	return GetWorld()->LineTraceSingleByObjectType(OutGroundHit, TraceStart, TraceEnd, ObjectQueryParams, QueryParams);
}

UStaticMeshComponent* UBuildingPlacementComponent::GetOrCreateServerValidationMesh()
{
	if (true == IsValid(ServerValidationMeshComponent)) return ServerValidationMeshComponent; // 있으면 재사용, 없으면 만들어용

	AActor* OwnerActor = GetOwner();
	if (false == IsValid(OwnerActor) || false == OwnerActor->HasAuthority()) return nullptr;

	ServerValidationMeshComponent = NewObject<UStaticMeshComponent>(OwnerActor);

	if (false == IsValid(ServerValidationMeshComponent)) return nullptr;

	// 서버 내부 검사 전용이므로 복제 안 함!
	ServerValidationMeshComponent->SetIsReplicated(false);

	// 화면에도 안 보이게
	ServerValidationMeshComponent->SetVisibility(false);
	ServerValidationMeshComponent->SetHiddenInGame(true);

	// 내비게이션에 영향을 주지 않음
	ServerValidationMeshComponent->SetCanEverAffectNavigation(false);
	ServerValidationMeshComponent->SetMobility(EComponentMobility::Movable);

	// 평소에는 노콜리전
	ServerValidationMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 검사할 때 접촉 결과를 얻기 위한 설정
	ServerValidationMeshComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
	ServerValidationMeshComponent->SetGenerateOverlapEvents(false);

	// 렌더링을 숨기지만
	// 충돌을 Query Only로 바꿀 때 Physics State를 사용할 수 있도록 월드에 등록을 해야 해요..
	OwnerActor->AddInstanceComponent(ServerValidationMeshComponent);
	ServerValidationMeshComponent->RegisterComponent();

	return ServerValidationMeshComponent;
}

bool UBuildingPlacementComponent::HasServerPlacementOverlap(const UBuildingPartDefinition* Definition, const FTransform& InPlacementTransform, const UPrimitiveComponent* IgnoredSupportingComponent, const ABuildingActor* IgnoredBuilding)
{
	if (false == IsValid(Definition) || false == IsValid(Definition->PartMesh)) return true;

	UStaticMeshComponent* ValidationMesh = GetOrCreateServerValidationMesh();
	if (false == IsValid(ValidationMesh)) return true;

	// 클라가 요청한 위치에 해당 파츠를 임시로 놔봐요 (눈에 보이진 않음)
	ValidationMesh->SetStaticMesh(Definition->PartMesh);
	ValidationMesh->SetWorldTransform(InPlacementTransform);

	// 콜리전 Physics State를 생성하기 위해 검사할 때만 QueryOnly로 잠시!! 변경해요
	ValidationMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 장애물로 취급할 오브젝트 종류
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FComponentQueryParams QueryParams;

	// 스냅 대상 파츠는 의도적으로 닿는 것이기에 겹침 겹사에서는 제외
	if (true == IsValid(IgnoredSupportingComponent))
		QueryParams.AddIgnoredComponent(IgnoredSupportingComponent);

	// 서버가 검증한 소켓 위치에 추가되는 파츠니까 같은 건물을 구성하는 기존 파츠와의 의도된 겹침은 허용하기로 해요.
	if (true == IsValid(IgnoredBuilding))
		QueryParams.AddIgnoredActor(IgnoredBuilding);

	AActor* OwnerActor = GetOwner();
	if (true == IsValid(OwnerActor))
		QueryParams.AddIgnoredActor(OwnerActor); // 자기 자신 제외

	if (APlayerController* OwnerController = Cast<APlayerController>(OwnerActor))
	{
		if (APawn* OwnerPawn = OwnerController->GetPawn())
			QueryParams.AddIgnoredActor(OwnerPawn); // 자기 자신 제외
	}

	// 로컬 프리뷰가 서버 월드에 있으므로 얘도 제외
	if (true == IsValid(PreviewActor))
		QueryParams.AddIgnoredActor(PreviewActor);

	// ValidationMesh를 InPlacementTransform 위치에 놓았을 때 겹치는 게 있는지요?
	TArray<FOverlapResult> OverlapResults;
	GetWorld()->ComponentOverlapMulti( 
		OverlapResults, ValidationMesh,
		InPlacementTransform.GetLocation(), InPlacementTransform.GetRotation(), 
		QueryParams, ObjectQueryParams);

	// 실제 배치될 위치 겹침 검사가 끝났으므로 다시 노콜리전으로 변경
	ValidationMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* OverlappedActor = OverlapResult.GetActor();
		UPrimitiveComponent* OverlappedComponent = OverlapResult.GetComponent();
		
		// 다른 로컬 프리뷰까지 장애물로 판단하지 않도록.. 모든 PreviewActor 제외	
		if (true == IsValid(OverlappedActor) && OverlappedActor->IsA<ABuildingPreviewActor>())
			continue;

		UE_LOG(LogTemp, Warning, TEXT("서버 배치 겹침 감지 | Actor=%s | Component=%s"),
			*GetNameSafe(OverlappedActor), *GetNameSafe(OverlappedComponent));

		return true;
	}
	return false;
}

bool UBuildingPlacementComponent::IsServerPlacementRuleValid(const UBuildingPartDefinition* Definition, const FTransform& InPlacementTransform)
{
	if (false == IsValid(Definition)) return false;

	switch (Definition->PlacementType)
	{
	case EBuildingPlacementType::FOUNDATION:
		return IsServerFoundationPlacementValid(Definition, InPlacementTransform);

	case EBuildingPlacementType::STRUCTURE_SNAP:
		// 스냅 시스템 구현 전까지 서버에서 설치를 거부할게요
		UE_LOG(LogTemp, Warning, TEXT("[IsServerPlacementRuleValid] StructureSnap 검사는 아직 구현되지 않았습니다."));
		return false;

	case EBuildingPlacementType::SURFACE:
		UE_LOG(LogTemp, Warning, TEXT("[IsServerPlacementRuleValid] Surface 검사는 아직 구현되지 않았습니다."));
		return false;

	case EBuildingPlacementType::ATTACHMENT:
		UE_LOG(LogTemp, Warning,TEXT("[IsServerPlacementRuleValid] Attachment 검사는 아직 구현되지 않았습니다."));
		return false;

	default:
		return false;
	}
}

bool UBuildingPlacementComponent::IsServerFoundationPlacementValid(const UBuildingPartDefinition* Definition, const FTransform& InPlacementTransform)
{
	FHitResult GroundHit;

	//  지면이 다리가 닿는 범위 안에 있는지 검사해요
	if (false == FindSupportingGround(Definition, InPlacementTransform.GetLocation(), GroundHit))
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerPlaceNewBuilding : 파츠를 지지할 지면이 없습니다."));
		return false;
	}

	// 건축이 ㄱㅏ능한 지형인지 검사해요
	if (false == IsBuildableSurface(Definition, GroundHit.GetComponent()))
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerPlaceNewBuilding : BuildableGround가 아닌 표면입니다."));
		return false;
	}

	// 지면 ImpactNormal을 이용해 파츠 배치 최대 경사각을 넘는지 검사해요
	if (false == IsGroundSlopeValid(Definition, GroundHit.ImpactNormal))
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerPlaceNewBuilding: 허용되지 않는 지면 경사입니다."));
		return false;
	}

	return true;
}

void UBuildingPlacementComponent::CycleSnapYawOffset()
{
	if (false == IsValid(SelectedDefinition)) return;

	const int32 OffsetCnt = SelectedDefinition->AllowedSnapYawOffsets.Num();

	if (OffsetCnt <= 0)
	{
		CurSnapYawOffsetIdx = 0;
		return;
	}

	// 배열의 마지막 인덱스에서 다시 배열의 0번째 인덱스로 돌아가요
	CurSnapYawOffsetIdx = (CurSnapYawOffsetIdx + 1) % OffsetCnt;
}

float UBuildingPlacementComponent::GetCurSnapYawOffset()
{
	if (false == IsValid(SelectedDefinition)) return 0.f;
	if (false == SelectedDefinition->AllowedSnapYawOffsets.IsValidIndex(CurSnapYawOffsetIdx)) return 0.f;

	return SelectedDefinition->AllowedSnapYawOffsets[CurSnapYawOffsetIdx];
}
