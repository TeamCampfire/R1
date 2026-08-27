// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemPickup.h"
#include "Data/Item/ItemDataBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

// Sets default values
AItemPickup::AItemPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	
	// 드랍 시 자연스럽게 바닥/다른 오브젝트 위에 안착하도록 물리 시뮬레이션은 켜두되,
	// 플레이어(폰)와는 절대 물리적으로 충돌(밀림/막힘)하지 않게 Pawn 채널만 Ignore로
	// 뺀다. BlockAllDynamic 프로파일을 그대로 쓰면 Pawn도 Block이라 캐릭터가 부딪혀
	// 밀려나는 부자연스러운 충돌이 생긴다 — 플레이어와의 상호작용 판정은 어차피
	// InteractionSphere(Overlap)가 전담하므로 Mesh가 Pawn을 막을 이유가 없다.
	/// 추후 콜리전 채널 세분화 되면 수정 필요
	Mesh->SetCollisionObjectType(ECC_PhysicsBody);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);	
	Mesh->SetSimulatePhysics(true);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(120.f);
	
	// 프로젝트에 별도 상호작용 콜리전 채널이 있다면 그쪽 프로파일로 교체 권장.
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AItemPickup::InitializeFromItem(UItemDataBase* InItemData, int32 InCount)
{
	ItemData = InItemData;
	Count = FMath::Max(1, InCount);
	RefreshVisual();
}

// Called when the game starts or when spawned
void AItemPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

void AItemPickup::RefreshVisual()
{
	if (!Mesh)
	{
		return;
	}

	if (!ItemData)
	{
		Mesh->SetStaticMesh(nullptr);
		return;
	}

	// 에디터 배치/즉시 프리뷰 목적이라 동기 로드 사용. 한 번에 대량 스폰(예: 채집 시
	// 자원 무더기가 흩뿌려지는 연출 등)이 필요해지면 Asset Manager 비동기 로드로
	// 교체하는 걸 고려.
	if (UStaticMesh* LoadedMesh = ItemData->PickupMesh.LoadSynchronous())
	{
		Mesh->SetStaticMesh(LoadedMesh);
	}
}

// Called every frame
void AItemPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AItemPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisual();
}

