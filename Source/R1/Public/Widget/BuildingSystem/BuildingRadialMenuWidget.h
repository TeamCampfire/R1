/// 최초작성 : 2026.09.02
/// 작 성 자 : 우 진
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuildingRadialMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class R1_API UBuildingRadialMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// 파츠 목록으로 부터 Entry 생성하는 함수
	void InitializeMenu(const TArray<TObjectPtr<class UBuildingPartDefinition>>& InAvailableParts, UBuildingPartDefinition* InCurrentPart);

	// 마우스 호버중인 파츠를 가져오는
	UBuildingPartDefinition* GetHoveredPart() const;

protected:
	// 메뉴 초기화 할 때 Entry도 다시 세팅해줘요
	void RebuildEntries();

	// 마우스 호버중인 파츠를 세팅
	void SetHoveredIndex(int32 NewIndex);

	// Radial 중간에 있는 정보에 현재 파츠의 데이터를 세팅해줘요
	void UpdateCenterInfo(UBuildingPartDefinition* PartDefinition);

	// 현재 파츠 Radial영역을 강조하기 위한 머티리얼 작업을 여기서 해줘요
	void UpdateSelectionSegment();

	// 마우스 위치를 통해 결정되는 파츠를 계산하는 함수
	void UpdateHoveredPartFromMouse();

	// =========================================================================
protected:
	// UI 바인딩 변수들
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Radial Menu")
	TObjectPtr<class UCanvasPanel> Canvas_PartSlots;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Radial Menu")
	TObjectPtr<class UImage> Image_SelectedSegment;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Radial Menu")
	TObjectPtr<UImage> Image_SelectedPartIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget),Category = "Building Radial Menu")
	TObjectPtr<class UTextBlock> Text_SelectedPartName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Radial Menu")
	TObjectPtr<UTextBlock> Text_SelectedPartDesc;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Radial Menu")
	TObjectPtr<UTextBlock> Text_SelectedPartCost;

	// 기능
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Radial Menu|Entry")
	TSubclassOf<class UBuildingRadialEntryWidget> EntryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Radial Menu|Layout")
	FVector2D EntrySize = FVector2D(56.0f, 56.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Radial Menu|Layout")
	float EntryRadius = 245.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Radial Menu|Layout")
	FVector2D FallbackCanvasSize = FVector2D(620.0f, 620.0f);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuildingPartDefinition>> AvailableParts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuildingRadialEntryWidget>> Entries;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SelectionMaterial;

	int32 HoveredIndex = INDEX_NONE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Radial Menu|Input", meta = (ClampMin = "0.0"))
	float SelectionDeadZone = 70.0f; // 마우스가 중심에서 이 거리 안에 있으면 기존 선택을 유지
};
