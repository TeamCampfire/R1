


#include "Item/HeldItem/BuildingHammer.h"
#include "Data/Item/HeldItemData.h"
#include "Character/ActionCharacter.h"
#include "BuildingSystem/BuildingActor.h"
#include "Framework/MainHUD.h"
#include "Widget/MainHUDWidget.h"
#include "GameFramework/PlayerController.h"
#include "Item/PlaceableItemBase.h"

void ABuildingHammer::OnSecondaryActionStarted()
{
	// 우클릭 하면 Super에서 몽타주만 시작하고
	// 실제 라인트레이스랑 데미지 적용은
	// 데이터에 바인딩된 해머 액션 몽타주 안에 있는 노티파이 시점에 진행되도록 했어요
	Super::OnSecondaryActionStarted();
}

void ABuildingHammer::PerformBuildingHit()
{
	// 몽타주 노티파이는 다른 클라이언트의 복제 캐릭터에서도 발생할 수 있기 떄문에
	// 실제 공격 요청은 이 해머를 직접 조작하는 로컬 플레이어만 진행

	UE_LOG(LogTemp, Display, TEXT("test0"));

	if (false == IsValid(OwnerCharacter) || false == OwnerCharacter->IsLocallyControlled() || false == IsValid(ItemData))
		return;

	UE_LOG(LogTemp, Display, TEXT("test1"));

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

				UE_LOG(LogTemp, Display, TEXT("test2"));
			if (GetWorld()->LineTraceSingleByChannel(OutHitRes, StartPos, EndPos, ECC_Visibility, Params))
			{
					UE_LOG(LogTemp, Display, TEXT("test3"));
				// 건축물을 공격한 경우
				if (ABuildingActor* BuildingActor = Cast<ABuildingActor>(OutHitRes.GetActor()))
				{
					// 건물에 데미지를 준다.
					Server_ApplyBuildingDamage(BuildingActor, 50.f);
				}

				// Placeable 아이템을 공격한 경우
				if (APlaceableItemBase* PlaceableActor = Cast<APlaceableItemBase>(OutHitRes.GetActor()))
				{
					// 개별 Placeable에 데미지를 준다
					Server_ApplyPlaceableDamage(PlaceableActor, 50.f);
				}
			}

		}
	}
}

void ABuildingHammer::Server_ApplyPlaceableDamage_Implementation(APlaceableItemBase* TargetPlaceable, float Damage)
{
	if (nullptr == TargetPlaceable || Damage <= 0.f) return;

	// 아이템이 이번 공격으로 파괴되더라도 UI에 표시는 할 수 있도록 피해 적용 전에 최대 내구도와 예상 결과를 보관
	float MaxDurability = TargetPlaceable->GetMaxDurability();
	float ResultDurability = FMath::Max(0.f, TargetPlaceable->GetCurrentDurability() - Damage);

	if (false == TargetPlaceable->ApplyPlaceableDamage(Damage)) return;

	// 서버가 확정한 공격 이후 내구도를 공격한 클라이언트에게 전달
	Client_ShowBuildingDurability(ResultDurability, MaxDurability);
}

void ABuildingHammer::Server_ApplyBuildingDamage_Implementation(ABuildingActor* TargetBuilding, float Damage)
{
	if (!TargetBuilding || Damage <= 0.f) return;

	// 건물이 이번 공격으로 파괴되더라도 UI에 표시는 할 수 있도록 피해 적용 전에 최대 내구도와 예상 결과를 보관
	float MaxDurability = TargetBuilding->GetMaxDurability();
	float ResultDurability = FMath::Max(0.f, TargetBuilding->GetCurrentDurability() - Damage);

	if(false == TargetBuilding->ApplyBuildingDamage(Damage)) return;

	// 서버가 확정한 공격 이후 내구도를 공격한 클라이언트에게 전달
	Client_ShowBuildingDurability(ResultDurability, MaxDurability);
}

void ABuildingHammer::Client_ShowBuildingDurability_Implementation(float CurrentDurability, float MaxDurability)
{
	AActionCharacter* OwningCharacter = Cast<AActionCharacter>(GetOwner());
	APlayerController* PlayerController = IsValid(OwningCharacter) ? Cast<APlayerController>(OwningCharacter->GetController()): nullptr;

	if (false == IsValid(PlayerController)) return;

	// 공격한 플레이어 자신의 MainHUD 가져옴
	AMainHUD* MainHUD = Cast<AMainHUD>(PlayerController->GetHUD());
	UMainHUDWidget* MainHUDWidget = IsValid(MainHUD) ? MainHUD->GetMainHudWidget() : nullptr;
	if (false == IsValid(MainHUDWidget)) return;

	// MainHUD 내부 타이머 동작하는 동안만 내구도 위젯을 보여줘요
	MainHUDWidget->ShowBuildingDurability(CurrentDurability, MaxDurability);
}
