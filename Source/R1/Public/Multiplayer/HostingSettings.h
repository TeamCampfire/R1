

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "Engine/World.h"

#include "HostingSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Hosting Settings"))
class R1_API UHostingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public :

	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

public :

	// 방 생성 시 이동할 맵: Project Settings -> Game -> Hosting Settings -> Hosting Map 설정
	UPROPERTY(Config, EditAnywhere, Category="Maps")
	TSoftObjectPtr<UWorld> HostingMap;
	
};
