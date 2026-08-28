/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StatInterface.generated.h"

class UStatComponent;

UINTERFACE(MinimalAPI)
class UStatInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IStatInterface
{
	GENERATED_BODY()

public:
	virtual UStatComponent* GetStatComponent() const = 0;
};
