
#include "Widget/DeathScreenOverlayWidget.h"
#include "Components/Button.h"

void UDeathScreenOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RespawnButton)
	{
		RespawnButton->OnClicked.AddDynamic(
			this,
			&UDeathScreenOverlayWidget::OnRespawnButtonClicked
		);
	}
}

void UDeathScreenOverlayWidget::OnRespawnButtonClicked()
{
	OnRespawnClicked.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("SpawnRequested"));
}
