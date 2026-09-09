/// 최초작성 : 2026.08.30
/// 작 성 자 : 주 형 진

// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/HeldItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Item/HeldItemBase.h"
#include "Data/Item/HeldItemData.h"
#include "Character/ActionCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "UObject/ConstructorHelpers.h"

UHeldItemComponent::UHeldItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHeldItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHeldItemComponent, CurrentHeldItem);
	DOREPLIFETIME(UHeldItemComponent, CurrentEquippedItemData);
}

void UHeldItemComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AActionCharacter>(GetOwner());

	// 서버에서만 초기 아이템 자동 장착 수행
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (DefaultItemData)
		{
			EquipHeldItemByData(DefaultItemData);
		}
		else if (DefaultHeldItemClass)
		{
			EquipHeldItemByClass(DefaultHeldItemClass);
		}
	}
}

void UHeldItemComponent::AttachHeldItemToCharacter(AHeldItemBase* ItemToAttach)
{
	if (!ItemToAttach) return;
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AActionCharacter>(GetOwner());
	}
	if (!OwnerCharacter) return;

	// 1. 도구에 데이터 에셋의 WeaponMesh 적용
	if (CurrentEquippedItemData)
	{
		ItemToAttach->InitItemVisual(CurrentEquippedItemData);
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("[UHeldItemComponent::AttachHeldItemToCharacter] CurrentEquippedItemData is NULL!"));
	}

	// 2. 3인칭 전신(GetMesh()) 소켓에 액터 및 3P 메시 부착
	if (USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh())
	{
		const FName HandSocket = CharacterMesh->DoesSocketExist(FName(TEXT("r_prop"))) 
			? FName(TEXT("r_prop")) 
			: (CharacterMesh->DoesSocketExist(FName(TEXT("r_handSocket"))) 
				? FName(TEXT("r_handSocket")) 
				: (CharacterMesh->DoesSocketExist(FName(TEXT("RightHandSocket"))) ? FName(TEXT("RightHandSocket")) : FName(NAME_None)));

		if (HandSocket != NAME_None)
		{
			ItemToAttach->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);

			// 3P 무기 메시 오프셋 적용
			FTransform Offset3P = ItemToAttach->GetItemMesh3POffset();
			if (Offset3P.GetScale3D().IsNearlyZero())
			{
				Offset3P.SetScale3D(FVector::OneVector);
			}
			ItemToAttach->SetActorRelativeTransform(Offset3P);

			//UE_LOG(LogTemp, Log, TEXT("[UHeldItemComponent::AttachHeldItemToCharacter] Attached 3P to socket: %s"), *HandSocket.ToString());
		}
		else
		{
			ItemToAttach->AttachToActor(OwnerCharacter, FAttachmentTransformRules::KeepRelativeTransform);
			//UE_LOG(LogTemp, Warning, TEXT("[UHeldItemComponent::AttachHeldItemToCharacter] 3P Hand socket NOT found, attached to Actor"));
		}

		// 아이템 변경시에는 모든 몽타주 종료
		if (CharacterMesh->GetAnimInstance())
		{
			CharacterMesh->GetAnimInstance()->StopAllMontages(0);
		}
	}


	// 3. 1인칭 팔(FirstPersonMesh) 소켓에 ItemMesh1P 분리 부착
	if (USkeletalMeshComponent* FPMesh = OwnerCharacter->GetFirstPersonMesh())
	{
		// 1인칭 팔 자체의 위치 및 회전 오프셋 적용
		FVector TargetLocation = ItemToAttach->GetFirstPersonMeshLocation();
		FRotator TargetRotation = ItemToAttach->GetFirstPersonMeshRotation();

		// 돌(Rock)의 경우 데이터 에셋에서 별도 회전 변경이 없었다면 기존 -105도 보정 유지 (하위 호환)
		if (ItemToAttach->GetItemData() && ItemToAttach->GetItemData()->ItemID.ToString() == TEXT("Item_Held_Rock"))
		{
			if (TargetRotation == FRotator(0.f, -90.f, 0.f))
			{
				TargetRotation = FRotator(0.f, -105.f, 0.f);
			}
		}

		FPMesh->SetRelativeLocationAndRotation(TargetLocation, TargetRotation);

		if (UStaticMeshComponent* Mesh1P = ItemToAttach->GetItemMesh1P())
		{
			const FName FPSocket = FPMesh->DoesSocketExist(FName(TEXT("r_prop"))) 
				? FName(TEXT("r_prop")) 
				: (FPMesh->DoesSocketExist(FName(TEXT("hand_rSocket"))) 
					? FName(TEXT("hand_rSocket")) 
					: (FPMesh->DoesSocketExist(FName(TEXT("r_handSocket"))) ? FName(TEXT("r_handSocket")) : FName(NAME_None)));

			if (FPSocket != NAME_None)
			{
				Mesh1P->AttachToComponent(FPMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FPSocket);

				// 1P 무기 메시 소켓 기준 오프셋 적용
				FTransform Offset1P = ItemToAttach->GetItemMesh1POffset();
				if (Offset1P.GetScale3D().IsNearlyZero())
				{
					Offset1P.SetScale3D(FVector::OneVector);
				}
				Mesh1P->SetRelativeTransform(Offset1P);

				//UE_LOG(LogTemp, Log, TEXT("[UHeldItemComponent::AttachHeldItemToCharacter] Attached 1P to socket: %s"), *FPSocket.ToString());
			}
			else
			{
				//UE_LOG(LogTemp, Warning, TEXT("[UHeldItemComponent::AttachHeldItemToCharacter] 1P Hand socket NOT found on FirstPersonMesh!"));
			}
		}
	}
}

void UHeldItemComponent::OnRep_CurrentHeldItem(AHeldItemBase* PreviousHeldItem)
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AActionCharacter>(GetOwner());
	}

	// 1. 이전 아이템 해제 라이프사이클 처리
	if (PreviousHeldItem && IsValid(PreviousHeldItem))
	{
		PreviousHeldItem->OnUnequipped();
	}

	// 2. 무기가 해제되어 빈 손(nullptr)이 된 경우 -> 클라이언트에서도 애니메이션 레이어 즉시 언링크 및 1P 팔 기본 위치 복원!
	if (!CurrentHeldItem)
	{
		UnlinkItemAnimLayers();
		if (OwnerCharacter)
		{
			if (USkeletalMeshComponent* FPMesh = OwnerCharacter->GetFirstPersonMesh())
			{
				FPMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -130.f), FRotator(0.f, -90.f, 0.f));
			}
		}
		return;
	}

	// 3. 새 아이템 장착 및 소켓 부착
	AttachHeldItemToCharacter(CurrentHeldItem);
	CurrentHeldItem->OnEquipped(OwnerCharacter);

	// 애니메이션 레이어 동적 링크 (1P 팔과 3P 몸 모두에 연결)
	if (CurrentEquippedItemData && CurrentEquippedItemData->AnimLayer)
	{
		LinkItemAnimLayers(CurrentEquippedItemData->AnimLayer);
	}

	// 로컬 컨트롤러인 경우 입력 컴포넌트 바인딩 전달
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled() && OwnerCharacter->InputComponent)
	{
		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(OwnerCharacter->InputComponent))
		{
			CurrentHeldItem->SetupInputComponent(EIC);
		}
	}
}

void UHeldItemComponent::OnRep_CurrentEquippedItemData()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AActionCharacter>(GetOwner());
	}

	if (CurrentEquippedItemData)
	{
		if (CurrentHeldItem)
		{
			CurrentHeldItem->InitItemVisual(CurrentEquippedItemData);
		}
		if (CurrentEquippedItemData->AnimLayer)
		{
			LinkItemAnimLayers(CurrentEquippedItemData->AnimLayer);
		}
	}
	else
	{
		// 데이터가 null로 비워졌다면 무장 해제 상태 -> 언링크
		UnlinkItemAnimLayers();
	}
}

AHeldItemBase* UHeldItemComponent::EquipItem(UItemDataBase* ItemData)
{
	if (!ItemData)
	{
		UnequipHeldItem();
		return nullptr;
	}

	UHeldItemData* EquipData = Cast<UHeldItemData>(ItemData);
	if (EquipData)
	{
		return EquipHeldItemByData(EquipData);
	}

	return nullptr;
}

AHeldItemBase* UHeldItemComponent::EquipHeldItemByData(UHeldItemData* EquipItemData)
{
	if (!EquipItemData)
	{
		UnequipHeldItem();
		return nullptr;
	}

	TSubclassOf<AHeldItemBase> ItemClassToSpawn = EquipItemData->HeldItemClass;
	if (!ItemClassToSpawn)
	{
		ItemClassToSpawn = DefaultHeldItemClass ? DefaultHeldItemClass : TSubclassOf<AHeldItemBase>(AHeldItemBase::StaticClass());
	}

	UnequipHeldItem();
	CurrentEquippedItemData = EquipItemData;
	return EquipHeldItemByClass(ItemClassToSpawn);
}

AHeldItemBase* UHeldItemComponent::EquipHeldItemByClass(TSubclassOf<AHeldItemBase> ItemClass)
{
	// 장착/스폰은 반드시 서버에서만 실행
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return nullptr;
	}

	// CurrentEquippedItemData가 이미 세팅되어 있다면 보존하고 기존 액터만 정리
	UHeldItemData* SavedItemData = CurrentEquippedItemData;
	if (CurrentHeldItem)
	{
		CurrentHeldItem->OnUnequipped();
		CurrentHeldItem->Destroy();
		CurrentHeldItem = nullptr;
	}
	CurrentEquippedItemData = SavedItemData;

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AActionCharacter>(GetOwner());
	}

	if (!OwnerCharacter || !ItemClass || !GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentHeldItem = GetWorld()->SpawnActor<AHeldItemBase>(ItemClass, OwnerCharacter->GetActorLocation(), OwnerCharacter->GetActorRotation(), SpawnParams);
	if (CurrentHeldItem)
	{
		CurrentHeldItem->SetOwner(OwnerCharacter);
		AttachHeldItemToCharacter(CurrentHeldItem);
		CurrentHeldItem->OnEquipped(OwnerCharacter);

		// 애니메이션 레이어 동적 링크 (서버/호스트: 1인칭 팔 & 3인칭 몸)
		if (CurrentEquippedItemData && CurrentEquippedItemData->AnimLayer)
		{
			LinkItemAnimLayers(CurrentEquippedItemData->AnimLayer);
		}

		// 호스트(리슨 서버)의 로컬 캐릭터인 경우 입력 바인딩 설정
		if (OwnerCharacter->IsLocallyControlled() && OwnerCharacter->InputComponent)
		{
			if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(OwnerCharacter->InputComponent))
			{
				CurrentHeldItem->SetupInputComponent(EIC);
			}
		}
	}

	return CurrentHeldItem;
}

void UHeldItemComponent::SwapEquippedItemData(UHeldItemData* NewData)
{
	// 장착 데이터 변경은 반드시 서버에서만 — 클라이언트가 호출해도 CurrentEquippedItemData는
	// ReplicatedUsing이라 서버 값만이 진짜다.
	if (!GetOwner() || !GetOwner()->HasAuthority() || !NewData)
	{
		return;
	}

	CurrentEquippedItemData = NewData;

	// OnRep_CurrentEquippedItemData는 "다른 클라이언트"에서만 리플리케이션으로 자동 호출되므로,
	// 이 함수를 실행 중인 서버(리슨 서버 포함) 자신의 비주얼은 여기서 직접 갱신해줘야 한다.
	if (CurrentHeldItem)
	{
		CurrentHeldItem->InitItemVisual(NewData);
	}
}

void UHeldItemComponent::UnequipHeldItem()
{
	// 이전 애니메이션 레이어 해제 (1인칭 팔 & 3인칭 몸)
	UnlinkItemAnimLayers();

	// 맨손이 되었을 때 1인칭 팔 위치/회전 기본값(0, 0, -130 / 0, -90, 0)으로 복구
	if (OwnerCharacter)
	{
		if (USkeletalMeshComponent* FPMesh = OwnerCharacter->GetFirstPersonMesh())
		{
			FPMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -130.f), FRotator(0.f, -90.f, 0.f));
		}
	}

	if (CurrentHeldItem)
	{
		CurrentHeldItem->OnUnequipped();

		// 액터 소멸은 서버에서만 호출 (클라이언트는 Replication으로 소멸됨)
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			CurrentHeldItem->Destroy();
		}
		CurrentHeldItem = nullptr;
	}
	CurrentEquippedItemData = nullptr;
}

void UHeldItemComponent::UsePrimaryAction(bool bStarted)
{
	if (CurrentHeldItem)
	{
		if (bStarted)
		{
			CurrentHeldItem->OnPrimaryActionStarted();
		}
		else
		{
			CurrentHeldItem->OnPrimaryActionCompleted();
		}
	}
}

void UHeldItemComponent::UseSecondaryAction(bool bStarted)
{
	if (CurrentHeldItem)
	{
		if (bStarted)
		{
			CurrentHeldItem->OnSecondaryActionStarted();
		}
		else
		{
			CurrentHeldItem->OnSecondaryActionCompleted();
		}
	}
}

void UHeldItemComponent::CancelAction()
{
	if (CurrentHeldItem)
	{
		CurrentHeldItem->OnCancelAction();
	}
}

void UHeldItemComponent::OnMoveInput(const FVector2D& MoveValue)
{
	if (CurrentHeldItem)
	{
		CurrentHeldItem->OnMoveInput(MoveValue);
	}
}

bool UHeldItemComponent::BlocksCharacterMovement() const
{
	if (CurrentHeldItem)
	{
		return CurrentHeldItem->BlocksCharacterMovement();
	}
	return false;
}

bool UHeldItemComponent::BlocksDefaultAttack() const
{
	if (CurrentHeldItem)
	{
		return CurrentHeldItem->BlocksDefaultAttack();
	}
	return false;
}

void UHeldItemComponent::LinkItemAnimLayers(TSubclassOf<UAnimInstance> LayerClass)
{
	if (!LayerClass) return;

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AActionCharacter>(GetOwner());
	}
	if (!OwnerCharacter) return;

	// 이미 다른 레이어가 링크되어 있다면 먼저 언링크
	if (LinkedAnimLayerClass && LinkedAnimLayerClass != LayerClass)
	{
		UnlinkItemAnimLayers();
	}

	LinkedAnimLayerClass = LayerClass;

	if (USkeletalMeshComponent* TPMesh = OwnerCharacter->GetMesh())
	{
		TPMesh->LinkAnimClassLayers(LayerClass);
		//UE_LOG(LogTemp, Log, TEXT("[UHeldItemComponent::LinkItemAnimLayers] Linked AnimLayer to 3P Mesh: %s"), *LayerClass->GetName());
	}
}

void UHeldItemComponent::UnlinkItemAnimLayers()
{
	if (!LinkedAnimLayerClass) return;

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AActionCharacter>(GetOwner());
	}

	if (OwnerCharacter)
	{
		if (USkeletalMeshComponent* TPMesh = OwnerCharacter->GetMesh())
		{
			TPMesh->UnlinkAnimClassLayers(LinkedAnimLayerClass);
			//UE_LOG(LogTemp, Log, TEXT("[UHeldItemComponent::UnlinkItemAnimLayers] Unlinked AnimLayer from 3P Mesh: %s"), *LinkedAnimLayerClass->GetName());
		}
	}

	LinkedAnimLayerClass = nullptr;
}
