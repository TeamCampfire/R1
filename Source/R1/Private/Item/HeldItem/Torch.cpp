


// Test build
#include "Item/HeldItem/Torch.h"
#include "Character/ActionCharacter.h"
#include "NiagaraComponent.h"
#include "Components/PointLightComponent.h"

ATorch::ATorch()
	:Super()
{
	FlameFxComponent3P = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlameFx3"));
	FlameFxComponent3P->SetupAttachment(ItemMesh3P);
	FlameFxComponent3P->SetOwnerNoSee(true);
	FlameFxComponent3P->bAutoActivate = false;

	FlameFxComponent1P = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlameFx1"));
	FlameFxComponent1P->SetupAttachment(ItemMesh1P);
	FlameFxComponent1P->SetOnlyOwnerSee(true);
	FlameFxComponent1P->bAutoActivate = false;

	TorchFireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light3"));
	TorchFireLight->SetupAttachment(ItemMesh3P);
	TorchFireLight->SetVisibility(false);
	TorchFireLight->SetLightColor(FLinearColor(1.f, 0.6f, 0.2f));
	TorchFireLight->SetIntensity(3500.0f);
	TorchFireLight->SetAttenuationRadius(800.0f);
}

void ATorch::OnPrimaryActionStarted()
{
	if (bIsActive && !LitAttackMontage) return;
	if (!bIsActive && !UnlitAttackMontage) return;

	UAnimMontage* TargetMontage = nullptr;
	if (bIsActive)	TargetMontage = LitAttackMontage;
	if (!bIsActive) TargetMontage = UnlitAttackMontage;
	if (!TargetMontage) return;

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

void ATorch::OnSecondaryActionStarted()
{

	if (bIsActive && !LitMontage) return;
	if (!bIsActive && !UnlitMontage) return;

	UAnimMontage* TargetMontage = nullptr;
	if (!bIsActive)	TargetMontage = LitMontage;
	if (bIsActive) TargetMontage = UnlitMontage;
	if (!TargetMontage) return;


	//ToggleState();

	if (UAnimInstance* AnimInst3P = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
	{
		if (!AnimInst3P->IsAnyMontagePlaying())
		{
			const float duration = OwnerCharacter->PlayAnimMontage(TargetMontage);

			if (duration > 0)
			{
				FOnMontageBlendingOutStarted BlendOutDelegate;
				BlendOutDelegate.BindUObject(this, &ATorch::OnMontageEnded);
				AnimInst3P->Montage_SetBlendingOutDelegate(BlendOutDelegate, TargetMontage);
			}

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

void ATorch::InitItemVisual(UHeldItemData* InItemData)
{
	Super::InitItemVisual(InItemData);

	const FName SocketName = TEXT("Prop_End");

	if (ItemMesh3P)
	{
		if (ItemMesh3P->DoesSocketExist(SocketName))
		{
			if (FlameFxComponent3P) FlameFxComponent3P->AttachToComponent(ItemMesh3P, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		}
		else
		{
			if (FlameFxComponent3P) FlameFxComponent3P->SetRelativeLocation(FVector(0.f, 0.f, 26.f));
		}
	}

	if (ItemMesh1P)
	{
		if (ItemMesh1P->DoesSocketExist(SocketName))
		{
			if (FlameFxComponent1P) FlameFxComponent1P->AttachToComponent(ItemMesh1P, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		}
		else
		{
			if (FlameFxComponent1P) FlameFxComponent1P->SetRelativeLocation(FVector(0.f, 0.f, 26.f));
		}
	}

	if (TorchFireLight)
	{
		if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
		{
			if (!ItemMesh1P) return;
			TorchFireLight->AttachToComponent(ItemMesh1P, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		}
		else if (ItemMesh3P)
		{
			TorchFireLight->AttachToComponent(ItemMesh3P, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		}
	}

}

void ATorch::Server_ToggleState_Implementation()
{
	ToggleState();
}

void ATorch::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted) return;

	ToggleState();

	if (!HasAuthority())
	{
		Server_ToggleState();
	}
}

void ATorch::OnItemStateChanged(bool bNewState)
{
	Super::OnItemStateChanged(bNewState);
	if (bNewState)
	{
		if (FlameFxComponent3P) FlameFxComponent3P->Activate(true);
		if (FlameFxComponent1P) FlameFxComponent1P->Activate(true);
		if (TorchFireLight) TorchFireLight->SetVisibility(true);
	}
	else
	{
		if (FlameFxComponent3P) FlameFxComponent3P->Deactivate();
		if (FlameFxComponent1P) FlameFxComponent1P->Deactivate();
		if (TorchFireLight) TorchFireLight->SetVisibility(false);
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

