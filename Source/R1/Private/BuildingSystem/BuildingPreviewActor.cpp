#include "BuildingSystem/BuildingPreviewActor.h"
#include "Data/Building/BuildingPartDefinition.h"

ABuildingPreviewActor::ABuildingPreviewActor()
{
 	PrimaryActorTick.bCanEverTick = false;

	// Root Component 생성
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bReplicates = false; // 액터를 서버에서 다른 클라로 복제 안할거예요
}

void ABuildingPreviewActor::BeginPlay()
{
	Super::BeginPlay();
}

void ABuildingPreviewActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABuildingPreviewActor::SetDefinition(UBuildingPartDefinition* Definition)
{
	PreviewDefinition = Definition;

	if (false == IsValid(Definition))
	{
		MeshComponent->SetStaticMesh(nullptr);
		return;
	}

	MeshComponent->SetStaticMesh(PreviewDefinition->PartMesh);
}
