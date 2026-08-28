/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatusBarWidget.generated.h"

class UTextBlock;
enum class EStatusEffect : uint8;

/**
 * 
 */
UCLASS()
class R1_API UStatusBarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitializeStatusEffectWidget(const EStatusEffect inEStatusEffect);


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusEffectText;

};
