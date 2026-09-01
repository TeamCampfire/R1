/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 캐릭터에 붙는 상호작용 컴포넌트.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "InteractionComponent.generated.h"

// Icon은 UI에서 바로 그릴 수 있게 이미 로드된 UTexture2D*로 넘긴다(대상이 바뀔 때만
// 쏘는 이벤트라 동기 로드해도 비용이 크지 않음) 
// — TSoftObjectPtr을 그대로 넘기면 UI(WBP) 쪽에서 매번 로드 처리를 해야 해서 델리게이트 소비 쪽이 번거로워진다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInteractableTargetChanged, AActor*, NewTarget, const FText&, DisplayName, UTexture2D*, Icon);

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
	// 상호작용 가능 판단 후 서버에 요청만 함.
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	// 실제로 상호작용 동작하는 서버측 실행 함수
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TryInteract(AActor* Target);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateTargeting();

	// Actor의 모든 프리미티브 컴포넌트에 Custom Depth 렌더링을 켜고/끈다 
	// — 어떤 인터랙터블 액터든(메시 컴포넌트 이름/개수와 무관하게) 공통으로 동작하도록
	// 인터페이스가 아니라 컴포넌트 순회로 처리한다.
	void SetActorHighlight(AActor* Actor, bool bEnable);

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

	/// outline 용 코드
	// CustomDepth 스텐실 값 — 레벨의 PostProcessVolume에 등록한
	// M_InteractionOutline_PostProcess가 참조하는 스텐실 값과 반드시 일치해야 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Highlight")
	int32 HighlightStencilValue = 1;
	///-----------------------------------------------------------------------------------

private:
	// 현재 타겟
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;
};
