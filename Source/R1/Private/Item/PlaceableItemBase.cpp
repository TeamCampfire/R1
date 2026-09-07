// 작업 시작일 : 9/6
// 작업자 : 우진

#include "Item/PlaceableItemBase.h"
#include "Net/UnrealNetwork.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Data/Building/BuildingPartDefinition.h"
#include "Data/Item/PlaceableItemData.h"

APlaceableItemBase::APlaceableItemBase()
{
 	PrimaryActorTick.bCanEverTick = false;

	// 서버 관련 세팅
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceableMesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
}

void APlaceableItemBase::BeginPlay()
{
	Super::BeginPlay();
}

void APlaceableItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APlaceableItemBase, PlaceableItemData);
}

void APlaceableItemBase::InitializePlaceable(UPlaceableItemData* InPlaceableItemData)
{
	// 설치 결과물의 초기화는 서버에서만 진행해요
	if (false == HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[APlaceableActorBase::InitializePlaceable] 서버에서만 초기화할 수 있습니다."));
		return;
	}

	if (false == IsValid(InPlaceableItemData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[APlaceableActorBase::InitializePlaceable] PlaceableItemData가 유효하지 않습니다."));
		return;
	}

	PlaceableItemData = InPlaceableItemData;
	ApplyPlaceableData();
	ForceNetUpdate();
}

void APlaceableItemBase::OnRep_PlaceableItemData()
{
	ApplyPlaceableData();
}

void APlaceableItemBase::ApplyPlaceableData()
{
	if (false == IsValid(MeshComponent))
		return;

	if (false == IsValid(PlaceableItemData))
	{
		MeshComponent->SetStaticMesh(nullptr);
		return;
	}

	UBuildingPartDefinition* BuildingPart = PlaceableItemData->BuildingPart.LoadSynchronous();

	if (false == IsValid(BuildingPart) || false == IsValid(BuildingPart->PartMesh))
	{
		MeshComponent->SetStaticMesh(nullptr);

		UE_LOG(LogTemp, Warning, TEXT("[APlaceableActorBase::ApplyPlaceableData] 배치 메시를 찾지 못했습니다. Item=%s"),
			*GetNameSafe(PlaceableItemData));
		return;
	}

	MeshComponent->SetStaticMesh(BuildingPart->PartMesh);
}
