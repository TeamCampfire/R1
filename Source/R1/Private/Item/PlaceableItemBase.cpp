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
	DOREPLIFETIME(APlaceableItemBase, CurrentDurability);
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
	CurrentDurability = GetMaxDurability();
	ApplyPlaceableData();
	ForceNetUpdate();
}

float APlaceableItemBase::GetMaxDurability()
{
	if(false == IsValid(PlaceableItemData)) return 0.f;

	UBuildingPartDefinition* BuildingPart = PlaceableItemData->BuildingPart.LoadSynchronous();
	if (false == IsValid(BuildingPart)) return 0.f;

	return BuildingPart->MaxDurability;
}

bool APlaceableItemBase::ApplyPlaceableDamage(float DamageAmount)
{
	// BuildingActor::ApplyBuildingDamage()와 동일한 로직
	// Placeable Item은 파괴하면 
	// 건물 내구도는 서버가 단독으로 변경해야 해요 고로 클라가 호출하면 데미지 적용 안 해요
	if (false == HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[APlaceableItemBase::ApplyPlaceableDamage] 서버에서만 피해를 적용할 수 있습니다."));
		return false;

	}

	// 0 또는 음수 피해는 잘못된 요청이므로 데미지 적용 안 해요
	if (DamageAmount <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APlaceableItemBase::ApplyPlaceableDamage] DamageAmount가 올바르지 않습니다. Damage=%.1f"), DamageAmount);
		return false;
	}

	const float  MaxDurability = GetMaxDurability();

	// 최대 내구도가 없는 건물은 정상적인 내구도 데이터가 구성되지 않은 상태
	if (MaxDurability <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APlaceableItemBase::ApplyPlaceableDamage] 건물의 최대 내구도가 설정되지 않았습니다."));
		return false;
	}

	const float PreviousDurability = CurrentDurability;
	CurrentDurability = FMath::Max(0.f, CurrentDurability - DamageAmount); // 내구도 음수 방지
	ForceNetUpdate(); // 변경된 내구도가 클라이언트에 전달될 수 있도록 복제 갱신

	UE_LOG(LogTemp, Log, TEXT("[APlaceableItemBase::ApplyPlaceableDamage] Damage=%.1f, Durability=%.1f -> %.1f / %.1f"),
		DamageAmount, PreviousDurability, CurrentDurability, MaxDurability);

	// 내구도가 남아 있다면 피해 처리만 마치고 건물은 유지
	if (CurrentDurability > 0.f) return true;
	UE_LOG(LogTemp, Log, TEXT("[APlaceableItemBase::ApplyPlaceableDamage] 내구도가 0이 되어 건물 전체를 파괴합니다."));

	// 내구도가 없으면 걍 부숴버렷
	Destroy();
	return true;
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
