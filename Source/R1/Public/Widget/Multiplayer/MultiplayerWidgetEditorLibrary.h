#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MultiplayerWidgetEditorLibrary.generated.h"

class UWidgetBlueprint;
class UWorld;

/**
* WBP 디자이너 트리를 생성하기 위한 에디터 전용 유틸리티
* - 최초 WBP 생성 또는 전체 재생성에만 사용
* - 게임 실행 중에는 호출되지 않음
* - BP를 디자이너에서 수정한 이후에는 생성 스크립트를 다시 실행하지 말 것
*   -> 다시 실행하면 WBP의 내용을 전부 삭제 후 C++ 코드와 맞게 재생성함 (변경 내용 사라짐)
*/
UCLASS()
class R1_API UMultiplayerWidgetEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	// WBP_MultiplayerMenu에서 BindWidget으로 참조할 전체 디자이너 트리를 재구성
	UFUNCTION(BlueprintCallable, Category = "R1|Editor")
	static bool BuildMultiplayerMenuWidgetTree(UWidgetBlueprint* WidgetBlueprint);

	// 세션 하나를 표시하는 WBP_MultiplayerSessionRow 디자이너 트리를 재구성
	UFUNCTION(BlueprintCallable, Category = "R1|Editor")
	static bool BuildMultiplayerSessionRowWidgetTree(UWidgetBlueprint* WidgetBlueprint);

	// 전달된 메인 메뉴 월드가 멀티플레이 메뉴용 GameMode를 사용하도록 설정
	UFUNCTION(BlueprintCallable, Category = "R1|Editor")
	static bool ConfigureMultiplayerMainMenuWorld(UWorld* World);
#endif
};
