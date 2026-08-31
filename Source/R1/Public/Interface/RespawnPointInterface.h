/// 최초작성 : 2026.08.31
/// 작 성 자 : 강 진 구
/// 리스폰 지점(ex: 침낭) 인터페이스

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RespawnPointInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URespawnPointInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class R1_API IRespawnPointInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 액터 트랜스폼 반환 함수
	UFUNCTION(BlueprintNativeEvent)
	FTransform GetRespawnTransform() const;
};
