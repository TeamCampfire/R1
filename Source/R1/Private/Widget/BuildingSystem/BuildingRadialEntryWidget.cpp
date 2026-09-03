/// 최초작성 : 2026.09.02
/// 작 성 자 : 우 진

#include "Widget/BuildingSystem/BuildingRadialEntryWidget.h"
#include "Components/Image.h"

#include "Data/Building/BuildingPartDefinition.h"

void UBuildingRadialEntryWidget::InitializeEntry(UBuildingPartDefinition* InPartDefinition)
{
	PartDefinition = InPartDefinition;

	if (false == IsValid(PartDefinition) || false == IsValid(Image_PartIcon)) return;

	if (false == PartDefinition->Icon.IsNull())
	{
		// 데이터의 아이콘을 Image에 적용
		Image_PartIcon->SetBrushFromSoftTexture(PartDefinition->Icon, false);
	}

	SetSelected(false); // 아이콘 색상 세팅
}

void UBuildingRadialEntryWidget::SetSelected(bool bSelected)
{
	if (false == IsValid(Image_PartIcon)) return;
	Image_PartIcon->SetColorAndOpacity(bSelected ? SelectedColor : UnselectedColor);
}

UBuildingPartDefinition* UBuildingRadialEntryWidget::GetPartDefinition() const
{
	return PartDefinition;
}
