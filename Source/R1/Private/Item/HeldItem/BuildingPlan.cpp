/// 최초작성 : 2026.09.02
/// 작 성 자 : 우 진

#include "Item/HeldItem/BuildingPlan.h"

#include "Character/ActionCharacter.h"
#include "Character/ActionPlayerController.h"

#include "Data/Building/BuildingPartDefinition.h"
#include "Widget/BuildingSystem/BuildingRadialMenuWidget.h"

ABuildingPlan::ABuildingPlan()
{
}

void ABuildingPlan::OnEquipped(AActionCharacter* InCharacter)
{
	Super::OnEquipped(InCharacter);

	CurrentBuildingPart = nullptr;

	// 건축모드 시작
	if (false == SelectBuildingPart(DefaultBuildingPart))
	{
		UE_LOG(LogTemp, Log, TEXT("BuildingPlan: 기본 건축 파츠 선택에 실패했습니다."));
	}
}

void ABuildingPlan::OnUnequipped()
{
	CloseRadialMenu(); // RadialMenu 닫아요

	if (true == IsValid(OwnerCharacter))
	{
		// 건축모드 끝
		if (AActionPlayerController* PlayerController = Cast<AActionPlayerController>(OwnerCharacter->GetController()))
			PlayerController->OnStopPlacement();

		CurrentBuildingPart = nullptr;
	}

	Super::OnUnequipped();
}

void ABuildingPlan::OnSecondaryActionStarted()
{
	Super::OnSecondaryActionStarted();

	// 우클릭 시작할 때 Radial UI 열어요
	OpenRadialMenu();
}

void ABuildingPlan::OnSecondaryActionCompleted()
{
	Super::OnSecondaryActionCompleted();

	if (true == IsValid(RadialMenuWidget) && true == RadialMenuWidget->IsInViewport()) // RadialMenuWidget이 유효하고 실제로 열려있었는지 검사해요
	{
		// 현재 마우스 호버중인 건축 파츠를 가져와 해당 건축 파츠를 선택한걸로 선택(?)
		UBuildingPartDefinition* HoveredPart = RadialMenuWidget->GetHoveredPart();

		if (true == IsValid(HoveredPart) && HoveredPart != CurrentBuildingPart)
			SelectBuildingPart(HoveredPart); // 여기서 현재 건축 파츠를 진짜로 갈아끼워요
	}

	// 마우스 오른쪽에서 손을 떼면 Radial UI도 닫혀요
	CloseRadialMenu();
}

bool ABuildingPlan::SelectBuildingPart(UBuildingPartDefinition* Definition)
{
	if (false == IsValid(OwnerCharacter))
	{
		UE_LOG(LogTemp, Log, TEXT("Item [BuildingPlan] : 장착 중인 캐릭터가 없습니다."));
		return false;
	}

	if (false == IsValid(Definition))
	{
		UE_LOG(LogTemp, Log, TEXT("Item [BuildingPlan] : 선택할 건축 파츠가 유효하지 않습니다."));
		return false;
	}

	// ContainsByPredicate()를 이용
	// AvailableBuildingParts를 순회해 인자로 들어온 건축 파츠와 동일한 건축 파츠를 찾는 순간 return
	const bool bIsAvailable = AvailableBuildingParts.ContainsByPredicate([Definition](const TObjectPtr<UBuildingPartDefinition>& Part)
	{
		return Part.Get() == Definition;
	});

	if (false == bIsAvailable)
	{
		UE_LOG(LogTemp, Log, TEXT("Item [BuildingPlan] : 선택 가능한 목록에 없는 건축 파츠입니다. Part=%s"), *GetNameSafe(Definition));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("여기까지 들어오나요?"));

	AActionPlayerController* PlayerController = Cast<AActionPlayerController>(OwnerCharacter->GetController());
	if (false == IsValid(PlayerController))
	{
		UE_LOG(LogTemp, Log, TEXT("Item [BuildingPlan] : ActionPlayerController를 찾지 못했습니다."));
		return false;
	}

	// 건축 파츠 갈아끼우기
	PlayerController->OnStartPlacement(Definition);
	CurrentBuildingPart = Definition;
	UE_LOG(LogTemp, Log, TEXT("Item [BuildingPlan] : 건축 파츠 선택 완료. Part=%s"), *GetNameSafe(CurrentBuildingPart));

	return true;

}

void ABuildingPlan::OpenRadialMenu()
{
	//  Radial이 유효한데 && 심지어 뷰포트에 붙어있으면 -> 이미 열린 상태
	if (true == IsValid(RadialMenuWidget) && true == RadialMenuWidget->IsInViewport()) return;

	if (false == IsValid(OwnerCharacter))
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildingPlan : OwnerCharacter가 없습니다."));
		return;
	}

	if (nullptr == RadialMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildingPlan : RadialMenuWidgetClass가 설정되지 않았습니다."));
		return;
	}

	AActionPlayerController* PlayerController = Cast<AActionPlayerController>(OwnerCharacter->GetController());
	if (false == IsValid(PlayerController))
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildingPlan : ActionPlayerController를 찾지 못했습니다."));
		return;
	}

	// RadialMenuWidget이 첫 오픈이라 인스턴스가 없었다면 하나 생성해요
	if (false == IsValid(RadialMenuWidget))
		RadialMenuWidget = CreateWidget<UBuildingRadialMenuWidget>(PlayerController, RadialMenuWidgetClass);

	// 그래도 없다? 문제 있는 거예요 이거
	if (false == IsValid(RadialMenuWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildingPlan: 다이얼 위젯 생성에 실패했습니다."));
		return;
	}

	// 위젯 부착!
	RadialMenuWidget->AddToViewport(100); // 기본 HUD 같은 것들 보다 앞에 그려졌음 해서 Z Order를 높여놨어요
	RadialMenuWidget->InitializeMenu(AvailableBuildingParts, CurrentBuildingPart); // 계속 메뉴를 최신으로 

	PlayerController->SetIgnoreLookInput(true); // 마우스 회전 막음
	//PlayerController->bShowMouseCursor = true; // 마우스도 보여요

	// GameAndUI 를 이용한 이유는 두 입력이 모두 필요해서 입니다..
	// UI는 마우스 위치를 사용해야 하고,
	// Game에서는 우클릭 해제 InputAction 작업에 관여하니까...
	FInputModeGameAndUI InputMode;

	// 커서를 게임 화면 영역으로 가두는 세팅입니다 (다이얼 선택 중에 커서가 게임 창 밖으로 빠져나가는 걸 막기 위함)
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);

	// 다이얼 시작할 때 마우스가 이상한 위치에 있길 윈하지 않아서
	// 마우스를 화면 정중앙으로 옮겨요
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

	if ((ViewportWidth > 0) && (ViewportHeight > 0))
		PlayerController->SetMouseLocation(ViewportWidth / 2, ViewportHeight / 2);
}

void ABuildingPlan::CloseRadialMenu()
{
	// 실제로 뷰포트에 붙어 있으면 이제 뷰포트에서 떼어내줘요
	// RemoveFromParent()는 화면에서만 떼어냄
	if (true == IsValid(RadialMenuWidget) && true == RadialMenuWidget->IsInViewport())
		RadialMenuWidget->RemoveFromParent();

	if (false == IsValid(OwnerCharacter)) return;

	AActionPlayerController* PlayerController = Cast<AActionPlayerController>(OwnerCharacter->GetController());

	if (false == IsValid(PlayerController)) return;

	PlayerController->SetIgnoreLookInput(false); // 카메라 회전 락 걸어둔 거 복구
	//PlayerController->bShowMouseCursor = false;
	
	//TODO 플레이어 컨트롤러 패널 몇 개 열렸는지 확인 후 InputMode 세팅하는 걸로 변경
	PlayerController->SetInputMode(FInputModeGameOnly()); // 게임 전용모드로 되돌리기
}
