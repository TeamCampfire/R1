


#include "Item/HarvestMisc/Hemp.h"
#include "Component/InventoryComponent.h"
#include "Character/ActionCharacter.h"

// Sets default values
AHemp::AHemp()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	// 카메라 라인트레이스(ECC_Visibility)에 감지되도록 설정
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

FText AHemp::GetInteractionDisplayName_Implementation() const
{
	return DisplayName;
}

bool AHemp::CanInteract_Implementation(APawn* Interactor) const
{
	return YieldItemData != nullptr; 
}

void AHemp::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || bHarvested || !Interactor || !YieldItemData) return;
	// 여기까지 들어왔으면 채집완료 표시
	bHarvested = true;

	if (AActionCharacter* Character = Cast<AActionCharacter>(Interactor))
	{
		if (UInventoryComponent* Inven = Character->GetInventoryComponent())
		{
			// 아이템 추가
			int32 Remainder = 0;
			Inven->AddItem(YieldItemData, YieldCount, Remainder);
			Inven->NotifyItemAcquired(YieldItemData, YieldCount - Remainder);
		}
	}

	Destroy();
}

TSoftObjectPtr<UTexture2D> AHemp::GetInteractionIcon_Implementation() const
{
	return InteractionIcon; 
}

// Called when the game starts or when spawned
void AHemp::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHemp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

