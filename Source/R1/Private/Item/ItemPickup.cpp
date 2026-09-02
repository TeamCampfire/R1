// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemPickup.h"
#include "Data/Item/ItemDataBase.h"
#include "Component/InventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AItemPickup::AItemPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// 리플리케이트 적용
	bReplicates = true;


	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	
	// 드랍 시 자연스럽게 바닥/다른 오브젝트 위에 안착하도록 물리 시뮬레이션은 켜두되,
	// 플레이어(폰)와는 절대 물리적으로 충돌(밀림/막힘)하지 않게 Pawn 채널만 Ignore로
	// 뺀다. BlockAllDynamic 프로파일을 그대로 쓰면 Pawn도 Block이라 캐릭터가 부딪혀
	// 밀려나는 부자연스러운 충돌이 생긴다 — 플레이어와의 상호작용 판정은 어차피
	// InteractionSphere(Overlap)가 전담하므로 Mesh가 Pawn을 막을 이유가 없다.
	// 픽업끼리도 서로 Block이면(둘 다 ObjectType=PhysicsBody라) 무더기로 드랍/스폰됐을 때
	// 서로 밀어내며 튕기므로, PhysicsBody 채널도 Ignore로 빼서 바닥/벽(WorldStatic 등)과는
	// 계속 충돌하되 아이템끼리는 그냥 겹쳐 쌓이게 한다.
	/// 추후 콜리전 채널 세분화 되면 수정 필요
	Mesh->SetCollisionObjectType(ECC_PhysicsBody);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Mesh->SetSimulatePhysics(true);

	// UInteractionComponent가 조준 시 스텐실 값만 바꿔 하이라이트를 켜고 끌 수 있도록
	// CustomDepth 렌더링 자체는 스폰 시점(등록 전)에 미리 켜둔다. 컴포넌트가 등록되기
	// 전에 설정하는 것이라 씬 프록시 재생성 비용이 없다 — 조준하는 그 순간 처음 켜면
	// MarkRenderStateDirty()로 프록시가 다시 만들어지면서 TSR 히스토리가 깨져 한두
	// 프레임 노이즈가 튀는데, 스폰 시점에 미리 켜두면 그 비용을 게임플레이 중이 아니라
	// 아무도 안 보고 있는 스폰 시점에 미리 치르게 된다.
	Mesh->SetRenderCustomDepth(true);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(120.f);

	// 프로젝트에 별도 상호작용 콜리전 채널이 있다면 그쪽 프로파일로 교체 권장.
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// AutoOnOverlap 아이템의 근접 자동 획득용 — 실제 처리는 OnInteractionSphereBeginOverlap 참고.
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AItemPickup::OnInteractionSphereBeginOverlap);
}

void AItemPickup::InitializeFromItem(UItemDataBase* InItemData, int32 InCount)
{
	ItemData = InItemData;
	Count = FMath::Max(1, InCount);
	RefreshVisual();
}

void AItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemPickup, ItemData);
	DOREPLIFETIME(AItemPickup, Count);
}

void AItemPickup::OnRep_ItemData()
{
	RefreshVisual();
}

void AItemPickup::AddThrowImpulse(const FVector& Impulse)
{
	if (Mesh)
	{
		Mesh->AddImpulse(Impulse);
	}
}

// Called when the game starts or when spawned
void AItemPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

void AItemPickup::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 오버랩은 서버/클라이언트 양쪽에서 각자 독립적으로 발생한다 — 클라이언트에서도 처리해버리면
	// 권한 없이 TryGrantToInventory(내부에서 Destroy() 호출)가 실행돼 리플리케이션이 꼬인다.
	// 서버는 어차피 모든 폰을 권한 있게 시뮬레이션하니 이 가드만으로 충분하고, 별도 RPC가 필요 없다.
	if (!HasAuthority())
	{
		return;
	}

	if (!ItemData || ItemData->DefaultPickupMode != EPickupMode::AutoOnOverlap)
	{
		return;
	}

	if (APawn* Interactor = Cast<APawn>(OtherActor))
	{
		TryGrantToInventory(Interactor);
	}
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

TSoftObjectPtr<UTexture2D> AItemPickup::GetInteractionIcon_Implementation() const
{
	return ItemData ? ItemData->Icon : nullptr;
}

void AItemPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisual();
}

