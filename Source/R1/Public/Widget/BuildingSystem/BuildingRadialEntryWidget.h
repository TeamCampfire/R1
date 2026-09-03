/// 최초작성 : 2026.09.02
/// 작 성 자 : 우 진

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuildingRadialEntryWidget.generated.h"

/**
Radial Menu에서 파츠 하나의 아이콘을 담당하는 위젯
 */
UCLASS()
class R1_API UBuildingRadialEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 Entry가 표시할 건축 파츠를 설정
	void InitializeEntry(class UBuildingPartDefinition* InPartDefinition);

	// 현재 선택 여부에 따라 아이콘 색상 변경
	void SetSelected(bool bSelected);

	UFUNCTION(BlueprintPure, Category = "Building Radial Entry")
	UBuildingPartDefinition* GetPartDefinition() const;

	// =========================================================================
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Radial Entry")
	TObjectPtr<class UImage> Image_PartIcon; // 이미지 위젯

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Radial Entry|Style")
	FLinearColor UnselectedColor = FLinearColor(0.8f, 0.08f, 0.04f, 1.0f); // 선택되지 않은 아이콘 색상

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Radial Entry|Style")
	FLinearColor SelectedColor = FLinearColor::White; // 선택된 아이콘 색상

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Building Radial Entry")
	TObjectPtr<UBuildingPartDefinition> PartDefinition; // 현재 파츠 정보
};
