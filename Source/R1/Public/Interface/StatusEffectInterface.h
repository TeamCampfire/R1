/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StatusEffectInterface.generated.h"

UENUM(BlueprintType, meta = (Bitflags))
enum class EStatusEffect : uint8
{
	None		= 0,
	Cold		= 1 << 0,
	FoodPoison	= 1	<< 1,
	Thirsty		= 1 << 2,
	Hungry		= 1 << 3,
	Dehydrated	= 1 << 4,
	Starving	= 1 << 5,
	Overheat	= 1 << 6,
	Frostbite	= 1 << 7,
	//Tetanus		= 1 << 8
};
ENUM_CLASS_FLAGS(EStatusEffect)

UINTERFACE(MinimalAPI)
class UStatusEffectInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IStatusEffectInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	EStatusEffect GetCurrentStatusEffect();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void RemoveStatusEffect(EStatusEffect inStatusEffectType);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Status Effect")
	void SetStatusEffect(EStatusEffect inStatusEffectType);
};
