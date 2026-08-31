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
public:
	// 인벤토리 패널이 열리면 마우스 커서를 보여주고 UI 입력을 받도록, 닫히면 다시 게임 전용 입력으로 되돌린다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventoryInputState(bool bOpen);

protected:
	// 기본 입력 맵핑 컨텍스트(캐릭터 조작) — 인벤토리가 열려있는 동안엔 제거된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UBuildingPlacementComponent> BuildingPlacementComponent; // 건축물 설치 컴포넌트
	// UI 입력 맵핑 컨텍스트(인벤토리 토글 등) — BeginPlay에 한 번 추가되면 제거되지 않는다.
	// DefaultMappingContext를 뺐다 켰다 해도 이 토글 키만은 항상 눌리게 하기 위함.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> UIMappingContext = nullptr;

private:
	// 입력 우선 순위
	int32 GameInputPriority = 1;
	int32 UIInputPriority = 2;
};
