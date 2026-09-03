/// 최초작성 : 2026.09.02
/// 작 성 자 : 우 진

#pragma once

#include "CoreMinimal.h"
#include "Item/HeldItemBase.h"
#include "BuildingPlan.generated.h"

/**
 * 
 */
UCLASS()
class R1_API ABuildingPlan : public AHeldItemBase
{
	GENERATED_BODY()

public:
	ABuildingPlan();

public:
	// 도구 장착/해제 오버라이드 함수
	virtual void OnEquipped(AActionCharacter* InCharacter) override;
	virtual void OnUnequipped() override;

	// 우클릭 액션 오버라이드 함수
	// 우클릭 했을 때 건축 파츠 다이얼을 열어 보여줘요
	virtual void OnSecondaryActionStarted() override;
	virtual void OnSecondaryActionCompleted() override;

protected:
	// 인자로 들어온 건축 파츠를 현재 건축 파츠로 세팅하는 함수
	UFUNCTION(BlueprintCallable, Category = "Building Plan|Parts")
	bool SelectBuildingPart(class UBuildingPartDefinition* Definition);

	// Building Radial Menu UI 열/닫 함수
	void OpenRadialMenu();
	void CloseRadialMenu();

	// =========================================================================
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Plan")
	TObjectPtr<class UBuildingPartDefinition> DefaultBuildingPart; // 건축 모드가 실행되었을 때 제일 기본으로 들고 있을 건축 파츠

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Building Plan")
	TObjectPtr<UBuildingPartDefinition> CurrentBuildingPart = nullptr; // 현재 프리뷰로 선택된 건축 파츠

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Plan")
	TArray<TObjectPtr<UBuildingPartDefinition>> AvailableBuildingParts; // 이 건축 도면으로 선택할 수 있는 건축 파츠 목록

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Plan|UI")
	TSubclassOf<class UBuildingRadialMenuWidget> RadialMenuWidgetClass; // BP_BuildingPlan에서 지정할 다이얼 위젯 클래스

	UPROPERTY(Transient)
	TObjectPtr<class UBuildingRadialMenuWidget> RadialMenuWidget; // 실행 중 생성된 다이얼 위젯 인스턴스
};
