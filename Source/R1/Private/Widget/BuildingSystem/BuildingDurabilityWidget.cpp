// 작업 시작일 : 9/5
// 작업자 : 우진

#include "Widget/BuildingSystem/BuildingDurabilityWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBuildingDurabilityWidget::UpdateDurability(float CurrentDurability, float MaxDurability)
{
	// 최대 내구도가 0이면 나눗셈을 하지 않고 빈 게이지로 표시
	const float DurabilityPercent = MaxDurability > 0.f ? CurrentDurability / MaxDurability : 0.f;

	if (true == IsValid(ProgressBar_BuildingDurability))
		ProgressBar_BuildingDurability->SetPercent(DurabilityPercent);

	if (true == IsValid(Text_BuildingDurability))
	{
		const int32 DisplayCurrent = FMath::RoundToInt(CurrentDurability);
		const int32 DisplayMax = FMath::RoundToInt(MaxDurability);

		Text_BuildingDurability->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), DisplayCurrent, DisplayMax)));
	}
}
