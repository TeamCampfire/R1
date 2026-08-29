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

	if ((false == bIsPlacing) || (nullptr == SelectedDefinition))
		return;

	// 서버가 가진 원격 PlayerController 사본은 로컬 프리뷰 연산도 하면 안됩니다
	if (false == IsLocalPlacementController()) return;

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
		{
			ShowPreviewAtLocation(VerticalHitResult.ImpactPoint, VerticalHitResult.ImpactNormal, VerticalHitResult.GetComponent());
			PreviewActor->SetActorHiddenInGame(false);
		}
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
		// 높이 고려 안 해요
		ShowPreviewAtLocation(ForwardHitResult.ImpactPoint, ForwardHitResult.ImpactNormal, ForwardHitResult.GetComponent());
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
	ShowPreviewAtLocation(PreviewLocation, GroundHitResult.ImpactNormal, GroundHitResult.GetComponent());

	PreviewActor->SetActorHiddenInGame(false);
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

	if (nullptr == BuildingActorClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[UBuildingPlacementComponent::ConfirmPlacement()] : BuildingActorClass가 설정되지 않았습니다."));
		return;
	}

	//UE_LOG(LogTemp, Log, TEXT("ConfirmPlacement: 설치 가능 위치입니다. Location=%s"), *PreviewActor->GetActorLocation().ToString());

	const FTransform PlacementTransform = PreviewActor->GetActorTransform(); // 프리뷰의 트랜스폼 가져오기
	ServerPlaceNewBuilding(SelectedDefinition.Get(), PlacementTransform); // 실제 건축물 생성은 서버에 요청해요
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

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerController; // 이 건물을 설치한 PlayerController
	SpawnParameters.Instigator = OwnerPawn; // 이 건물을 설치한 Pawn
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 클라이언트가 보낸 Scale은 사용하지 않음
	const FTransform SafePlacementTransform(
		InPlacementTransform.GetRotation(), InPlacementTransform.GetLocation(), FVector::OneVector);

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

bool UBuildingPlacementComponent::HasPlacementOverlap( const UPrimitiveComponent* SupportingComponent) const
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
	const bool bIsSurfaceValid = IsBuildableSurface(SupportingComponent);

	// 지형 경사 각도 검사를 해서 결과를 얻어요
	const bool bIsSlopeValid = IsGroundSlopeValid(InSurfaceNormal);

	// 오브젝트 겹침 검사해서 결과를 얻어요
	const bool bHasOverlap = HasPlacementOverlap(SupportingComponent);
	
	bCanPlace = (bIsSurfaceValid) && (bIsSlopeValid) && (!bHasOverlap);

	PreviewActor->SetPlacementValid(bCanPlace); // 여기서 PreviewActor Valid/Invalid 머터리얼 세팅해요
	PreviewActor->SetActorHiddenInGame(false);
}

bool UBuildingPlacementComponent::IsBuildableSurface(const UPrimitiveComponent* SupportingComponent) const
{
	if (false == IsValid(SelectedDefinition)) return false;

	// 건축 파츠의 배치 규칙을 가져옴
	const FGroundPlacementRule& GroundRule = SelectedDefinition->GroundPlacementRule;

	if (false == GroundRule.bEnabled) return true; // 경사를 신경 안 쓰니 true. 무조건 배치 가능해요

	// 라인트레이스가 유효한 컴포넌트를 맞히지 못했다면 설치 불가!
	if (false == IsValid(SupportingComponent)) return false;

	// 맞힌 컴포넌트가 BuildableGround 옵젝 타입인지 확인하고 그 결과를 리턴해요
	return (SupportingComponent->GetCollisionObjectType() == ECC_BUILDABLEGROUND);
}

bool UBuildingPlacementComponent::IsGroundSlopeValid(const FVector& InSurfaceNormal)
{
	// 인자 InSurfaceNormal은 HitResult.ImpactNormal이 들어와요 (ShowPreviewAtLocation()로 넘겨옴)
	// ImpactNormal : 표면을 기준으로 수직으로 뻗어나오는 방향
	// 표면이 기울수록 노말도 옆으로 기울어짐

	if (false == IsValid(SelectedDefinition)) return false;

	// 건축 파츠의 배치 규칙을 가져옴
	const FGroundPlacementRule& GroundRule = SelectedDefinition->GroundPlacementRule;

	if (false == GroundRule.bEnabled) return true; // 경사를 신경 안 쓰니 true. 무조건 배치 가능해요

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
