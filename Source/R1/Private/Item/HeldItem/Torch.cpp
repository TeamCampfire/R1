


#include "Item/HeldItem/Torch.h"
#include "Character/ActionCharacter.h"

void ATorch::OnPrimaryActionStarted()
{
	if (bIsActive && !LitAttackMontage) return;
	if (!bIsActive && !UnlitAttackMontage) return;

	UAnimMontage* TargetMontage = nullptr;
	if (bIsActive)	TargetMontage = LitAttackMontage;
	if (!bIsActive) TargetMontage = UnlitAttackMontage;
	if (!TargetMontage) return;

	if (HasAuthority())
	{
		Multicast_PlayMontage(TargetMontage);
	}
	else
	{
		Server_PlayMontage(TargetMontage);
	}
}

void ATorch::OnSecondaryActionStarted()
{

	if (bIsActive && !LitMontage) return;
	if (!bIsActive && !UnlitMontage) return;

	UAnimMontage* TargetMontage = nullptr;
	if (bIsActive)	TargetMontage = LitMontage;
	if (!bIsActive) TargetMontage = UnlitMontage;
	if (!TargetMontage) return;


	ToggleState();
	UE_LOG(LogTemp, Display, TEXT("current : %d"), bIsActive);

	if (UAnimInstance* AnimInst3P = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
	{
		if (!AnimInst3P->IsAnyMontagePlaying())
		{
			OwnerCharacter->PlayAnimMontage(TargetMontage);

			// 2) 멀티플레이어 동기화 (서버 및 다른 클라이언트)
			if (HasAuthority())
			{
				Multicast_PlayMontage(TargetMontage);
			}
			else
			{
				Server_PlayMontage(TargetMontage);
			}
		}
	}
	
}


void ATorch::Server_PlayMontage_Implementation(UAnimMontage* TargetMontage)
{
	Multicast_PlayMontage(TargetMontage);
}


void ATorch::Multicast_PlayMontage_Implementation(UAnimMontage* TargetMontage)
{
	if (!TargetMontage || !OwnerCharacter) return;

	// 로컬 컨트롤러는 이미 선행 재생했으므로 중복 방지
	if (OwnerCharacter->IsLocallyControlled()) return;

	OwnerCharacter->PlayAnimMontage(TargetMontage);
}

