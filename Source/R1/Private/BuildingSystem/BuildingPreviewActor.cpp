#include "BuildingSystem/BuildingPreviewActor.h"
#include "Data/Building/BuildingPartDefinition.h"

#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	SetPlacementValid(bIsPlacementValid);
}

void ABuildingPreviewActor::SetPlacementValid(bool bIsValid)
{
	bIsPlacementValid = bIsValid;
	if (false == IsValid(MeshComponent)) return;

	// 설치 가능 상태에 따라 머티리얼 갈아끼우기
	UMaterialInterface* CurMaterial = bIsPlacementValid ? ValidPreviewMtrl : InvalidPreviewMtrl;
	if (false == IsValid(CurMaterial)) return;

	// 메시가 가진 모든 머티리얼 슬롯에 적용
	const int32 MaterialSlotCount = MeshComponent->GetNumMaterials();

	for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
	{
		MeshComponent->SetMaterial(SlotIndex, CurMaterial);
	}
}
