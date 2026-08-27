// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemPickup.h"
#include "Data/Item/ItemDataBase.h"
#include "Component/InventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"

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

void AItemPickup::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
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

void AItemPickup::TryGrantToInventory(APawn* Interactor)
{
	if (!Interactor || !ItemData)
	{
		return;
	}

	UInventoryComponent* Inventory = Interactor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("아이템 획득 : %s"), *(IInteractableInterface::Execute_GetInteractionDisplayName(this).ToString()));
	
	int32 Remainder = 0;
	Inventory->AddItem(ItemData, Count, Remainder);
	
	if (Remainder <= 0)
	{
		Destroy();
	}
	else if (Remainder < Count)
	{
		Count = Remainder;
		RefreshVisual(); // 메시는 그대로지만, 나중에 수량 표시 UI가 붙으면 갱신 지점이 된다.
	}
	// Remainder == Count면 하나도 못 들어간 것 — 인벤토리가 꽉 찬 경우 등, 액터는 그대로 남는다.
}

// Called every frame
void AItemPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FText AItemPickup::GetInteractionDisplayName_Implementation() const
{
	return ItemData ? ItemData->DisplayName : FText::GetEmpty();
}

bool AItemPickup::CanInteract_Implementation(APawn* Interactor) const
{
	// LookAndPress 아이템만 이 경로로 상호작용한다. AutoOnOverlap은 오버랩에서
	// 이미 처리되므로 조준+단축키로 또 시도해도 ItemData가 남아있으면 그냥
	// 같은 로직(TryGrantToInventory)을 한 번 더 태우는 것뿐이라 안전하다.
	return ItemData != nullptr;
}

void AItemPickup::Interact_Implementation(APawn* Interactor)
{
	TryGrantToInventory(Interactor);
}

void AItemPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisual();
}

