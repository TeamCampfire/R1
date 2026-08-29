/// 최초작성 : 2026.08.25
/// 작 성 자 : 최 요 환

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ActionPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
/**
 * 
 */
UCLASS()
class R1_API AActionPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AActionPlayerController();

protected:
	virtual void BeginPlay() override;

public:
	// 플레이어가 건축 좌키 눌렀을 때 컨트롤러로 넘어온 함수
	void OnConfirmBuildingPlacement();

	//  ===================================================================================
protected:
	// 기본 입력 맵핑 컨텍스트(캐릭터 조작)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UBuildingPlacementComponent> BuildingPlacementComponent; // 건축물 설치 컴포넌트

private:
	// 입력 우선 순위
	int32 GameInputPriority = 1;
};
