/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 캐릭터에 붙는 상호작용 컴포넌트.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractableTargetChanged, AActor*, NewTarget, const FText&, DisplayName);

/**
 * 캐릭터에 붙는 상호작용 컴포넌트.
 *
 * 매 틱 카메라 방향으로 라인 트레이스해서 IInteractable을 구현한 액터를
 * 조준 중인지 확인하고, 조준 대상이 "바뀔 때만" OnInteractableTargetChanged를
 * 브로드캐스트한다 — UI(크로스헤어 위젯)는 여기 바인딩해서 이름 표시를
 * 보이거나 숨기면 된다. 실제 상호작용 실행은 TryInteract()를 입력
 * 액션(예: IA_Interact)에 바인딩해서 호출하는 식으로 쓴다.
 *
 * 아이템 픽업(AItemPickup)뿐 아니라 창고/전리품 상자 등 IInteractable을
 * 구현하는 모든 액터에 공통으로 쓰이는 게 이 컴포넌트를 따로 뺀 이유다.
 *
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class R1_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractionComponent();

	// 현재 조준 중인 대상에게 실제 상호작용을 실행한다. 입력 액션에 바인딩해서 호출.
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateTargeting();

public:
	// 타겟 감지 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	float TraceDistance = 300.f;

	/// TODO : 추후 별도 상호작용 트레이스 채널이 생기면 교체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// 타겟 갱신시 브로드캐스트할 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableTargetChanged OnInteractableTargetChanged;

private:
	// 현재 타겟
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;
};
