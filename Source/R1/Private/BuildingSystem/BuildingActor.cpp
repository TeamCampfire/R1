#include "BuildingSystem/BuildingActor.h"
#include "Data/Building/BuildingPartDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Data/Item/ItemDataBase.h"
#include "Item/ItemPickup.h"

ABuildingActor::ABuildingActor()
{
 	PrimaryActorTick.bCanEverTick = false;

	// 서버에서 생성한 건물을 클라이언트에도 생성해야 하니까.. 복제 true
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

void ABuildingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABuildingActor, PlacedParts); // PlacedParts는 변경될 때 마다 계속 클라에 복제할게요

	// 건물 전체 내구도도 복제..
	DOREPLIFETIME(ABuildingActor, CurrentDurability);
	DOREPLIFETIME(ABuildingActor, MaxDurability);
}

UStaticMeshComponent* ABuildingActor::AddPart(UBuildingPartDefinition* Definition, const FTransform& InRelativeTransform)
{
	// 파츠 생성과 PartID 발급은 서버에서만 수행
	if (false == HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::AddPart] : 서버에서만 파츠를 추가할 수 있습니다."));
		return nullptr;
	}

	if (false == IsValid(Definition) || false == IsValid(Definition->PartMesh))
	{
		UE_LOG(LogTemp, Log, TEXT(" [ABuildingActor::AddPart] : Is Not Valid"));
		return nullptr;
	}

	//! 파트 메시 컴포넌트 생성 및 등록
	UStaticMeshComponent* NewPart = NewObject<UStaticMeshComponent>(this);

	if (false == IsValid(NewPart))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::AddPart] : 메시 컴포넌트 생성에 실패했습니다."));
		return nullptr;
	}
	

	AddInstanceComponent(NewPart);
	NewPart->SetStaticMesh(Definition->PartMesh);
	NewPart->SetupAttachment(RootComponent);
	NewPart->SetRelativeTransform(InRelativeTransform);
	NewPart->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NewPart->SetIsReplicated(true); // 서버에서 생성한 동적 컴포넌트를 클라이언트에도 생성
	NewPart->RegisterComponent(); // 액터 소유 인스턴스 컴포넌트로 등록

	if (false == NewPart->IsRegistered())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::AddPart] : 메시 컴포넌트 등록에 실패했습니다."));

		NewPart->DestroyComponent();
		return nullptr;
	}

	// 설치 완료 파츠 정보 기록하기
	FPlacedBuildingPart PlacedPart;
	PlacedPart.PartID = FGuid::NewGuid();
	PlacedPart.Definition = Definition;
	PlacedPart.RelativeTransform = NewPart->GetRelativeTransform();
	//PlacedPart.CurDurability = Definition->MaxDurability;
	PlacedPart.MeshComponent = NewPart;

	PlacedParts.Add(MoveTemp(PlacedPart));

	const float AddedDurability = FMath::Max(0.f, static_cast<float>(Definition->MaxDurability)); // 이상한 쓰레기 음수값 제외

	// 현재값과 최대값을 함께 증가
	CurrentDurability += AddedDurability;
	MaxDurability += AddedDurability;

	UE_LOG(LogTemp, Log, TEXT("[ABuildingActor::AddPart] 내구도 추가 완료. Added=%.1f, Current=%.1f, Max=%.1f"),
		AddedDurability, CurrentDurability, MaxDurability);

	UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::AddPart] Authority: %s / PlacedParts Num: %d"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"), PlacedParts.Num());

	return NewPart;
}

const FPlacedBuildingPart* ABuildingActor::FindPlacedPartByComponent(const UPrimitiveComponent* InComponent) const
{
	if (false == IsValid(InComponent)) return nullptr;

	// 들고 있는 파츠들을 순회하면서 같은 파츠가 있다면 반환
	for (const FPlacedBuildingPart& PlacedPart : PlacedParts)
	{
		if (PlacedPart.MeshComponent.Get() == InComponent)
			return &PlacedPart;
	}

	return nullptr;
}

FPlacedBuildingPart* ABuildingActor::FindPlacedPartByComponent(UPrimitiveComponent* InComponent)
{
	if (false == IsValid(InComponent)) return nullptr;

	for (FPlacedBuildingPart& PlacedPart : PlacedParts)
	{
		if (PlacedPart.MeshComponent.Get() == InComponent)
			return &PlacedPart;
	}

	return nullptr;
}

bool ABuildingActor::TryGetSnapPointWorldTransform(const UPrimitiveComponent* TargetComponent, const UBuildingPartDefinition* IncomingDefinition, const FName& InSocketName, FTransform& OutWorldTransform) const
{
	// Wall 설치할 때 라인트레이스가 걸려서 이 함수로 들어온 상태

	OutWorldTransform = FTransform::Identity;

	// 맞힌 Foundation이 있는지 ?, 설치하려는 데이터가 있는지?, SocketName이 올바른지?
	if (false == IsValid(TargetComponent) || false == IsValid(IncomingDefinition) || InSocketName.IsNone()) return false;

	// 라인트레이스 맞힌 메시 컴포넌트에 해당하는 건축 파츠가 있는지 확인해요
	// 어디에 세울 지에 대한 그 건축 파츠.. 
	const FPlacedBuildingPart* TargetPart = FindPlacedPartByComponent(TargetComponent);

	// 세울려고 했던 곳에 있던 건축 파츠가 온전한지 확인해요
	if (nullptr == TargetPart || false == IsValid(TargetPart->Definition) || false == IsValid(TargetPart->MeshComponent)) return false;

	// 인자로 들어온 소켓이 세울려고 했던 곳에 있던 건축 파츠의 Definition에 등록된 소켓인지 확인해요
	const FBuildingSnapPointDefinition* SnapPoint = nullptr;
	for (const FBuildingSnapPointDefinition& Candidate : TargetPart->Definition->SnapPoints)
	{
		if (Candidate.SocketName == InSocketName)
		{
			SnapPoint = &Candidate;
			break;
		}
	}

	if (nullptr == SnapPoint) return false;

	// 해당 소켓이 설치하려는 파츠 종류를 허용하는지 확인해요
	if (false == SnapPoint->AllowedPartTypes.Contains(IncomingDefinition->PartType)) return false;

	// Definition에 이름만 등록하고 실제 메시에는 소켓을 만들지 않은 설정 실수를 방지!@!!
	if (false == TargetPart->MeshComponent->DoesSocketExist(InSocketName))
	{
		UE_LOG(LogTemp, Log, TEXT("[ABuildingActor::TryGetSnapPointWorldTransform] : 메시에 소켓을 만들어두지 않은 것 같습니다."));
		return false;
	}

	// Static Mesh에 든 Socket의 로컬 Transform을 (현재 메시 컴포넌트 기준) 월드 Transform으로 변환해요
	OutWorldTransform = TargetPart->MeshComponent->GetSocketTransform(InSocketName, RTS_World);

	// true 반환 조건
	// 맞힌 컴포넌트가 실제 설치 파츠임
	// -> 대상 Definition에 Wall_PosX가 등록됨
	// -> Wall_PosX가 Wall 타입을 허용함
	// -> 실제 Foundation 메시에도 Wall_PosX 소켓이 존재함
	// -> 소켓의 월드 Transform 획득 성공
	return true;
}

FPlacedBuildingPart* ABuildingActor::FindPlacedPartByID(const FGuid& InPartID)
{
	if (false == InPartID.IsValid()) return nullptr;

	for (FPlacedBuildingPart& _placedPart : PlacedParts)
	{
		if (InPartID == _placedPart.PartID)
			return &_placedPart; // 찾았어요
	}

	return nullptr;
}

void ABuildingActor::ResolveAdjacentFoundationConnections(FGuid NewPartID, float AnchorTolerance)
{
	FPlacedBuildingPart* NewPlacedPart = FindPlacedPartByID(NewPartID);

	if (nullptr == NewPlacedPart || false == IsValid(NewPlacedPart->Definition) || false == IsValid(NewPlacedPart->MeshComponent)) return;

	if (NewPlacedPart->Definition->PlacementType != EBuildingPlacementType::FOUNDATION) return;

	const float AnchorToleranceSquared = FMath::Square(AnchorTolerance);

	for (const FBuildingSnapPointDefinition& NewSnapPoint :
		NewPlacedPart->Definition->SnapPoints)
	{
		const bool bNewSocketOccupied = NewPlacedPart->OccupiedSnapPoints.Contains(NewSnapPoint.SocketName);

		const bool bHasAnchorName = false == NewSnapPoint.ConnectionAnchorSocketName.IsNone();

		const bool bAnchorExists = bHasAnchorName && NewPlacedPart->MeshComponent->DoesSocketExist(NewSnapPoint.ConnectionAnchorSocketName);

		if (true == NewSnapPoint.ConnectionAnchorSocketName.IsNone())
			continue;

		// 직접 연결에 사용되어 이미 점유된 면은 다시 검사하지 않아요
		if (NewPlacedPart->OccupiedSnapPoints.Contains(NewSnapPoint.SocketName))
			continue;

		if (false == NewPlacedPart->MeshComponent->DoesSocketExist(NewSnapPoint.ConnectionAnchorSocketName))
			continue;

		const FVector NewAnchorLocation = NewPlacedPart->MeshComponent->GetSocketLocation(NewSnapPoint.ConnectionAnchorSocketName);

		bool bFoundConnection = false;

		for (FPlacedBuildingPart& ExistingPart : PlacedParts)
		{
			if (ExistingPart.PartID == NewPartID)
				continue;

			if (false == IsValid(ExistingPart.Definition) || false == IsValid(ExistingPart.MeshComponent))
				continue;

			if (ExistingPart.Definition->PlacementType != EBuildingPlacementType::FOUNDATION)
				continue;

			for (const FBuildingSnapPointDefinition& ExistingSnapPoint : ExistingPart.Definition->SnapPoints)
			{
				if (true == ExistingSnapPoint.ConnectionAnchorSocketName.IsNone())
					continue;

				if (ExistingPart.OccupiedSnapPoints.Contains(ExistingSnapPoint.SocketName))
					continue;

				// 양쪽 스냅 포인트가 서로의 Foundation 타입을
				// 허용하는지 확인해요
				const bool bNewAllowsExisting = NewSnapPoint.AllowedPartTypes.Contains(ExistingPart.Definition->PartType);

				const bool bExistingAllowsNew = ExistingSnapPoint.AllowedPartTypes.Contains(NewPlacedPart->Definition->PartType);

				if (false == bNewAllowsExisting || false == bExistingAllowsNew)
					continue;

				if (false == ExistingPart.MeshComponent->DoesSocketExist(ExistingSnapPoint.ConnectionAnchorSocketName))
					continue;

				const FVector ExistingAnchorLocation = ExistingPart.MeshComponent->GetSocketLocation(ExistingSnapPoint.ConnectionAnchorSocketName);

				const float DistanceSquared = FVector::DistSquared(NewAnchorLocation, ExistingAnchorLocation);

				const float AnchorDistance = FMath::Sqrt(DistanceSquared);

				if (DistanceSquared > AnchorToleranceSquared)
					continue;

				// 같은 위치에서 맞닿은 두 연결면을 모두 점유
				NewPlacedPart->OccupiedSnapPoints.AddUnique(NewSnapPoint.SocketName);

				ExistingPart.OccupiedSnapPoints.AddUnique(ExistingSnapPoint.SocketName);

				UE_LOG(LogTemp, Warning,
					TEXT("[Foundation Auto Connection] ") TEXT("NewPart=%s | NewSocket=%s | ")
					TEXT("ExistingPart=%s | ExistingSocket=%s | Distance=%.3f"),
					*NewPlacedPart->PartID.ToString(), *NewSnapPoint.SocketName.ToString(), *ExistingPart.PartID.ToString(),
					*ExistingSnapPoint.SocketName.ToString(), FMath::Sqrt(DistanceSquared));

				bFoundConnection = true;
				break;
			}

			if (bFoundConnection)
				break;
		}
	}
}

bool ABuildingActor::DemolishAndDropResources()
{
	// 실제 월드 아이템 생성과 건물 제거는 반드시 서버에서만 수행해야 해요
	if (false == HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::DemolishAndDropResources()] : 서버에서만 건물을 해체할 수 있습니다."));
		return false;
	}

	TMap<UItemDataBase*, int32> Refunds; // 다시 반환시킬 자원들 데이터
	if (false == BuildDemolitionRefunds(Refunds))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::DemolishAndDropResources()] : 환급 자원 계산에 실패했습니다."));
		return false;
	}

	// 중간에 픽업 생성이 실패했을 때.. 이미 생성한 자원만 남는 문제를 막기 위해 기억해둠
	TArray<AItemPickup*> SpawnedPickups;

	int32 RefundIndex = 0;
	const int32 RefundTypeCount = Refunds.Num();

	for (const TPair<UItemDataBase*, int32>& Refund : Refunds)
	{
		if (false == IsValid(Refund.Key) || 0 >= Refund.Value) continue;


		// 여러 종류의 자원 픽업이 완전히 같은 자리에 겹치지 않도록 건물 중심 주변에 원형으로 조금씩 나누어 생성
		const float DropAngle = RefundTypeCount > 0 ? (2.0f * PI * RefundIndex) / static_cast<float>(RefundTypeCount): 0.0f;

		const FVector DropOffset(FMath::Cos(DropAngle) * 80.0f, FMath::Sin(DropAngle) * 80.0f, 100.0f);
		const FVector DropLocation = GetActorLocation() + DropOffset;
		const FTransform DropTransform(FRotator::ZeroRotator, DropLocation);

		// 픽얻액터로 다시 자원 생성해요
		AItemPickup* Pickup = GetWorld()->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), DropTransform);

		if (false == IsValid(Pickup))
		{
			// 해체 자체는 실패시킴, 이번 호출에서 이미 만든 픽업도 지워 재시도 시 자원이 중복 환급되지 않게
			for (AItemPickup* SpawnedPickup : SpawnedPickups)
			{
				if (true == IsValid(SpawnedPickup))
					SpawnedPickup->Destroy();
			}

			UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::DemolishAndDropResources()] : 환급 아이템 스폰에 실패했습니다."));
			return false;
		}

		// 기존 ItemPickup에 자원 종류와 합산된 수량을 전달
		Pickup->InitializeFromItem(Refund.Key, Refund.Value);
		SpawnedPickups.Add(Pickup);
		++RefundIndex;
	}

	// 환급 픽업 생성이 끝난 뒤에만! 건물 전체 제거를 요청
	if (false == Destroy())
	{
		// 건물 제거가 실패했다면 환급 픽업도 되돌려서
		// 건물과 자원이 동시에 남는 중복 환급을 방지
		for (AItemPickup* SpawnedPickup : SpawnedPickups)
		{
			if (true == IsValid(SpawnedPickup))
				SpawnedPickup->Destroy();
		}

		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::DemolishAndDropResources()] : 건물 제거 요청에 실패했습니다."));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[Building Demolition] 건물 전체 해체 완료. PartCount=%d, RefundTypeCount=%d"), PlacedParts.Num(), Refunds.Num());
	return true;
}

bool ABuildingActor::ApplyBuildingDamage(float DamageAmount)
{
	// 건물 내구도는 서버가 단독으로 변경해야 해요 고로 클라가 호출하면 데미지 적용 안 해요
	if (false == HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::ApplyBuildingDamage] 서버에서만 피해를 적용할 수 있습니다."));
		return false;
	}

	// 0 또는 음수 피해는 잘못된 요청이므로 데미지 적용 안 해요
	if (DamageAmount <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::ApplyBuildingDamage] DamageAmount가 올바르지 않습니다. Damage=%.1f"), DamageAmount);
		return false;
	}

	// 최대 내구도가 없는 건물은 정상적인 내구도 데이터가 구성되지 않은 상태
	if (MaxDurability <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABuildingActor::ApplyBuildingDamage] 건물의 최대 내구도가 설정되지 않았습니다."));
		return false;
	}

	const float PreviousDurability = CurrentDurability;
	CurrentDurability = FMath::Max(0.f, CurrentDurability - DamageAmount); // 내구도 음수 방지
	ForceNetUpdate(); // 변경된 내구도가 클라이언트에 전달될 수 있도록 복제 갱신

	UE_LOG(LogTemp, Log, TEXT("[ABuildingActor::ApplyBuildingDamage] Damage=%.1f, Durability=%.1f -> %.1f / %.1f"),
		DamageAmount, PreviousDurability, CurrentDurability, MaxDurability);

	// 내구도가 남아 있다면 피해 처리만 마치고 건물은 유지
	if (CurrentDurability > 0.f) return true;
	UE_LOG(LogTemp, Log, TEXT("[ABuildingActor::ApplyBuildingDamage] 내구도가 0이 되어 건물 전체를 파괴합니다."));

	// 기존에 구현한 자원 드롭 및 건물 전체 제거 함수로 연결!
	return DemolishAndDropResources();
}

bool ABuildingActor::BuildDemolitionRefunds(TMap<class UItemDataBase*, int32>& OutRefunds) const
{
	OutRefunds.Reset();

	// 구성 파츠가 하나도 없다면 환급할 대상도 없으므로 잘못된 건물 상태로 판단
	if (PlacedParts.Num() == 0) return false;

	for (const FPlacedBuildingPart& PlacedPart : PlacedParts)
	{
		// 어떤 파츠의 원본 데이터가 유효하지 않으면 자원이 누락된 채 건물이 삭제되는 일을 막ㅏ요
		if (false == IsValid(PlacedPart.Definition)) return false;

		for (const FBuildingResourceCost& ResourceCost : PlacedPart.Definition->ResourceCosts)
		{
			// 비용 데이터가 비어 있거나 잘못된 파츠는 해체 자체를 막아버려요
			if (false == IsValid(ResourceCost.ItemData) || 0 >= ResourceCost.RequiredCount) return false;

			// 같은 자원이 여러 파츠의 비용에 포함되어 있어도 하나의 픽업 수량으로 합산시켜요
			OutRefunds.FindOrAdd(ResourceCost.ItemData.Get()) += ResourceCost.RequiredCount;
		}
	}
	return true;
}
