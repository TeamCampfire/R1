#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingWidget.generated.h"

class UCraftingComponent;
class UCraftingItemWidget;
class UCraftingMaterialWidget;
class UItemDataBase;
class AWorkbench;
class UWrapBox;
class UVerticalBox;
class UEditableTextBox;
class UButton;
class UTextBlock;
class UImage;

// HUD에 바인딩된 제작창을 Q와 작업대 상호작용에서 재사용하고 데이터 원본만 교체
// 제작 화면의 검색/수량/재료/큐 표시를 조정
// 개별 타일과 재료 행은 분리된 위젯으로 구성
UCLASS()
class R1_API UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 개인 제작 컴포넌트와 선택한 작업대를 연결하고 화면 상태를 새로 초기화
	// InBench가 nullptr이면 개인 제작, 유효하면 작업대 제작 모드로 동작
	void BindCrafting(UCraftingComponent* InCrafting, AWorkbench* InBench);

	// 창을 숨길 때 화면의 참조만 해제
	// 실제 제작 큐는 컴포넌트에 유지
	void UnbindCrafting();

protected:

	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:

	// 검색어 갱신
	// 조건에 맞는 레시피 목록을 다시 만듦
	UFUNCTION()
	void HandleSearchChanged(const FText& Text);

	// 선택한 레시피의 아이콘, 설명, 제작 시간과 재료 정보 표시
	UFUNCTION()
	void HandleRecipeClicked(UItemDataBase* Item);

	/* 제작 요청 수량 조절 버튼 처리 */
	UFUNCTION()
	void HandleDecrease();
	UFUNCTION()
	void HandleIncrease();
	UFUNCTION()
	void HandleMaximum();

	// 선택한 레시피를 현재 수량만큼 서버 제작 큐에 등록
	UFUNCTION()
	void HandleCraft();

	// 작업대의 회수 대기 목록에 있는 완성품을 서버에 요청해 회수
	UFUNCTION()
	void HandleCollect();

	// 소유 컨트롤러를 통해 제작창 닫기
	UFUNCTION()
	void HandleClose();

	// 제작 취소 처리
	UFUNCTION()
	void HandleCancelOrder(FGuid OrderId);

	// 검색 비교가 공백과 대소문자의 영향을 받지 않도록 문자열 정규화
	static FString NormalizeSearchText(const FString& Text);

	// 공통 카탈로그에서 현재 모드와 검색어에 맞는 레시피 타일을 다시 구성
	void RebuildRecipes();

	// 제작 가능 수량, 버튼 상태, 선택 레시피의 재료와 큐 표시 갱신
	void Refresh();

	// 진행 중인 주문과 작업대의 회수 대기 주문을 각각 타일로 표시
	void RefreshQueue();

	// 개인 제작에서는 플레이어 컴포넌트,
	// 작업대 제작에서는 작업대 컴포넌트를 반환
	UCraftingComponent* GetQueueSource() const;

	// 현재 선택한 레시피를 보유 재료와 제작 조건으로
	// 만들 수 있는 최대 수량 반환
	int32 GetMaximum() const;

protected :

	// 레시피 목록과 제작 큐에서 공통으로 사용하는 아이템 타일 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Crafting")
	TSubclassOf<UCraftingItemWidget> ItemWidgetClass;

	// 선택 레시피의 필요 재료 한 줄을 표시하는 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Crafting")
	TSubclassOf<UCraftingMaterialWidget> MaterialWidgetClass;

	/* 아래 위젯들은 WBP_Crafting에서 바인딩 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWrapBox> RecipeContainer;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> SearchBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWrapBox> QueueContainer;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWrapBox> CompletedContainer;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> MaterialContainer;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> SelectedIcon;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SelectedNameText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DurationText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> QuantityText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> DecreaseButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> IncreaseButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> MaximumButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CraftButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CollectButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CloseButton;

private :

	UPROPERTY()
	TObjectPtr<UCraftingComponent> Crafting;	// 서버 제작 요청을 보내는 플레이어 소유 제작 컴포넌트

	UPROPERTY()
	TObjectPtr<UItemDataBase> Selected;			// 현재 상세 정보와 제작 수량을 표시 중인 아이템 레시피

	// 작업대 제작 모드의 대상
	// 작업대가 제거되거나 범위를 벗어나면 제작창 닫기
	TWeakObjectPtr<AWorkbench> BoundBench;

	bool bWorkbenchMode = false;	// 참이면 작업대 큐와 회수 대기 목록을 표시

	FString SearchText;				// 공백 제거와 소문자 변환이 적용된 현재 검색어

	int32 CraftQuantity = 0;		// 현재 선택한 레시피에서 한 번에 요청할 제작 수량

	float RefreshElapsed = 0.f;		// 재료와 큐 표시를 0.2초 간격으로 갱신하기 위한 누적 시간
};
