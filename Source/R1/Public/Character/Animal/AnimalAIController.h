/// 최초작성 : 2026.09.03
/// 작성자 : 주형진
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AnimalAIController.generated.h"

/**
 * 사슴 등 동물 전용 AI 컨트롤러 (네브메시 기반 자립형 랜덤 패트롤)
 */
UCLASS()
class R1_API AAnimalAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAnimalAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	// 다음 패트롤 지점 탐색 및 이동 시작
	void MoveToRandomPatrolLocation();

	// 대기 후 다음 이동 타이머 호출
	void OnWaitFinished();

protected:
	// 배회 반경 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
	float PatrolRadius = 1500.0f;

	// 도착 후 대기 최소/최대 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
	float MinWaitTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
	float MaxWaitTime = 6.0f;

	FTimerHandle WaitTimerHandle;
};
