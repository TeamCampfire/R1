#include "BuildingSystem/BuildingActor.h"
#include "Data/Building/BuildingPartDefinition.h"

ABuildingActor::ABuildingActor()
{
 	PrimaryActorTick.bCanEverTick = false;

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

void ABuildingActor::AddPart(UBuildingPartDefinition* Definition, const FTransform& InRelativeTransform)
{
	if (false == IsValid(Definition) || false == IsValid(Definition->PartMesh))
	{
		UE_LOG(LogTemp, Log, TEXT(" [ABuildingActor::AddPart] : Is Not Valid"));
		return;
	}

	//! 파트 메시 컴포넌트 생성 및 등록
	UStaticMeshComponent* NewPart = NewObject<UStaticMeshComponent>(this);
	AddInstanceComponent(NewPart);

	NewPart->SetStaticMesh(Definition->PartMesh);
	NewPart->SetupAttachment(RootComponent);
	NewPart->SetRelativeTransform(InRelativeTransform);
	NewPart->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NewPart->RegisterComponent();

	PartComponents.Add(NewPart);	
}