


#include "Framework/GameMode/TestGameMode.h"
#include "GameFramework/Controller.h"
#include "Character/ActionCharacter.h"
#include "Character/ActionPlayerController.h"

void ATestGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void ATestGameMode::HandlePlayerDeath(AActionCharacter* InDeadChar)
{
	if (!IsValid(InDeadChar)) return;

	AController* PC = InDeadChar->GetController();

	if (IsValid(PC))
	{
		PC->UnPossess();
	}
}

void ATestGameMode::RespawnPlayer(AController* InController)
{
	if (!IsValid(InController)) return;

	FVector SpawnLocation;
	FRotator SpawnRotation = FRotator::ZeroRotator;
	FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	if (IsValid(Cast<AActionPlayerController>(InController)->GetRespawnPoint()))
	{
		SpawnTransform =
			IRespawnPointInterface::Execute_GetRespawnTransform(
				Cast<AActionPlayerController>(InController)->GetRespawnPoint()
			);
		SpawnLocation = SpawnTransform.GetLocation();
	}
	else if (!FindRandomSpawnLocation(SpawnLocation))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("적절한 스폰 지점을 찾지 못했습니다..."));

		return;
	}


	SpawnTransform = FTransform(SpawnRotation, SpawnLocation);

	TSubclassOf<APawn> PlayerClass =
		GetDefaultPawnClassForController(InController);

	if (!PlayerClass)	return;

	AActionCharacter* NewCharacter =
		GetWorld()->SpawnActor<AActionCharacter>(
			PlayerClass,
			SpawnTransform
		);

	if (NewCharacter)
	{
		AActionPlayerController* PC =
			Cast<AActionPlayerController>(InController);

		if (PC)	PC->PossessChar(NewCharacter);
	}
}

bool ATestGameMode::FindRandomSpawnLocation(FVector& OutLocation) const
{
	UWorld* World = GetWorld();

	if (!World)
		return false;
	// 월드 원점
	const FVector Origin = FVector::ZeroVector;
	// 라인트레이스 길이
	const float TraceHeight = 10000.0f;
	// 리스폰 지점 탐색 최대횟수
	const int32 MaxAttempts = 20;

	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		const float RandomX = FMath::RandRange(-SpawnRadius, SpawnRadius);
		const float RandomY = FMath::RandRange(-SpawnRadius, SpawnRadius);

		const FVector TraceStart(
			Origin.X + RandomX,
			Origin.Y + RandomY,
			TraceHeight
		);

		const FVector TraceEnd(
			Origin.X + RandomX,
			Origin.Y + RandomY,
			-TraceHeight
		);

		FHitResult Hit;

		FCollisionQueryParams Params;
		Params.bTraceComplex = true;

		const bool bHit = World->LineTraceSingleByChannel(
			Hit,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			Params
		);

		if (!bHit)
			continue;

		if (Hit.Component.IsValid() &&
			Hit.Component->GetCollisionProfileName() == TEXT("WaterBodyCollision"))
		{
			continue;
		}

		OutLocation = Hit.Location;
		return true;
	}

	return false;
}
