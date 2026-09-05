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
#include "Component/StatComponent.h"	
#include "Component/InteractionComponent.h"
#include "Component/InventoryComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "Character/ActionPlayerController.h"
#include "Framework/MainHUD.h"
#include "Widget/MainHUDWidget.h"
#include "Data/Item/ItemDataBase.h"

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
	if (StatComponent)
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

		// 인벤토리 토글
		EIC->BindAction(IA_InventoryToggle, ETriggerEvent::Started, this, &AActionCharacter::OnInventoryTogglePressed);

		// 벨트슬롯 단축키(1~6) — 인벤토리가 열려있는 동안엔 DefaultMappingContext 자체가 빠져있어서
		// 이 액션들도 같이 안 눌린다(ActionPlayerController::SetInventoryInputState 참고).
		EIC->BindAction(IA_Use_BeltSlot_1, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 0);
		EIC->BindAction(IA_Use_BeltSlot_2, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 1);
		EIC->BindAction(IA_Use_BeltSlot_3, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 2);
		EIC->BindAction(IA_Use_BeltSlot_4, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 3);
		EIC->BindAction(IA_Use_BeltSlot_5, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 4);
		EIC->BindAction(IA_Use_BeltSlot_6, ETriggerEvent::Started, this, &AActionCharacter::OnUseBeltSlotPressed, 5);

		// 공격 (좌클릭 / 도구 주 액션)
		if (IA_Attack)
		{
			//EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &AActionCharacter::OnAttackPressed);
		}

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

		if(IA_BuildingPlacement)
			EIC->BindAction(IA_BuildingPlacement, ETriggerEvent::Started, this, &AActionCharacter::OnBuildingPlacementPressed);

		if (IA_RotateBuildingPart)
			EIC->BindAction(IA_RotateBuildingPart, ETriggerEvent::Started, this, &AActionCharacter::OnRotateBuildingPartPressed);
	}
}

void AActionCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AActionCharacter, bIsSprinting);
}

void AActionCharacter::OnSecondaryActionPressed()
{
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

void AActionCharacter::ProcessAttack()
{
	// 여기 들어왔다는건 일단 휘둘렀다는 뜻.
	// TODO 하드코딩 수정
	// TODO DrainSurvivalStats 열어줄 수 있는지 물어볼 것
	ICaloriesInterface::Execute_DecreaseCalories(StatComponent, 10.016f);
	IHydrationInterface::Execute_DecreaseHydration(StatComponent, 10.0032f);

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
	else
	{
		UE_LOG(LogTemp, Display, TEXT("아무도 것도 맞지 않았습니다."));
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
			IHealthInterface::Execute_InflictDamage(TargetActor, 50.f);
			return;
		}

	}

	// 2. 자원을 얻을 수 있는 대상인지 확인
	if (UHarvestableComponent* HarvestComp = TargetActor->FindComponentByClass<UHarvestableComponent>())
	{
		// 서버에서 자원 획득 진행 (OnHitted_Implementation 실행)
		FHarvestRes HarvRes = IHarvestable::Execute_OnHitted(HarvestComp, this, HitLocation);
		if (HarvRes.HarvesResult)
		{
			// 서버에서 만들어준 자원을 인벤토리에 넣는다.
			for (const FHarvestItemResult& ItemRes : HarvRes.HarvestedItems)
			{
				if (ItemRes.ItemData && InventoryComponent)
				{
					int32 RemainCnt = 0;
					InventoryComponent->AddItem(ItemRes.ItemData, ItemRes.Count, RemainCnt);

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
}

void AActionCharacter::Die()
{
	if (!HasAuthority()) return;

	MulticastDie();

}

void AActionCharacter::MulticastDie_Implementation()
{
	// 캡슐 컴포넌트 충돌 끄기
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 애니메이션 중지
	//GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	//GetMesh()->Stop();
	GetMesh()->SetAnimInstanceClass(nullptr);
	// 메쉬 랙돌 전환
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
	// 컨트롤러 연결 해제
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->UnPossess();
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
	const FVector2D LookValue = InValue.Get<FVector2D>();
	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);

	//UE_LOG(LogTemp, Log, TEXT("OnLookInput"));
}

void AActionCharacter::OnSprintPressed()
{
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
	if (HeldItemComponent && HeldItemComponent->BlocksCharacterMovement()) return;
	Jump();
}

void AActionCharacter::OnBuildingPlacementPressed()
{

	// 플레이어 컨트롤러에게 건축 배치를 맡김
	if (AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetController()))
		PlayerController->OnConfirmBuildingPlacement();
}

void AActionCharacter::OnRotateBuildingPartPressed()
{
	if (AActionPlayerController* PlayerController = Cast<AActionPlayerController>(GetController()))
		PlayerController->OnRotateBuildingPart();
}

void AActionCharacter::OnInteractPressed()
{
	InteractionComponent->TryInteract();

	//UE_LOG(LogTemp, Log, TEXT("TryInteract()"));
}

void AActionCharacter::OnInventoryTogglePressed()
{
	AActionPlayerController* PC = Cast<AActionPlayerController>(GetController());
	UE_LOG(LogTemp, Warning, TEXT("[InvToggle] OnInventoryTogglePressed. PC=%s"), *GetNameSafe(PC));
	if (!PC)
	{
		return;
	}

	AMainHUD* HUD = PC->GetHUD<AMainHUD>();
	UMainHUDWidget* MainHudWidget = HUD ? HUD->GetMainHudWidget() : nullptr;
	UE_LOG(LogTemp, Warning, TEXT("[InvToggle] HUD=%s, MainHudWidget=%s"), *GetNameSafe(HUD), *GetNameSafe(MainHudWidget));
	if (!MainHudWidget)
	{
		return;
	}

	const bool bIsOpen = MainHudWidget->ToggleInventoryPanel();
	UE_LOG(LogTemp, Warning, TEXT("[InvToggle] ToggleInventoryPanel returned bIsOpen=%d"), bIsOpen);
	PC->SetInventoryInputState(bIsOpen);
}

void AActionCharacter::OnUseBeltSlotPressed(int32 BeltIndex)
{
	/// 임시 코드
	// InventoryComponent 멤버 대신 FindComponentByClass로 찾는다 — BP_PlayerCharacter의
	// 상속 컴포넌트 템플릿이 깨져서 멤버 포인터가 널로 읽히는 환경 문제가 있어(원인 조사 중),
	// UInventoryWidget/UBeltBarWidget이 이미 쓰고 있는 것과 같은 방식으로 우회한다.
	//if (UInventoryComponent* Inventory = FindComponentByClass<UInventoryComponent>())
	//{
	//	Inventory->UseBeltSlot(BeltIndex);
	//}

	if (InventoryComponent)
	{
		InventoryComponent->Server_UseBeltSlot(BeltIndex);
	}
}

void AActionCharacter::OnAttackPressed()
{
	// 손에 도구/무기가 장착되어 있으면 도구 주 액션(Primary Action) 실행
	if (HeldItemComponent && HeldItemComponent->GetCurrentHeldItem())
	{
		HeldItemComponent->UsePrimaryAction(true);
		return;
	}

	//if (!AM_Attack)
	//{
	//	UE_LOG(LogTemp, Display, TEXT("AM_Attack was nullptr"));
	//	return;
	//}

	//if (UAnimInstance* Instance = GetMesh()->GetAnimInstance())
	//{
	//	if (!Instance->IsAnyMontagePlaying())
	//	{
	//		// 1) 로컬 클라이언트 선행 재생 (인풋 랙 제거)
	//		PlayAnimMontage(AM_Attack);

	//		// 2) 리슨 서버 및 다른 클라이언트 동기화
	//		if (!HasAuthority())
	//		{
	//			Server_PlayAttackMontage();
	//		}
	//		else
	//		{
	//			Multicast_PlayAttackMontage();
	//		}
	//	}
	//}
}

void AActionCharacter::Server_PlayAttackMontage_Implementation()
{
	Multicast_PlayAttackMontage();
}

void AActionCharacter::Multicast_PlayAttackMontage_Implementation()
{
	// 이미 로컬에서 선행 재생한 공격자 본인은 중복 재생 방지를 위해 건너뜀
	if (IsLocallyControlled())
	{
		return;
	}

	PlayAnimMontage(AM_Attack);
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




