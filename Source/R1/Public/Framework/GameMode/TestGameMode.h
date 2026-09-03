/// 최초작성 : 2026.08.31
/// 작 성 자 : 강 진 구
/// 스폰 지점 설정 테스트용 게임모드

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TestGameMode.generated.h"


class AActionCharacter;
/**
 * 
 */
UCLASS()
class R1_API ATestGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;

	// 플레이어 사망 처리 함수
	void HandlePlayerDeath(AActionCharacter* InDeadChar);

protected:
	// 플레이어 리스폰 함수
	UFUNCTION(BlueprintCallable)
	void RespawnPlayer(AController* InController);

	// 스폰 가능한 지점 찾는 함수
	bool FindRandomSpawnLocation(FVector& OutLocation) const;

	// Debug--------------------------------------------------------------
	UFUNCTION(Exec)
	void Respawn(int32 ControllerIndex);
	// -------------------------------------------------------------------
protected:
	// 랜덤 스폰 범위
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float SpawnRadius = 5000.0f;
};
