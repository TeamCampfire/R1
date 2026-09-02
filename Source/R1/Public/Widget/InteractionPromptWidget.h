/// 최초작성 : 2026.08.28
/// 작 성 자 : 최 요 환
/// 간단설명 : 조준선에 잡힌 IInteractable 대상의 이름/ 아이콘을 보여주는 위젯의 C++ 베이스

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionPromptWidget.generated.h"

class UInteractionComponent;
class UTextBlock;
class UImage;
class UWidget;

/**
 * 조준선(크로스헤어)에 잡힌 IInteractable 대상의 이름/아이콘을 보여주는 위젯의
 * C++ 베이스. 실제 레이아웃/디자인(크로스헤어 점 이미지, 테두리, 폰트 등)은
 * 이 클래스를 부모로 하는 WBP(예: WBP_InteractionPrompt)에서 만든다 — 여기서는
 * "이 이름의 위젯이 있으면 이렇게 채운다"는 로직만 C++이 담당한다.
 *
 * WBP에서 아래 세 위젯을 정확히 이 이름 + 타입으로 배치하면 자동 바인딩된다
 * (BindWidgetOptional이라 없어도 컴파일은 되지만, 없으면 해당 기능만 빠진다):
 * - TargetPanel   : 이름/아이콘을 감싸는 컨테이너(테두리 등). 대상이 없으면 Collapsed.
 * - TargetNameText: 대상 이름 텍스트.
 * - TargetIcon    : 대상 아이콘 이미지. 아이콘이 없는 대상(창고 등)은 자동으로 숨겨진다.
 *
 */
UCLASS()
class R1_API UInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

private:
	UFUNCTION()
	void HandleInteractionTargetChanged(AActor* NewTarget, const FText& DisplayName, UTexture2D* Icon);

	// 상호작용 컴포넌트 바인딩(최초 1회 + 부활 등으로 폰이 바뀔 때마다) — 옛 컴포넌트 델리게이트
	// 해제 후 새 폰의 컴포넌트를 다시 찾아 구독한다. AActionPlayerController::OnPossessedCharChange에
	// 구독해서 부활 시에도 다시 호출되게 한다.
	UFUNCTION()
	void RebindInteraction();

	// RebindInteraction/NativeDestruct 양쪽에서 공유하는 델리게이트 해제 로직.
	void UnbindInteractionDelegates();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TargetPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TargetNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> TargetIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CrossHair;

private:
	// 언바인딩용으로 보관. 소유 폰이 사라지는 경우도 있어 약한 참조로 들고 있는다.
	TWeakObjectPtr<UInteractionComponent> BoundInteractionComponent;
};
