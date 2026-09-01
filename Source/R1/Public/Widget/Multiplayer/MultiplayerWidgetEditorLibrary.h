#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MultiplayerWidgetEditorLibrary.generated.h"

class UWidgetBlueprint;
class UWorld;

/** WBP 디자이너 트리를 생성하기 위한 에디터 전용 유틸리티. */
UCLASS()
class R1_API UMultiplayerWidgetEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = "R1|Editor")
	static bool BuildMultiplayerMenuWidgetTree(UWidgetBlueprint* WidgetBlueprint);

	UFUNCTION(BlueprintCallable, Category = "R1|Editor")
	static bool ConfigureMultiplayerMainMenuWorld(UWorld* World);
#endif
};
