

#pragma once

#include "CoreMinimal.h"
#include "Item/HeldItemBase.h"
#include "BuildingHammer.generated.h"

/**
 * 
 */
UCLASS()
class R1_API ABuildingHammer : public AHeldItemBase
{
	GENERATED_BODY()

public:
	// 우클릭 입력 시에는 해머 공격 몽타주만 시작
	virtual void OnSecondaryActionStarted() override;

public:
	// 우클릭 눌렀을 때 실행되는 해머 애님 몽타주에 있는 노티파이로 인해 실행되는 함수
	void PerformBuildingHit();

protected:
	UFUNCTION(Server, Reliable)
	void Server_ApplyBuildingDamage(ABuildingActor* TargetBuilding, float Damage);

	// 서버에서 확정된 공격 이후 내구도를 UI에 전달하는 함수
	UFUNCTION(Client, Reliable) 
	void Client_ShowBuildingDurability(float CurrentDurability, float MaxDurability);
};
