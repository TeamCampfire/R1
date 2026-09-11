// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ActionCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Component/HarvestableComponent.h"
#include "Interface/Harvestable.h"
#include "Component/HeldItemComponent.h"
#include "Data/Item/EquipmentItemData.h"

#include "Interface/StatusEffectInterface.h"
#include "Interface/Vehicle/VehicleInterface.h"
#include "Component/StatComponent.h"	
#include "Component/InteractionComponent.h"
#include "Component/InventoryComponent.h"
#include "Component/WarehouseInventoryComponent.h"
#include "Component/CraftingComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "Character/ActionPlayerController.h"
#include "Data/Item/ItemDataBase.h"
#include "Data/Item/PlaceableItemData.h"
#include "Data/Item/HeldItemData.h"
#include "Vehicle/WheeledVehicleBase.h"
#include "Item/SleepingBag.h"

#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"

#include "Net/UnrealNetwork.h"

// Sets default values
AActionCharacter::AActionCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Replicate 설정
	bReplicates = true;
	SetReplicateMovement(true);

	// Head메시 생성
	TorsoMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMesh"));
	TorsoMesh->SetupAttachment(GetMesh());
	TorsoMesh->SetLeaderPoseComponent(GetMesh());

	// Leg메시 생성
	LegMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegMesh"));
	LegMesh->SetupAttachment(GetMesh());
	LegMesh->SetLeaderPoseComponent(GetMesh());

	// HandMesh 생성
	HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	HandMesh->SetupAttachment(GetMesh());
	HandMesh->SetLeaderPoseComponent(GetMesh());

	// FeetMesh 생성
	FeetMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMesh"));
	FeetMesh->SetupAttachment(GetMesh());
	FeetMesh->SetLeaderPoseComponent(GetMesh());

	// 각 파트를 메인 메시의 본에 따라서 움직이도록 부착
	TorsoMesh->SetLeaderPoseComponent(GetMesh());
	LegMesh->SetLeaderPoseComponent(GetMesh());
	HandMesh->SetLeaderPoseComponent(GetMesh());
	FeetMesh->SetLeaderPoseComponent(GetMesh());

	/// 카메라 생성 및 세팅
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight)); // 캡슐 기준 눈높이
	FirstPersonCamera->bUsePawnControlRotation = true;

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(FirstPersonCamera);
	FirstPersonMesh->SetRelativeLocationAndRotation(FVector(0, 0, -130), FRotator(0, -90, 0));

	FirstPersonMesh->SetBoundsScale(2);
	FirstPersonMesh->SetCastShadow(false);

	bUseControllerRotationYaw = true;	// 캐릭터 몸체(액터) 자체가 좌우로 회전하도록
	GetCharacterMovement()->bOrientRotationToMovement = false; // 이동 방향으로 자동 회전하지 않게 (1인칭은 항상 카메라 보는 방향이 정면)

	// 크라우치 가능 모드로 세팅
	// 빈 프로젝트 시작시 기본 비활성화 / Third Person 탬플릿으로 시작하면 활성화 되어 있음
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);		// Standing: Radius, HalfHeight
	GetCharacterMovement()->SetCrouchedHalfHeight(60.f);		// Crouch 시 목표 HalfHeight

	// 메시 초기 위치 세팅
	FRotator Rot(0, -90, 0);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -88.f), Rot);

	// 눈높이(카메라) 포지션
	DefaultEyeHeight = BaseEyeHeight;
	CurrentWorldEyeHeight = GetActorLocation().Z + DefaultEyeHeight;

	// 스탯 컴포넌트 세팅
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	StatComponent->SetIsReplicated(true);

	/// 컴포넌트 생성
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interact"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	CorpseStorageComponent = CreateDefaultSubobject<UWarehouseInventoryComponent>(TEXT("CorpseStorage"));
	HeldItemComponent = CreateDefaultSubobject<UHeldItemComponent>(TEXT("HeldItemComponent"));
}

// Called when the game starts or when spawned
void AActionCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// PC의 카메라 상하각도 세팅
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->ViewPitchMax = ViewPicthMax;
			PC->PlayerCameraManager->ViewPitchMin = ViewPicthMin;
		}
	}
	if (HasAuthority() && StatComponent)
	{
		StatComponent->InitializeStat();
	}
	// 이동관련 파라미터 세팅
	ApplyMovementSettings();

	// 사망 델리게이트 연결
	if (StatComponent)
	{
		StatComponent->OnDeath.AddDynamic(
			this,
			&AActionCharacter::Die
		);
	}

	
	// 디버깅용 기본 아이템 지급 중 인벤토리 누락은 중단하고 비어 있는 아이템 항목은 건너뛰어 크래시 방지
	if (DefaultItems.Num() > 0)
	{
		if (!IsValid(InventoryComponent))
		{
			UE_LOG(LogTemp, Error, TEXT("%s has no valid InventoryComponent."), *GetName());
			return;
		}

		for (int32 Index = 0; Index < DefaultItems.Num(); ++Index)
		{
			UItemDataBase* Item = DefaultItems[Index];
			if (!IsValid(Item))
			{
				UE_LOG(LogTemp, Warning, TEXT("%s DefaultItems[%d] is empty."), *GetName(), Index);
				continue;
			}

			int32 Remain = 0;
			//for debug
			if (Item->DisplayName.ToString().Contains(TEXT("나무")) || Item->DisplayName.ToString().Contains(TEXT("돌조각")))
			{
				InventoryComponent->AddItem(Item,1000, Remain);

			}
			else
			{
				InventoryComponent->AddItem(Item, 1, Remain);
			}
		}

	}
	
}

// Called every frame
void AActionCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/// 크라우치모드(true/false)에 따라 카메라 높이 보간
	const float TargetLocalEyeHeight = bIsCrouched ? CrouchedEyeHeight : DefaultEyeHeight;
	const float TargetWorldEyeHeight = GetActorLocation().Z + TargetLocalEyeHeight; // 캡슐이 이미 이동한 뒤 기준

	CurrentWorldEyeHeight = FMath::FInterpTo(CurrentWorldEyeHeight, TargetWorldEyeHeight, DeltaTime, CrouchInterpSpeed);

	//const float LocalOffset = CurrentWorldEyeHeight - GetActorLocation().Z; // 현재 캡슐 위치 기준으로 역산
	//FirstPersonCamera->SetRelativeLocation(FVector(
	//	FirstPersonCamera->GetRelativeLocation().X, 
	//	FirstPersonCamera->GetRelativeLocation().Y,
	//	LocalOffset
	//));

	if (bIsSitting && CurrentVehicle && IsLocallyControlled())
	{
		const float CurrentVehicleYaw =
			CurrentVehicle->GetActorRotation().Yaw;

		const float DeltaYaw =
			FMath::FindDeltaAngleDegrees(
				VehicleYawOffset,
				CurrentVehicleYaw);

		if (!FMath::IsNearlyZero(DeltaYaw))
		{
			if (AController* PC = GetController())
			{
				FRotator ControlRotation =
					PC->GetControlRotation();

				ControlRotation.Yaw += DeltaYaw;

				PC->SetControlRotation(ControlRotation);
			}
		}

		VehicleYawOffset = CurrentVehicleYaw;
	}

	if (!bIsSleeping)
	{
		const float LocalOffset = CurrentWorldEyeHeight - GetActorLocation().Z; // 현재 캡슐 위치 기준으로 역산
		FirstPersonCamera->SetRelativeLocation(FVector(
			FirstPersonCamera->GetRelativeLocation().X,
			FirstPersonCamera->GetRelativeLocation().Y,
			LocalOffset
		));
	}
}

// Called to bind functionality to input
void AActionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// SetupPlayerInputComponent
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 이동, 회전
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AActionCharacter::OnMoveAction);
		EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &AActionCharacter::OnMoveCompleted);
		EIC->BindAction(IA_Move, ETriggerEvent::Canceled, this, &AActionCharacter::OnMoveCompleted);
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AActionCharacter::OnLookInput);

		// Jump는 눌렀을 때(Started) 시작, 뗐을 때(Completed) 멈춤
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AActionCharacter::OnJumpPressed);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	
		// 스프린트 (Started/Completed는 모드와 무관하게 항상 둘 다 바인딩)
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AActionCharacter::OnSprintPressed);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AActionCharacter::OnSprintReleased);

		// 크라우치 (Started/Completed 둘 다 바인딩)
		EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &AActionCharacter::OnCrouchPressed);
		EIC->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &AActionCharacter::OnCrouchReleased);

		// 상호작용
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AActionCharacter::OnInteractPressed);

		EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &AActionCharacter::OnAttackPressed);
		EIC->BindAction(IA_Attack, ETriggerEvent::Completed, this, &AActionCharacter::OnAttackReleased);

		// 인벤토리 토글은 AActionPlayerController::SetupInputComponent에 바인딩된다(폰이 바뀌어도
		// 유지돼야 하는 컨트롤러 레벨 UI 액션이라 캐릭터 쪽에 안 둠).

		// 벨트슬롯 단축키(1~6) — 창고/인벤토리 UI가 열려있는 동안엔 각 핸들러가
		// IsUIBlockingGameplayInput()으로 직접 걸러낸다(OnUseBeltSlotPressed 참고).
		EIC->BindAction(IA_Use_BeltSlot_1, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 0);
		EIC->BindAction(IA_Use_BeltSlot_2, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 1);
		EIC->BindAction(IA_Use_BeltSlot_3, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 2);
		EIC->BindAction(IA_Use_BeltSlot_4, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 3);
		EIC->BindAction(IA_Use_BeltSlot_5, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 4);
		EIC->BindAction(IA_Use_BeltSlot_6, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 5);

		// 보조 액션 (우클릭 / 도구 보조 기능 / 조준 등)
		if (IA_SecondaryAction)
		{
			EIC->BindAction(IA_SecondaryAction, ETriggerEvent::Started, this, &AActionCharacter::OnSecondaryActionPressed);
			EIC->BindAction(IA_SecondaryAction, ETriggerEvent::Completed, this, &AActionCharacter::OnSecondaryActionReleased);
		}

		// 손에 장착된 도구가 있다면 도구 고유 입력 바인딩 전달
		if (HeldItemComponent && HeldItemComponent->GetCurrentHeldItem())
		{
			HeldItemComponent->GetCurrentHeldItem()->SetupInputComponent(EIC);
		}

		// 좌클릭 중복 제거
		//if(IA_BuildingPlacement)
			//EIC->BindAction(IA_BuildingPlacement, ETriggerEvent::Started, this, &AActionCharacter::OnBuildingPlacementPressed);

		if (IA_RotateBuildingPart)
			EIC->BindAction(IA_RotateBuildingPart, ETriggerEvent::Started, this, &AActionCharacter::OnRotateBuildingPartPressed);
	}
}

void AActionCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AActionCharacter, bIsSprinting);
	DOREPLIFETIME(AActionCharacter, bIsSitting);
	DOREPLIFETIME(AActionCharacter, CurrentVehicle);
	DOREPLIFETIME(AActionCharacter, bIsSleeping);
}

void AActionCharacter::OnSecondaryActionPressed()
{
	if (IsUIBlockingGameplayInput()) return;

	// 배치 중에는 우클릭을 보조 액션보다 배치 취소로 우선 처리해요
	if (AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetController()))
	{
		if (true == PlayerController->TryCancelPlacement())
			return;
	}

	if (HeldItemComponent)
	{
		HeldItemComponent->UseSecondaryAction(true);
	}

}

void AActionCharacter::OnSecondaryActionReleased()
{
	if (HeldItemComponent)
	{
		HeldItemComponent->UseSecondaryAction(false);
	}
}

void AActionCharacter::ServerSetIsSprinting_Implementation(bool bIsSprintingNew)
{
	bIsSprinting = bIsSprintingNew;

	ApplyMovementSettings();
}

void AActionCharacter::SetSprintInputMode(ESprintInputMode NewNode)
{
	SprintInputMode = NewNode;
	bIsSprinting = false;		// 모드 전환 순간 스프린트 상태 꼬이는 것 방지 (Hold 누르고 있던 중 전환 등)
	ApplyMovementSettings();
}

void AActionCharacter::SetCrouchInputMode(ECrouchInputMode NewMode)
{
	CrouchInputMode = NewMode;
	UnCrouch(); // 모드 전환 시 안전하게 초기화 (Hold 누르고 있던 중 전환 등)
	ApplyMovementSettings();
}

void AActionCharacter::SetIsInVehicle(bool bIsInVehicleNew, bool bIsDriver)
{
	bIsSitting = bIsInVehicleNew;
	//LegMesh->SetVisibility(!bIsInVehicleNew);
	//FeetMesh->SetVisibility(!bIsInVehicleNew);

	VehicleYawOffset = CurrentVehicle->GetActorRotation().Yaw;

	GetCapsuleComponent()->SetCollisionEnabled(
		bIsInVehicleNew?
		ECollisionEnabled::NoCollision
       :ECollisionEnabled::QueryAndPhysics);
	bUseControllerRotationYaw = !bIsInVehicleNew;

	SetReplicateMovement(!bIsInVehicleNew);

	if (!HasAuthority())
	{
	}

	if (bIsDriver && IsLocallyControlled())
	{
		GetMesh()->SetVisibility(!(bIsInVehicleNew));
	}
}

void AActionCharacter::ServerRequestExitVehicle_Implementation()
{
	if (bIsSitting && CurrentVehicle)
	{
		CurrentVehicle->PassengerPressToExitVehicle(this);
	}
}

void AActionCharacter::OnRep_IsSitting()
{
	UE_LOG(LogTemp, Warning,
		TEXT("[ON REP SITTING] Char=%s Authority=%d Local=%d Sitting=%d"),
		*GetNameSafe(this),
		HasAuthority(),
		IsLocallyControlled(),
		bIsSitting
	);
	GetCapsuleComponent()->SetCollisionEnabled(
		bIsSitting
		? ECollisionEnabled::NoCollision
		: ECollisionEnabled::QueryAndPhysics
	);
	if (!bIsSitting)
	{
		GetMesh()->SetVisibility(true);
		return;
	}

	// 현재 로컬 PlayerController가 Possess하고 있는 Pawn
	AActionPlayerController* PC =
		Cast<AActionPlayerController>(GetWorld()->GetFirstPlayerController());

	if (!PC || !PC->IsLocalController())
		return;

	APawn* PossessedPawn = PC->GetPawn();

	if (!PossessedPawn)
		return;

	// 현재 Possess한 Pawn이 Vehicle인지 확인
	IVehicleInterface* VehicleInterface = Cast<IVehicleInterface>(PossessedPawn);

	if (!VehicleInterface) return;

	// 이 Character가 현재 Vehicle의 운전자인 경우에만
	// 로컬 화면에서 Mesh 숨김
	if (VehicleInterface->GetDriverCharacter() == this)
	{
		GetMesh()->SetVisibility(false);

		UE_LOG(LogTemp, Warning,
			TEXT("[LOCAL DRIVER] Hide Mesh - Character=%s"),
			*GetNameSafe(this)
		);
	}
}

void AActionCharacter::StartSleeping(AActor* InActor)
{
	if (ASleepingBag* SleepingBag = Cast<ASleepingBag>(InActor))
	{
		if (!HasAuthority()) return;
		bIsSleeping = true;
		OnRep_IsSleeping();
		CurrentSleepingBag = SleepingBag;

		// 침낭의 위치로 이동
		FVector Location = SleepingBag->GetActorLocation() + FVector(-30, 20, 100);
		//Location.Z = GetActorLocation().Z;
		FRotator Rotator = SleepingBag->GetActorForwardVector().Rotation();
		SetActorLocationAndRotation(Location, Rotator);
		//SetActorLocation(Location);

		if (AController* C = GetController())
		{
			C->SetControlRotation(Rotator);
		}
		// 자는동안 이동 막기
		GetCharacterMovement()->DisableMovement();
	}
	
}

void AActionCharacter::StopSleeping()
{
	if (!HasAuthority()) return;
	bIsSleeping = false;
	OnRep_IsSleeping();

	if (ASleepingBag* SleepingBag = Cast<ASleepingBag>(CurrentSleepingBag))
	{
		SleepingBag->ClearOccupant(this);
		CurrentSleepingBag = nullptr;
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

}

void AActionCharacter::ServerRequestWakeUp_Implementation()
{
	StopSleeping();
}

void AActionCharacter::OnRep_IsSleeping()
{
	if (bIsSleeping)
	{
		if (SleepingMontage)
		{
			PlayAnimMontage(SleepingMontage);

			

			// 1인칭 팔 지우기
			FirstPersonMesh->SetVisibility(false);

			// 자는동안 몸 회전 막기
			bUseControllerRotationYaw = false;
			FirstPersonCamera->bUsePawnControlRotation = false;

			CameraPosCache = FirstPersonCamera->GetRelativeLocation();
			CameraRotCache = FirstPersonCamera->GetRelativeRotation();

			FirstPersonCamera->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				FName(TEXT("HeadCamera"))
			);
			FirstPersonCamera->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		}
	}
	else
	{
		if (SleepingMontage)
		{
			StopAnimMontage(SleepingMontage);
			FirstPersonMesh->SetVisibility(true);

			bUseControllerRotationYaw = true;
			FirstPersonCamera->bUsePawnControlRotation = true;

			FirstPersonCamera->AttachToComponent(
				GetCapsuleComponent(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale
			);

			FirstPersonCamera->SetRelativeLocationAndRotation(CameraPosCache, CameraRotCache);
		}
	}
}

void AActionCharacter::ProcessAttack()
{
	// 여기 들어왔다는건 일단 휘둘렀다는 뜻.
	// TODO 하드코딩 수정
	// TODO DrainSurvivalStats 열어줄 수 있는지 물어볼 것
	//ICaloriesInterface::Execute_DecreaseCalories(StatComponent, 10.016f);
	//IHydrationInterface::Execute_DecreaseHydration(StatComponent, 10.0032f);

	if (!IsLocallyControlled()) return;
	//TODO 무기 타입에 따라서 세분화
	FHitResult DetectRes;
	if (DetectdObjectInAttackRange(DetectRes))
	{
		AActor* Target = DetectRes.GetActor();
		if (!Target) return;

		// 서버 권한으로 타격 및 자원 채집 처리 요청
		Server_ProcessAttackTarget(Target, DetectRes.ImpactPoint);
	}
}

bool AActionCharacter::Server_ProcessAttackTarget_Validate(AActor* TargetActor, const FVector& HitLocation)
{
	// Validate에서 false를 반환하면 언리얼 엔진이 클라이언트를 즉시 강제 종료(Kick)하므로,
	// 대상 액터가 파괴/소멸 중이더라도 연결이 끊기지 않도록 true를 반환하고
	// 실제 널 체크 및 유효성 검사는 Implementation 내부에서 안전하게 처리합니다.
	return true;
}

void AActionCharacter::Server_ProcessAttackTarget_Implementation(AActor* TargetActor, const FVector& HitLocation)
{
	if (!TargetActor || !IsValid(TargetActor)) return;

	// 1. 자원이 아니라 공격을 받는 대상인 경우 
	if (IHealthInterface* IHealth = Cast<IHealthInterface>(TargetActor))
	{
		// 공격 실행
		if (IHealth->IsAlive())
		{
			//TODO 하드코딩 수정
			IHealthInterface::Execute_InflictDamage(TargetActor, HeldItemComponent->GetCurrentHeldItem()->GetItemData()->Damage);
			return;
		}

	}

	// 2. 자원을 얻을 수 있는 대상인지 확인
	if (UHarvestableComponent* HarvestComp = TargetActor->FindComponentByClass<UHarvestableComponent>())
	{
		// 서버에서 자원 획득 진행 (OnHitted_Implementation 실행)
		FHarvestRes HarvRes = IHarvestable::Execute_OnHitted(HarvestComp, this, HeldItemComponent->GetCurrentHeldItem(), HitLocation);
		if (HarvRes.HarvesResult)
		{
			// 서버에서 만들어준 자원을 인벤토리에 넣는다.
			for (const FHarvestItemResult& ItemRes : HarvRes.HarvestedItems)
			{
				if (ItemRes.ItemData && InventoryComponent)
				{
					int32 RemainCnt = 0;
					InventoryComponent->AddItem(ItemRes.ItemData, ItemRes.Count, RemainCnt);
					InventoryComponent->NotifyItemAcquired(ItemRes.ItemData, ItemRes.Count - RemainCnt);

					//// For Debug
					//UE_LOG(LogTemp, Display, TEXT("[서버] 자원 [%s]를 %d개 획득! (스위트스팟: %s, 고갈보너스: %s)"),
					//	*(ItemRes.ItemData->DisplayName.ToString()),
					//	ItemRes.Count,
					//	HarvRes.bHitSweetSpot ? TEXT("O") : TEXT("X"),
					//	HarvRes.bIsDepleted ? TEXT("O") : TEXT("X"));
				}
			}
		}
	}

}

bool AActionCharacter::Server_GrantHarvestReward_Validate(UItemDataBase* ItemData, int32 Count)
{
	return ItemData != nullptr && Count > 0;
}

void AActionCharacter::Server_GrantHarvestReward_Implementation(UItemDataBase* ItemData, int32 Count)
{
	if (!InventoryComponent)
	{
		return;
	}

	int32 RemainCnt = 0;
	InventoryComponent->AddItem(ItemData, Count, RemainCnt);
	InventoryComponent->NotifyItemAcquired(ItemData, Count - RemainCnt);
}

void AActionCharacter::Die()
{
	if (bIsSleeping)
		StopSleeping();

	if (!HasAuthority()) return;

	// UnPossess 전에
	// 개인 제작 큐를 취소해 환불 재료와 지급 대기 중인 완성품을 이 캐릭터의 인벤토리에 넣고,
	// 남은 수량은 곧 랙돌이 될 캐릭터 주변에 드롭
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UCraftingComponent* Crafting = PlayerController->FindComponentByClass<UCraftingComponent>())
		{
			Crafting->CancelPersonalCraftingOnDeath(this);
		}
	}

	// 제작 큐 반환 후, 인벤토리에 있던 아이템을 전부 시체 창고에 옮기기
	MoveInventoryToCorpseStorage();

	MulticastDie();

}

void AActionCharacter::MulticastDie_Implementation()
{
	//// 1. 캡슐 충돌 완전 비활성화 (캡슐이 바닥에 걸려 렉돌과 부딪히는 것 방지)
	//GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	//// 2. 캐릭터 이동 컴포넌트 정지
	//if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	//{
	//	MoveComp->StopMovementImmediately();
	//	MoveComp->DisableMovement();
	//	MoveComp->SetComponentTickEnabled(false);
	//}
	//// 3. 메인 메시(GetMesh) 렉돌 물리 시뮬레이션 시작
	//GetMesh()->SetAnimInstanceClass(nullptr); // 애니메이션 정지
	//GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	//GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//GetMesh()->SetSimulatePhysics(true);
	//GetMesh()->WakeAllRigidBodies();
	//// 4. 모듈형 파츠 메시(Follower) 세팅
	//// 중요: 파츠들은 자체 물리를 켜지 않고(false), 충돌도 끕니다(NoCollision).
	//// LeaderPoseComponent가 메인 메시의 물리 뼈 위치를 그대로 복사해서 렌더링합니다.
	//TArray<USkeletalMeshComponent*> ModularParts = { TorsoMesh, LegMesh, HandMesh, FeetMesh };
	//for (USkeletalMeshComponent* Part : ModularParts)
	//{
	//	if (Part)
	//	{
	//		Part->SetSimulatePhysics(true);
	//		//Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//		// 리더가 물리 연산(TG_EndPhysics)을 마친 뒤 파츠가 갱신되도록 틱 의존성 보장
	//		Part->AddTickPrerequisiteComponent(GetMesh());
	//	}
	//}
	//// 5. 1인칭 프로젝트(R1) 전용 시각화 처리
	//if (IsLocallyControlled())
	//{
	//	// 1인칭 전용 팔 숨기기
	//	if (FirstPersonMesh)
	//	{
	//		FirstPersonMesh->SetVisibility(false);
	//	}
	//	// 죽었을 때는 내 3인칭 렉돌 몸뚱이가 보이도록 OwnerNoSee 해제
	//	GetMesh()->SetOwnerNoSee(false);
	//	for (USkeletalMeshComponent* Part : ModularParts)
	//	{
	//		if (Part)
	//		{
	//			Part->SetOwnerNoSee(false);
	//		}
	//	}
	//	// (선택) 카메라를 머리 본에 붙여 바닥에 쓰러지는 1인칭 시점 연출
	//	if (FirstPersonCamera)
	//	{
	//		FirstPersonCamera->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(TEXT("head")));
	//	}
	//}
	//// 6. 컨트롤러 연결 해제
	//if (APlayerController* PC = Cast<APlayerController>(GetController()))
	//{
	//	PC->UnPossess();
	//}

	// Looting: 서버와 클라이언트 모두 사망 후 이동 계산 중단
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
		Movement->SetComponentTickEnabled(false);
	}

	// 캡슐 컴포넌트 충돌 끄기
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 애니메이션 중지
	//GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	//GetMesh()->Stop();
	GetMesh()->SetAnimInstanceClass(nullptr);

	// 메쉬 랙돌 전환
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetMesh()->SetSimulatePhysics(true);
	if (HasAuthority())
	{
		// 컨트롤러 연결 해제: 서버만 실행
		// -> 어떤 컨트롤러가 어떤 캐릭터를 조종하는지는 서버가 결정하기 때문
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->UnPossess();
		}
	}
}

void AActionCharacter::MoveInventoryToCorpseStorage()
{
	// 서버가 수행, 이미 시체 창고 이관/준비 됐는지, 인벤토리 컴포넌트 있는지, 시체용 창고 컴포넌트 있는지
	if (!HasAuthority() || bCorpseInventoryPrepared || !InventoryComponent || !CorpseStorageComponent)
		return;

	bCorpseInventoryPrepared = true;	// 사망 아이템이 중복 이관되지 않도록 표시

	/* 시체 창고 슬롯 수 변경 */
	// 시체 창고에 필요한 슬롯 수 = 메인 인벤토리 슬롯 수 + 벨트 슬롯 수 + 장비 슬롯 수
	const int32 RequiredSlotCount = InventoryComponent->MainSlots.Num() + InventoryComponent->BeltSlots.Num() + InventoryComponent->EquipmentSlots.Num();
	CorpseStorageComponent->StorageSlotCount = FMath::Max(CorpseStorageComponent->StorageSlotCount, RequiredSlotCount);	// 원래 창고 슬롯 수랑 인벤토리 전체 슬롯 수 중 큰 값으로 최종 슬롯 수 결정
	CorpseStorageComponent->StorageSlots.SetNum(CorpseStorageComponent->StorageSlotCount);	// 시체 창고 실제 슬롯 수 변경

	/* 인벤토리에 있는 아이템들을 시체 창고에 옮기기 */
	int32 CorpseSlotIndex = 0;	// 현재 보고 있는 시체 창고 슬롯 인덱스

	// 현재 함수 안에서 메인+벨트+장비 배열에만 반복 사용하기 때문에 람다로 아이템 이동 구현
	auto MoveSlots = [this, &CorpseSlotIndex](EInventorySlotCategory Category, const TArray<FItemInstance>& Slots)
	{
		// Slots에 들어온 배열 순회
		for (int32 SourceIndex = 0; SourceIndex < Slots.Num(); ++SourceIndex)
		{
			const FItemInstance Item = Slots[SourceIndex];	// 현재 슬롯에 있는 아이템 복사
			if (!Item.IsValid())
				continue;	// 현재 슬롯의 아이템이 유효하지 않으면 다음 슬롯 검사

			CorpseStorageComponent->SetSlotItem(CorpseSlotIndex++, Item);									// 현재 보고 있는 시체 창고 슬롯 채우기 (서버 권한 및 변경 이벤트 발생)
			InventoryComponent->SetSlotItem(FInventorySlotRef{ Category, SourceIndex }, FItemInstance());	// 현재 보고 있는 원래 인벤토리 슬롯 비우기 (서버 권한 및 변경 이벤트 발생)
		}
	};

	MoveSlots(EInventorySlotCategory::Main, InventoryComponent->MainSlots);				// 인벤토리_메인 아이템 이동
	MoveSlots(EInventorySlotCategory::Belt, InventoryComponent->BeltSlots);				// 인벤토리_벨트 아이템 이동
	MoveSlots(EInventorySlotCategory::Equipment, InventoryComponent->EquipmentSlots);	// 인벤토리_장비 아이템 이동
}

FText AActionCharacter::GetInteractionDisplayName_Implementation() const
{
	return FText::FromString(TEXT("시체"));	// 액터 이름 대신 시체 문구 보이기
}

bool AActionCharacter::CanInteract_Implementation(APawn* Interactor) const
{
	// 스스로이거나 캐릭터가 살아있거나 시체 창고가 없는 경우에는 상호작용 불가
	if (!Interactor || Interactor == this || !StatComponent || StatComponent->IsAlive() || !CorpseStorageComponent)
		return false;

	// 거리 확인 (제곱인 상태로 비교하는 게 빠름)
	return FVector::DistSquared(Interactor->GetActorLocation(), CorpseStorageComponent->GetInteractionLocation()) <= FMath::Square(CorpseStorageComponent->MaxInteractDistance);
}

void AActionCharacter::Interact_Implementation(APawn* Interactor)
{
	// 권한 및 상호작용 가능 여부 확인
	if (!HasAuthority() || !CanInteract_Implementation(Interactor))
		return;

	// 시체 창고 열기
	if (AActionPlayerController* PC = Cast<AActionPlayerController>(Interactor->GetController()))
	{
		PC->Client_OpenWarehouse(CorpseStorageComponent);
	}
}


bool AActionCharacter::CanJumpInternal_Implementation() const
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	// 엔진 기본 구현에서 "!bIsCrouched" 체크만 제외하고 재구현.

	// CanAttemptJump()를 그대로 쓰면 크라우치 중엔 항상 막힘.
	// IsJumpAllowed()는 유지하고 크라우치 조건만 제외해서 직접 조합한다.
	bool bCanJump = MoveComp && MoveComp->IsJumpAllowed()
		&& (MoveComp->IsMovingOnGround() || MoveComp->IsFalling());

	if (bCanJump)
	{
		if (JumpCurrentCount == 0 && MoveComp->IsFalling())
		{
			bCanJump = JumpCurrentCount + 1 < JumpMaxCount;
		}
		else
		{
			bCanJump = JumpCurrentCount < JumpMaxCount;
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("CanJumpInternal called, bIsCrouched=%d, bCanJump=%d"), bIsCrouched, bCanJump);

	return bCanJump;
}

void AActionCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	//UE_LOG(LogTemp, Warning, TEXT("OnJumped_Implementation called! UnCrouching now."));
	if (bIsCrouched)
	{
		UnCrouch(); // 점프가 실제로 발동된 뒤에 크라우치를 풀어준다
	}
}

UStatComponent* AActionCharacter::GetStatComponent() const
{
	return StatComponent;
}

bool AActionCharacter::IsUIBlockingGameplayInput() const
{
	const AActionPlayerController* PC = Cast<AActionPlayerController>(GetController());
	return PC && PC->IsAnyUIPanelOpen();
}

void AActionCharacter::OnMoveAction(const FInputActionValue& InValue)
{
	const FVector2D MoveValue = InValue.Get<FVector2D>();

	// 손에 든 도구/무기가 이동 차단 중일 때 (예: 낚시 중 A/D 저항, S 릴 감기)
	if (HeldItemComponent && HeldItemComponent->BlocksCharacterMovement())
	{
		HeldItemComponent->OnMoveInput(MoveValue);
		return;
	}

	AddMovementInput(GetActorForwardVector(), MoveValue.Y);
	AddMovementInput(GetActorRightVector(), MoveValue.X);

	//UE_LOG(LogTemp, Log, TEXT("OnMoveAction"));
}

void AActionCharacter::OnMoveCompleted(const FInputActionValue& InValue)
{
	// 이동 키를 뗐을 때 손에 든 도구에 중립 입력(0, 0) 전달
	if (HeldItemComponent && HeldItemComponent->BlocksCharacterMovement())
	{
		HeldItemComponent->OnMoveInput(FVector2D::ZeroVector);
	}
}

void AActionCharacter::OnLookInput(const FInputActionValue& InValue)
{
	// UI가 열려있는 동안엔 마우스가 커서 조작용이라 시야 회전에 쓰면 안 된다(이동은 계속 받되
	// 카메라만 막는다) — IsUIBlockingGameplayInput 참고.
	if (bIsSleeping || IsUIBlockingGameplayInput())
	{
		return;
	}

	const FVector2D LookValue = InValue.Get<FVector2D>();
	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);

	//UE_LOG(LogTemp, Log, TEXT("OnLookInput"));
}

void AActionCharacter::OnSprintPressed()
{
	/// UI열려도 동작하도록 주석처리
	//if (IsUIBlockingGameplayInput()) return;	// UI가 열려있는 동안엔 스프린트 입력을 무시

	// 도구 액션 중이거나 크라우치 모드에는 스프린트 안함
	if (bIsCrouched || (HeldItemComponent && HeldItemComponent->BlocksCharacterMovement())) return;

	if (SprintInputMode == ESprintInputMode::Toggle)
	{
		bIsSprinting = !bIsSprinting;	// 누를 때만 반전

		//UE_LOG(LogTemp, Log, TEXT("OnSprintPressed  Toggle: %d"), SprintInputMode);
	}
	else  // Hold
	{
		bIsSprinting = true;
		//UE_LOG(LogTemp, Log, TEXT("OnSprintPressed Started"));
	}
	ApplyMovementSettings();

	// 서버에 RPC 보내기
	if (!HasAuthority())
	{
		ServerSetIsSprinting(bIsSprinting);
	}
}

void AActionCharacter::OnSprintReleased()
{
	if (SprintInputMode == ESprintInputMode::Hold)
	{
		bIsSprinting = false;
		ApplyMovementSettings();
		//UE_LOG(LogTemp, Log, TEXT("OnSprintPressed Released"));

		// 서버에 RPC 보내기
		if (!HasAuthority())
		{
			ServerSetIsSprinting(bIsSprinting);
		}
	}
	// Toggle 모드에서는 뗄 때 아무것도 안 함
}

void AActionCharacter::OnCrouchPressed()
{
	/// UI열려도 동작하도록 주석처리
	//if (IsUIBlockingGameplayInput()) return;	// UI가 열려있는 동안엔 크라우치 입력을 무시

	if (HeldItemComponent && HeldItemComponent->BlocksCharacterMovement()) return;

	if (CrouchInputMode == ECrouchInputMode::Toggle)
	{
		if (bIsCrouched)
		{
			UnCrouch();
			//UE_LOG(LogTemp, Log, TEXT("UnCrouch"));
		}
		else
		{
			bIsSprinting = false; // 상호배타 규칙 유지
			Crouch();
			//UE_LOG(LogTemp, Log, TEXT("Crouch"));
		}
	}
	else // Hold
	{
		bIsSprinting = false;
		Crouch();
		//UE_LOG(LogTemp, Log, TEXT("Crouch Hold"));
	}
	ApplyMovementSettings();
}

void AActionCharacter::OnCrouchReleased()
{
	if (CrouchInputMode == ECrouchInputMode::Hold)
	{
		UnCrouch();
		ApplyMovementSettings();
	}
	// Toggle 모드에서는 뗄 때 아무것도 안 함
}

void AActionCharacter::OnJumpPressed()
{
	/// UI열려도 동작하도록 주석처리
	//if (IsUIBlockingGameplayInput()) return;	// UI가 열려있는 동안엔 점프 입력을 무시
	if (HeldItemComponent && HeldItemComponent->BlocksCharacterMovement()) return;
	Jump();
}

void AActionCharacter::OnBuildingPlacementPressed()
{
	if (IsUIBlockingGameplayInput()) return;

	// 플레이어 컨트롤러에게 건축 배치를 맡김
	if (AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetController()))
		PlayerController->OnConfirmBuildingPlacement();
}

void AActionCharacter::OnRotateBuildingPartPressed()
{
	if (IsUIBlockingGameplayInput()) return;

	if (AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetController()))
		PlayerController->OnRotateBuildingPart();
}

void AActionCharacter::OnInteractPressed()
{
	if (bIsSleeping)
	{
		ServerRequestWakeUp();
		return;
	}

	if (bIsSitting && CurrentVehicle)
	{
		ServerRequestExitVehicle();
		return;
	}

	// BP_PlayerCharacter의 상속 컴포넌트 템플릿이 깨져서 멤버 포인터가 널로 읽히는 환경 문제가
	// 있어(OnUseBeltSlotPressed 참고) InteractionComponent가 null일 수 있다 — 가드 없이 호출하면
	// TryInteract() 진입 시점에 크래시난다.
	if (false == IsValid(InteractionComponent)) return;

	InteractionComponent->TryInteract();
}

void AActionCharacter::OnUseBeltSlotPressed(int32 BeltIndex)
{
	// 인벤토리/창고 UI가 열려있는 동안엔 예전처럼 DefaultMappingContext 제거로 이 액션이 아예
	// 안 눌렸는데, 이제 DefaultMappingContext는 이동을 위해 항상 켜져있으므로 여기서 직접 막는다.
	if (IsUIBlockingGameplayInput()) return;

	/// 임시 코드
	// InventoryComponent 멤버 대신 FindComponentByClass로 찾는다 — BP_PlayerCharacter의
	// 상속 컴포넌트 템플릿이 깨져서 멤버 포인터가 널로 읽히는 환경 문제가 있어(원인 조사 중),
	// UInventoryWidget/UBeltBarWidget이 이미 쓰고 있는 것과 같은 방식으로 우회한다.
	//if (UInventoryComponent* Inventory = FindComponentByClass<UInventoryComponent>())
	//{
	//	Inventory->UseBeltSlot(BeltIndex);
	//}

	if (false == IsValid(InventoryComponent)) return;

	if (InventoryComponent->BeltSlots.IsValidIndex(BeltIndex))
	{
		const FItemInstance& Instance = InventoryComponent->BeltSlots[BeltIndex];

		// Placeable 아이템은 즉시 소비하지 않고 로컬 설치 프리뷰를 시작해요
		if (Instance.IsValid() && EItemCategory::Placeable == Instance.ItemData->Category)
		{
			UPlaceableItemData* PlaceableData = Cast<UPlaceableItemData>(Instance.ItemData);
			AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetController());

			if (true == IsValid(PlaceableData) && true == IsValid(PlayerController))
			{
				// 설치 확정 시에 동일한 원본 아이템을 검증해야 하기 떄문에
				// 슬롯 위치랑 InstanceID를 전달해요
				const FInventorySlotRef SourceSlot{EInventorySlotCategory::Belt, BeltIndex};
				PlayerController->OnStartPlaceablePlacement(PlaceableData, SourceSlot, Instance.InstanceID);
			}
			return; // Placeable은 선택 시점에 아이템을 소비하지 않아요
		}

		// 다른 아이템을 선택하면 진행중이던 Placeable 설치 모드 종료
		if (true == Instance.IsValid())
		{
			if (AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetController()))
				PlayerController->OnStopPlacement();
		}
	}

	// 기존 HeldItem·Equipment·Consumable 사용은 서버에서 처리해요
	if (InventoryComponent)
	{
		InventoryComponent->Server_UseBeltSlot(BeltIndex);
	}
}

void AActionCharacter::OnAttackPressed()
{
	// Placeable 아이템 배치중인 경우 좌클릭을 공격 대신 설치 확정으로 처리
	if (AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetController()))
	{
		if (true == PlayerController->TryConfirmPlacement())
			return;
	}

	// 손에 도구/무기가 장착되어 있으면 도구 주 액션(Primary Action) 실행
	if (HeldItemComponent && HeldItemComponent->GetCurrentHeldItem())
	{
		HeldItemComponent->UsePrimaryAction(true);
		return;
	}
}

void AActionCharacter::OnAttackReleased()
{
	if (HeldItemComponent && HeldItemComponent->GetCurrentHeldItem())
	{
		HeldItemComponent->UsePrimaryAction(false);
	}
}


void AActionCharacter::ApplyMovementSettings()
{
	if (StatComponent && EnumHasAnyFlags(StatComponent->Execute_GetCurrentStatusEffect(StatComponent), EStatusEffect::Thirsty | EStatusEffect::Dehydrated)) return;	// 목마름 혹은 탈수일 경우 달리기 금지
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// 이동속도 세팅
		MoveComp->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
		MoveComp->MaxWalkSpeedCrouched = CrouchSpeed;

		// 점프파워 세팅
		MoveComp->JumpZVelocity = JumpPower;
	}
}

bool AActionCharacter::DetectdObjectInAttackRange(FHitResult& OutHitRes)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (APlayerCameraManager* CameraManger = PC->PlayerCameraManager)
		{
			// 카메라의 위치에서 사정거리만큼 line trace
			FVector StartPos = CameraManger->GetCameraLocation();
			FVector EndPos = StartPos + CameraManger->GetCameraRotation().Vector() * AttackRange;

			// 나는 제외
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			return GetWorld()->LineTraceSingleByChannel(OutHitRes, StartPos, EndPos, ECC_Visibility, Params);
		}
	}
	return false;
}

UInventoryComponent* AActionCharacter::GetInventoryComponent() const
{
	if (false == IsValid(InventoryComponent)) return nullptr;

	return InventoryComponent;
}




