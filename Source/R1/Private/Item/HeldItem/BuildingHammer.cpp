


#include "Item/HeldItem/BuildingHammer.h"
#include "Data/Item/HeldItemData.h"
#include "Character/ActionCharacter.h"
#include "BuildingSystem/BuildingActor.h"

void ABuildingHammer::OnSecondaryActionStarted()
{
	Super::OnSecondaryActionStarted();

	// 카메라 중앙에서 라인트레이스를 해서 범위 내에 세워진 건물이 있는지 확인
	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		if (APlayerCameraManager* CameraManger = PC->PlayerCameraManager)
		{
			FHitResult OutHitRes;

			// 카메라의 위치에서 사정거리만큼 line trace
			FVector StartPos = CameraManger->GetCameraLocation();
			FVector EndPos = StartPos + CameraManger->GetCameraRotation().Vector() * ItemData->EffectiveRange;

			// 나는 제외
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			if (GetWorld()->LineTraceSingleByChannel(OutHitRes, StartPos, EndPos, ECC_Visibility, Params))
			{
				if (ABuildingActor* BuildingActor = Cast<ABuildingActor>(OutHitRes.GetActor()))
				{
					// TODO 하드코딩 수정
					// 건물에 데미지를 준다.
					Server_ApplyBuildingDamage(BuildingActor, 100);
					
				}
			}
		}
	}
}

void ABuildingHammer::Server_ApplyBuildingDamage_Implementation(ABuildingActor* TargetBuilding, float Damage)
{
	if (!TargetBuilding) return;
	TargetBuilding->ApplyBuildingDamage(100);
}
