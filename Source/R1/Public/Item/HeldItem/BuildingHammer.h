

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
	virtual void OnSecondaryActionStarted() override;

protected:
	UFUNCTION(Server, Reliable)
	void Server_ApplyBuildingDamage(ABuildingActor* TargetBuilding, float Damage);

	// 서버에서 확정된 공격 이후 내구도를 UI에 전달하는 함수
	UFUNCTION(Client, Reliable) 
	void Client_ShowBuildingDurability(float CurrentDurability, float MaxDurability);
};
