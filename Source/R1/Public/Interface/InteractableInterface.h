/// 최초작성 : 2026.08.27
/// 작 성 자 : 최 요 환
/// 간단설명 : 조준점(크로스헤어) 대상 상호작용 공통 인터페이스.

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

/**
 * 조준점(크로스헤어) 대상 상호작용 공통 인터페이스.
 *
 * 아이템 픽업(AItemPickup)뿐 아니라 창고, 전리품 상자, 그 밖의 상호작용
 * 액터가 전부 이 인터페이스 하나만 구현하면 "조준하면 이름이 뜨고
 * 단축키를 누르면 상호작용된다"는 흐름을 공유하게 된다. 실제 조준(라인 트레이스) + 입력 처리는
 * 캐릭터 쪽 UInteractionComponent가 전담하고,
 * 이 인터페이스는 각 액터가 "무엇을 보여주고 무엇을 할지"만 정의한다.
 *
 */

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IInteractableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 조준 시 UI에 표시할 이름 (예: "방수포", "창고", "전리품 상자").
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	FText GetInteractionDisplayName() const;

	// 지금 상호작용 가능한 상태인지 (잠긴 상자, 이미 빈 상자 등에서 false를 반환할 수 있게).
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	bool CanInteract(APawn* Interactor) const;

	// 실제 상호작용 처리 — 아이템 픽업이면 인벤토리에 획득, 창고/상자면 UI 오픈 등.
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Interact(APawn* Interactor);

	// 조준 UI에 같이 표시할 아이콘(선택). 창고/전리품 상자처럼 아이콘이 없는
	// 상호작용 액터는 오버라이드하지 않으면 되고, 그 경우 UHT가 자동 생성한
	// 기본 구현이 빈 소프트 포인터를 반환한다 — UI 쪽에서 IsValid 체크로 숨기면 됨.
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	TSoftObjectPtr<UTexture2D> GetInteractionIcon() const;
};
