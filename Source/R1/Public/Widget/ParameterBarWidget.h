/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParameterBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UTexture2D;
/**
 * 
 */
UCLASS()
class R1_API UParameterBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void UpdateParameterBar(float inCurrent, float inMax);

protected:
	virtual void NativePreConstruct() override;

	void SetTargetPercent(float inPercent);

#if WITH_EDITOR
	// #if ~ #endif 사이의 코드는 에디터 상에서만 존재한다.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& inPropertyChangedEvent) override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CurrentText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> IconTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor FillColor = FLinearColor::White;

	FTimerHandle ParameterBarTimerHandle;

private:
	float CurrentPercent = 1.0f;
	float TargetPercent = 0.0f;
	float InterpSpeed = 5.0f;
};
