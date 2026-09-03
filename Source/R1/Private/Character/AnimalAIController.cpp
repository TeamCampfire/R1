/// 최초작성 : 2026.09.03
#include "Character/AnimalAIController.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

AAnimalAIController::AAnimalAIController()
{
	bWantsPlayerState = false;
	PrimaryActorTick.bCanEverTick = false;
}

void AAnimalAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 빙의 후 잠시 대기했다가 첫 패트롤 시작
	GetWorld()->GetTimerManager().SetTimer(WaitTimerHandle, this, &AAnimalAIController::MoveToRandomPatrolLocation, 1.5f, false);
}

void AAnimalAIController::MoveToRandomPatrolLocation()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		// 네브메시가 없으면 3초 후 재시도
		GetWorld()->GetTimerManager().SetTimer(WaitTimerHandle, this, &AAnimalAIController::MoveToRandomPatrolLocation, 3.0f, false);
		return;
	}

	FNavLocation RandomLocation;
	if (NavSys->GetRandomReachablePointInRadius(ControlledPawn->GetActorLocation(), PatrolRadius, RandomLocation))
	{
		MoveToLocation(RandomLocation.Location, 50.0f, false, true, true, false);
	}
	else
	{
		// 랜덤 지점 찾기 실패 시 잠시 후 재시도
		GetWorld()->GetTimerManager().SetTimer(WaitTimerHandle, this, &AAnimalAIController::MoveToRandomPatrolLocation, 2.0f, false);
	}
}

void AAnimalAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	// 목적지에 도착했거나 경로가 막혔을 때 일정 시간 풀 뜯기/휴식 후 다음 위치로 이동
	float WaitDuration = FMath::FRandRange(MinWaitTime, MaxWaitTime);
	GetWorld()->GetTimerManager().SetTimer(WaitTimerHandle, this, &AAnimalAIController::OnWaitFinished, WaitDuration, false);
}

void AAnimalAIController::OnWaitFinished()
{
	MoveToRandomPatrolLocation();
}
