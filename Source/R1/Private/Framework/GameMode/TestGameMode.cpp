


#include "Framework/GameMode/TestGameMode.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

#include "Component/StatComponent.h"
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

	AActionPlayerController* PC =
		Cast<AActionPlayerController>(InController);

	if (!PC) return;

	FVector SpawnLocation;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// Respawn 위치 결정
	if (IsValid(PC->GetRespawnPoint()))
	{
		const FTransform RespawnTransform =
			IRespawnPointInterface::Execute_GetRespawnTransform(
				PC->GetRespawnPoint()
			);

		SpawnLocation = RespawnTransform.GetLocation();
		SpawnRotation = RespawnTransform.Rotator();
	}
	else if (!FindRandomSpawnLocation(SpawnLocation))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("적절한 스폰 지점을 찾지 못했습니다..."));

		return;
	}

	// 현재 Controller에 연결된 캐릭터 확인
	AActionCharacter* ExistingCharacter =
		Cast<AActionCharacter>(InController->GetPawn());

	if (IsValid(ExistingCharacter))
	{
		if (UStatComponent* StatComp = ExistingCharacter->GetStatComponent())
		{
			IHealthInterface* HealthInterface = Cast<IHealthInterface>(StatComp);
			UE_LOG(LogTemp, Warning,
				TEXT("[SERVER] Respawn %s bAlive = %s"),
				*InController->GetName(),
				ExistingCharacter &&
				ExistingCharacter->GetStatComponent() &&
				Cast<IHealthInterface>(
					ExistingCharacter->GetStatComponent()
				)->IsAlive()
				? TEXT("TRUE")
				: TEXT("FALSE"));

			if (HealthInterface && HealthInterface->IsAlive())
			{
				ExistingCharacter->SetActorLocationAndRotation(
					SpawnLocation,
					SpawnRotation
				);

				return;
			}
		}
	}
	
	// 살아있는 캐릭터가 없다면 새 캐릭터 생성
	FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	TSubclassOf<APawn> PlayerClass =
		GetDefaultPawnClassForController(InController);

	if (!PlayerClass) return;

	AActionCharacter* NewCharacter =
		GetWorld()->SpawnActor<AActionCharacter>(
			PlayerClass,
			SpawnTransform
		);

	if (NewCharacter)
	{
		PC->PossessChar(NewCharacter);
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

		if (!bHit) continue;

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

void ATestGameMode::Respawn(int32 ControllerIndex)
{
	UWorld* World = GetWorld();

	if (!World) return;

	int32 CurrentIndex = 0;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator();
		It; ++It)
	{
		APlayerController* PC = It->Get();

		if (!IsValid(PC)) continue;
		if (CurrentIndex == ControllerIndex)
		{
			RespawnPlayer(PC);

			UE_LOG(LogTemp, Log, TEXT("Controller %d Respawn"), ControllerIndex);

			return;
		}

		++CurrentIndex;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Controller Index %d를 찾을 수 없습니다."),
		ControllerIndex);
}
