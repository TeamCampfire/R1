#include "BuildingSystem/BuildingActor.h"
#include "Data/Building/BuildingPartDefinition.h"
#include "Net/UnrealNetwork.h"
ABuildingActor::ABuildingActor()
{
 	PrimaryActorTick.bCanEverTick = false;

	// 서버에서 생성한 건물을 클라이언트에도 생성해야 하니까.. 복제 true
	bReplicates = true;

	// 건물 껍데기 액터의 Transform 복제
	SetReplicateMovement(true);

	// Root Component 생성
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ABuildingActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABuildingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABuildingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABuildingActor, PlacedParts); // PlacedParts는 변경될 때 마다 계속 클라에 복제할게요
}

UStaticMeshComponent* ABuildingActor::AddPart(UBuildingPartDefinition* Definition, const FTransform& InRelativeTransform)
{
	// 파츠 생성과 PartID 발급은 서버에서만 수행
	if (false == HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::AddPart] : 서버에서만 파츠를 추가할 수 있습니다."));
		return nullptr;
	}

	if (false == IsValid(Definition) || false == IsValid(Definition->PartMesh))
	{
		UE_LOG(LogTemp, Log, TEXT(" [ABuildingActor::AddPart] : Is Not Valid"));
		return nullptr;
	}

	//! 파트 메시 컴포넌트 생성 및 등록
	UStaticMeshComponent* NewPart = NewObject<UStaticMeshComponent>(this);

	if (false == IsValid(NewPart))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::AddPart] : 메시 컴포넌트 생성에 실패했습니다."));
		return nullptr;
	}
	

	AddInstanceComponent(NewPart);
	NewPart->SetStaticMesh(Definition->PartMesh);
	NewPart->SetupAttachment(RootComponent);
	NewPart->SetRelativeTransform(InRelativeTransform);
	NewPart->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NewPart->SetIsReplicated(true); // 서버에서 생성한 동적 컴포넌트를 클라이언트에도 생성
	NewPart->RegisterComponent(); // 액터 소유 인스턴스 컴포넌트로 등록

	if (false == NewPart->IsRegistered())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::AddPart] : 메시 컴포넌트 등록에 실패했습니다."));

		NewPart->DestroyComponent();
		return nullptr;
	}

	// 설치 완료 파츠 정보 기록하기
	FPlacedBuildingPart PlacedPart;
	PlacedPart.PartID = FGuid::NewGuid();
	PlacedPart.Definition = Definition;
	PlacedPart.RelativeTransform = NewPart->GetRelativeTransform();
	PlacedPart.CurDurability = Definition->MaxDurability;
	PlacedPart.MeshComponent = NewPart;

	PlacedParts.Add(MoveTemp(PlacedPart));

	UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::AddPart] Authority: %s / PlacedParts Num: %d"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"), PlacedParts.Num());

	return NewPart;
}

const FPlacedBuildingPart* ABuildingActor::FindPlacedPartByComponent(const UPrimitiveComponent* InComponent) const
{
	if (false == IsValid(InComponent)) return nullptr;

	// 들고 있는 파츠들을 순회하면서 같은 파츠가 있다면 반환
	for (const FPlacedBuildingPart& PlacedPart : PlacedParts)
	{
		if (PlacedPart.MeshComponent.Get() == InComponent)
			return &PlacedPart;
	}

	return nullptr;
}

FPlacedBuildingPart* ABuildingActor::FindPlacedPartByComponent(UPrimitiveComponent* InComponent)
{
	if (false == IsValid(InComponent)) return nullptr;

	for (FPlacedBuildingPart& PlacedPart : PlacedParts)
	{
		if (PlacedPart.MeshComponent.Get() == InComponent)
			return &PlacedPart;
	}

	return nullptr;
}

bool ABuildingActor::TryGetSnapPointWorldTransform(const UPrimitiveComponent* TargetComponent, const UBuildingPartDefinition* IncomingDefinition, const FName& InSocketName, FTransform& OutWorldTransform) const
{
	// Wall 설치할 때 라인트레이스가 걸려서 이 함수로 들어온 상태

	OutWorldTransform = FTransform::Identity;

	// 맞힌 Foundation이 있는지 ?, 설치하려는 데이터가 있는지?, SocketName이 올바른지?
	if (false == IsValid(TargetComponent) || false == IsValid(IncomingDefinition) || InSocketName.IsNone()) return false;

	// 라인트레이스 맞힌 메시 컴포넌트에 해당하는 건축 파츠가 있는지 확인해요
	// 어디에 세울 지에 대한 그 건축 파츠.. 
	const FPlacedBuildingPart* TargetPart = FindPlacedPartByComponent(TargetComponent);

	// 세울려고 했던 곳에 있던 건축 파츠가 온전한지 확인해요
	if (nullptr == TargetPart || false == IsValid(TargetPart->Definition) || false == IsValid(TargetPart->MeshComponent)) return false;

	// 인자로 들어온 소켓이 세울려고 했던 곳에 있던 건축 파츠의 Definition에 등록된 소켓인지 확인해요
	const FBuildingSnapPointDefinition* SnapPoint = nullptr;
	for (const FBuildingSnapPointDefinition& Candidate : TargetPart->Definition->SnapPoints)
	{
		if (Candidate.SocketName == InSocketName)
		{
			SnapPoint = &Candidate;
			break;
		}
	}

	if (nullptr == SnapPoint) return false;

	// 해당 소켓이 설치하려는 파츠 종류를 허용하는지 확인해요
	if (false == SnapPoint->AllowedPartTypes.Contains(IncomingDefinition->PartType)) return false;

	// Definition에 이름만 등록하고 실제 메시에는 소켓을 만들지 않은 설정 실수를 방지!@!!
	if (false == TargetPart->MeshComponent->DoesSocketExist(InSocketName))
	{
		UE_LOG(LogTemp, Log, TEXT("[ABuildingActor::TryGetSnapPointWorldTransform] : 메시에 소켓을 만들어두지 않은 것 같습니다."));
		return false;
	}

	// Static Mesh에 든 Socket의 로컬 Transform을 (현재 메시 컴포넌트 기준) 월드 Transform으로 변환해요
	OutWorldTransform = TargetPart->MeshComponent->GetSocketTransform(InSocketName, RTS_World);

	// true 반환 조건
	// 맞힌 컴포넌트가 실제 설치 파츠임
	// -> 대상 Definition에 Wall_PosX가 등록됨
	// -> Wall_PosX가 Wall 타입을 허용함
	// -> 실제 Foundation 메시에도 Wall_PosX 소켓이 존재함
	// -> 소켓의 월드 Transform 획득 성공
	return true;
}

FPlacedBuildingPart* ABuildingActor::FindPlacedPartByID(const FGuid& InPartID)
{
	if (false == InPartID.IsValid()) return nullptr;

	for (FPlacedBuildingPart& _placedPart : PlacedParts)
	{
		if (InPartID == _placedPart.PartID)
			return &_placedPart; // 찾았어요
	}

	return nullptr;
}

bool ABuildingActor::HasFoundationAtTransform(const FTransform& InWorldTransform, float LocationTolerance) const
{
	const float LocationToleranceSquared = FMath::Square(LocationTolerance); // 오차 허용 및 미리 제곱

	for (const FPlacedBuildingPart& PlacedPart : PlacedParts)
	{
		if (false == IsValid(PlacedPart.Definition) || false == IsValid(PlacedPart.MeshComponent))
			continue; // 유효하지 않은 거 제외

		if (PlacedPart.Definition->PlacementType != EBuildingPlacementType::FOUNDATION)
			continue; // Foundation 아니면 제외

		const FVector ExistingLocation = PlacedPart.MeshComponent->GetComponentLocation();
		bool bSameLocation = FVector::DistSquared(ExistingLocation, InWorldTransform.GetLocation()) <= LocationToleranceSquared;

		// Foundation은 같은 위치라면 회전과 관계없이(삼각 Foundation떄문에) 이미 점유된 칸
		if (true == bSameLocation) return true;
	}
	return false;
}
