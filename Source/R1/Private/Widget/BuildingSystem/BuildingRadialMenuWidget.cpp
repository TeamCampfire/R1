/// 최초작성 : 2026.09.02
/// 작 성 자 : 우 진

#include "Widget/BuildingSystem/BuildingRadialMenuWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Data/Building/BuildingPartDefinition.h"
#include "Widget/BuildingSystem/BuildingRadialEntryWidget.h"
#include "GameFramework/PlayerController.h"
#include "Character/ActionCharacter.h"
#include "Component/InventoryComponent.h"

void UBuildingRadialMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateHoveredPartFromMouse();
}

void UBuildingRadialMenuWidget::InitializeMenu(const TArray<TObjectPtr<UBuildingPartDefinition>>& InAvailableParts,
	UBuildingPartDefinition* InCurrentPart)
{
	AvailableParts = InAvailableParts;

	RebuildEntries(); // Entry 다시 만들기

	if (true == AvailableParts.IsEmpty()) // 선택 가능 파츠가 없는 상태
	{
		HoveredIndex = INDEX_NONE;
		return;
	}

	// IndexOfByPredicate()를 통해 배열을 순회해 현재 파츠를 찾아 인덱스를 반환함
	const int32 CurrentIndex = AvailableParts.IndexOfByPredicate([InCurrentPart](const TObjectPtr<UBuildingPartDefinition>& Part)
	{
		return Part.Get() == InCurrentPart;
	});

	SetHoveredIndex(CurrentIndex != INDEX_NONE ? CurrentIndex : 0); // 현재 호버 인덱스로 세팅
}

UBuildingPartDefinition* UBuildingRadialMenuWidget::GetHoveredPart() const
{
	if (false == AvailableParts.IsValidIndex(HoveredIndex)) return nullptr;
	return AvailableParts[HoveredIndex].Get();
}

void UBuildingRadialMenuWidget::RebuildEntries()
{
	if (false == IsValid(Canvas_PartSlots))
		return;

	// 세팅 초기화
	Canvas_PartSlots->ClearChildren();
	Entries.Reset();

	if (nullptr == EntryWidgetClass || true == AvailableParts.IsEmpty()) return;

	// UMG 레이아웃은 위젯이 생성되자마자 모든 크기를 즉시 계산하지 않는대요 -> Canavas 크기가 아직 0일 수도?
	// ForceLayoutPrepass() 는 이 레이아웃 계산을 미리 실행하도록 요청하는 함수예요
	ForceLayoutPrepass(); 

	FVector2D CanvasSize = Canvas_PartSlots->GetCachedGeometry().GetLocalSize();

	// 한 번 더.. Canvas 크기가 세팅 안 되었을 수도 있는 UI를 위한 세팅입니다
	if (CanvasSize.X <= 0.0f || CanvasSize.Y <= 0.0f)
		CanvasSize = FallbackCanvasSize;

	//! radial 원의 중심을 구하고
	//! 중심점에 방향 벡터와 반지름을 더해 각 아이콘의 위치를 계산해요
	const FVector2D Center = CanvasSize * 0.5f;
	const int32 PartCount = AvailableParts.Num();

	for (int32 Index = 0; Index < PartCount; ++Index)
	{
		// Entry widget 생성
		UBuildingRadialEntryWidget* Entry = CreateWidget<UBuildingRadialEntryWidget>(GetOwningPlayer(),EntryWidgetClass.Get());

		if (false == IsValid(Entry)) continue;

		Entry->InitializeEntry(AvailableParts[Index]); // Entry 세팅

		UCanvasPanelSlot* CanvasSlot = Canvas_PartSlots->AddChildToCanvas(Entry); // 실제 Canvas에 Entry 추가
		if (false == IsValid(CanvasSlot)) continue;

		// 각 Entry의 각도 계산
		// Angle =  시작 방향 보정 + 현재 인덱스 * (Entry 사이의 각도 간격)
		// 시작 방향을 보정하는 이유는 0번째 파츠의 시작 위치는 위쪽인데 삼각함수 좌표계에서는 0도는 오른쪽을 가리키기 떄문에
		// 위쪽을 가리키려면 -90도가 되어야 하고 그렇게 (-PI * 0.5f) 라는 식이 나옴다
		const float Angle =	(-PI * 0.5f) + (static_cast<float>(Index) * (2.0f * PI / PartCount));
		const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle)); // 각도를 방향 벡터로 변환 

		CanvasSlot->SetAutoSize(false); // cpp이 직접 슬롯 크기를 지정할 때 쓰는 세팅
		CanvasSlot->SetSize(EntrySize);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(Center + Direction * EntryRadius); // 최종 위치 계산

		Entries.Add(Entry);
	}
}

void UBuildingRadialMenuWidget::SetHoveredIndex(int32 NewIndex)
{
	if (false == AvailableParts.IsValidIndex(NewIndex) || false == Entries.IsValidIndex(NewIndex)) return;

	if (Entries.IsValidIndex(HoveredIndex)) // 이전 Entry를 미선택 상태로 변경
		Entries[HoveredIndex]->SetSelected(false);

	HoveredIndex = NewIndex;
	Entries[HoveredIndex]->SetSelected(true); // 새로운 Entry를 선택 상태로 변경

	UpdateCenterInfo(AvailableParts[HoveredIndex]); // Radial 중앙에 정보에 데이터를 갱신함
	UpdateSelectionSegment(); // 선택 영역 갱신
}

void UBuildingRadialMenuWidget::UpdateCenterInfo(UBuildingPartDefinition* PartDefinition)
{
	if (false == IsValid(PartDefinition)) return;

	if (true == IsValid(Text_SelectedPartName)) // 파츠 이름 변경
		Text_SelectedPartName->SetText(PartDefinition->DisplayName);

	if (true == IsValid(Text_SelectedPartDesc)) // 파츠 설명 변경
		Text_SelectedPartDesc->SetText(PartDefinition->Description);

	if (true == IsValid(Image_SelectedPartIcon))
	{
		if (false == PartDefinition->Icon.IsNull()) // 파츠 아이콘 변경
			Image_SelectedPartIcon->SetBrushFromSoftTexture(PartDefinition->Icon,false);
		else
			Image_SelectedPartIcon->SetBrushFromTexture(nullptr);
	}

	// 현재 파츠의 필요 자원, 수량 UI 갱신 (이름, 설명, 아이콘)
	UpdateResourceCostInfo(PartDefinition);
}

void UBuildingRadialMenuWidget::UpdateResourceCostInfo(const UBuildingPartDefinition* PartDefinition)
{
	if (false == IsValid(Text_SelectedPartCost)) return;

	if (false == IsValid(PartDefinition))
	{
		Text_SelectedPartCost->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 비용이 설정되지 않은 파츠는 무료 설치이므로 "설치 비용 : 무료"료 표시
	if (PartDefinition->ResourceCosts.Num() == 0)
	{
		Text_SelectedPartCost->SetText(FText::FromString(TEXT("설치 비용 : 무료")));
		Text_SelectedPartCost->SetColorAndOpacity(FSlateColor(SufficientResourceColor));
		Text_SelectedPartCost->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	// 현재 조종중인 캐릭터의 인벤토리를 가져옴
	const AActionCharacter* OwnerCharacter = Cast<AActionCharacter>(GetOwningPlayerPawn());
	const UInventoryComponent* Inventory = IsValid(OwnerCharacter) ? OwnerCharacter->GetInventoryComponent(): nullptr;

	if (false == IsValid(OwnerCharacter) || false == IsValid(Inventory))
		return; 

	TArray<FString> CostLines;
  	bool bHasValidCostData = true;

	// 건축에 필요한 자원들을 순회하면서 UI에 띄울 정보를 CostLines 배열로 관리
	for (const FBuildingResourceCost& ResourceCost : PartDefinition->ResourceCosts)
	{
		if (false == IsValid(ResourceCost.ItemData) || 0 >= ResourceCost.RequiredCount)
		{
			bHasValidCostData = false; // 잘못 세팅된 데이터들은 UI에 보여주지 않음
			continue;
		}

		UItemDataBase* ResourceItem = ResourceCost.ItemData.Get();
		const int32 OwnedCount = IsValid(Inventory) ? Inventory->GetItemCount(ResourceItem): 0;

		if (OwnedCount < ResourceCost.RequiredCount) // 설치 요구 개수보다 보유한 숫자가 작으면 기각
			bHasValidCostData = false;

		// UI에 보여질 문구 : 필요한 개수 X 자원 이름 (현재 보유 개수)
		CostLines.Add(FString::Printf(TEXT("%d X %s (%d)"), ResourceCost.RequiredCount, *ResourceItem->DisplayName.ToString(),OwnedCount));
	}

	if (0 == CostLines.Num())
	{
		CostLines.Add(TEXT("비용 정보 오류")); // 사실 없을 수 없거든요 (디버그 코드임)
		bHasValidCostData = false;
	}

	Text_SelectedPartCost->SetText(FText::FromString(FString::Join(CostLines, TEXT("\n"))));

	// 단일 TextBlock을 사용하므로 자원이 하나라도 부족하면 비용 텍스트 전체를 부족 색상으로 표시
	Text_SelectedPartCost->SetColorAndOpacity(FSlateColor(bHasValidCostData ? SufficientResourceColor : InsufficientResourceColor));
	Text_SelectedPartCost->SetVisibility(ESlateVisibility::Visible);
}

void UBuildingRadialMenuWidget::UpdateSelectionSegment()
{
	if (false == IsValid(Image_SelectedSegment) || true == AvailableParts.IsEmpty() || HoveredIndex == INDEX_NONE) return;

	if (false == IsValid(SelectionMaterial))
		SelectionMaterial = Image_SelectedSegment ->GetDynamicMaterial();

	if (false == IsValid(SelectionMaterial)) return;

	// 파츠 하나가 차지하는 각도 계산 (360 / 8 = 45)
	const float SegmentAngle = 2.0f * PI / static_cast<float>(AvailableParts.Num());

	// 현재 인덱스의 회전 방향 계산
	// 실제 0번째 파츠가 위쪽에 있기 때문에 시작 방향을 위쪽으로 만듦. --> (-PI * 0.5f) 
	// 삼각함수 0은 오른쪽으로 뻗어나가서 -90으로 시작해야 함
	const float Rotation = (-PI * 0.5f) + (static_cast<float>(HoveredIndex) * SegmentAngle);

	// 해당 머티리얼 Custom Node에 더 자세한 과정이 있어요
	// 여긴 그냥 값 전달
	SelectionMaterial->SetScalarParameterValue(TEXT("HalfAngle"), SegmentAngle * 0.5f);
	SelectionMaterial->SetScalarParameterValue(TEXT("Rotation"), Rotation);
}

void UBuildingRadialMenuWidget::UpdateHoveredPartFromMouse()
{
	if (true == AvailableParts.IsEmpty()) return;

	APlayerController* PlayerController = GetOwningPlayer();
	if (false == IsValid(PlayerController)) return;

	float MouseX = 0.0f;
	float MouseY = 0.0f;

	// 현재 마우스 위치, 뷰포트 크기 가져오기
	if (false == PlayerController->GetMousePosition(MouseX, MouseY)) return;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;

	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0) return;

	const FVector2D ViewportCenter(ViewportWidth * 0.5f, ViewportHeight * 0.5f); // 화면 중심 계산
	const FVector2D MousePosition(MouseX, MouseY); // 마우스 좌표를 Vector2D로 변환
	const FVector2D Direction = MousePosition - ViewportCenter; // 중심에서 마우스로 향하는 방향 벡터

	// 중앙 데드존 안에서는 현재 선택 유지
	if (Direction.SizeSquared() < FMath::Square(SelectionDeadZone)) return;

	// 화면 기준: 오른쪽 0도, 아래쪽 +90도, // 왼쪽 +-180도, 위쪽 -90도
	const float MouseAngle = FMath::Atan2(Direction.Y, Direction.X);

	float AngleFromTop = MouseAngle + PI * 0.5f; // 위쪽을 0번 인덱스로 만들기 위해 90도 더함
	AngleFromTop = FMath::Fmod(AngleFromTop + 2.0f * PI, 2.0f * PI); // 각도를 0 ~ 2PI 범위로 정규화

	const float SegmentAngle = 2.0f * PI / static_cast<float>(AvailableParts.Num()); // 파츠 하나의 각도 계산

	// 가장 가까운 부채꼴 중심 계산해서 인덱스 구하기
	const int32 NewHoveredIndex = FMath::FloorToInt((AngleFromTop + SegmentAngle * 0.5f) / SegmentAngle) % AvailableParts.Num();

	if (NewHoveredIndex == HoveredIndex) return;

	SetHoveredIndex(NewHoveredIndex); // 마우스 호버 인덱스로..
}
