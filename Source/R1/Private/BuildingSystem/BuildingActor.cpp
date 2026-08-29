#include "BuildingSystem/BuildingActor.h"
#include "Data/Building/BuildingPartDefinition.h"

ABuildingActor::ABuildingActor()
{
 	PrimaryActorTick.bCanEverTick = false;

	// 서버에서 생성한 건물을 클라이언트에도 생성
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

UStaticMeshComponent* ABuildingActor::AddPart(UBuildingPartDefinition* Definition, const FTransform& InRelativeTransform)
{
	if (false == IsValid(Definition) || false == IsValid(Definition->PartMesh))
	{
		UE_LOG(LogTemp, Log, TEXT(" [ABuildingActor::AddPart] : Is Not Valid"));
		return nullptr;
	}

	//! 파트 메시 컴포넌트 생성 및 등록
	UStaticMeshComponent* NewPart = NewObject<UStaticMeshComponent>(this);

	if (false == IsValid(NewPart))
		return nullptr;
	
	NewPart->SetIsReplicated(true); // 서버에서 생성한 동적 컴포넌트를 클라이언트에도 생성

	AddInstanceComponent(NewPart);
	NewPart->SetStaticMesh(Definition->PartMesh);
	NewPart->SetupAttachment(RootComponent);
	NewPart->SetRelativeTransform(InRelativeTransform);
	NewPart->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NewPart->RegisterComponent();

	PartComponents.Add(NewPart);	

	return NewPart;
}