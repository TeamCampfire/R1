#include "Widget/Multiplayer/MultiplayerWidgetEditorLibrary.h"

#if WITH_EDITOR

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Framework/MultiplayerMenuGameMode.h"
#include "Styling/CoreStyle.h"
#include "WidgetBlueprint.h"
#include "GameFramework/WorldSettings.h"
#include "Widget/Multiplayer/MultiplayerMenuWidget.h"

namespace
{
	// 반복되는 텍스트 생성, 기본 색상 적용, 부모 패널 추가를 묶은 에디터 전용 헬퍼
	UTextBlock* AddText(UWidgetTree* Tree, UPanelWidget* Parent, const FName Name, const TCHAR* Value)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(FText::FromString(Value));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.80f, 0.75f, 1.f)));
		Parent->AddChild(Text);
		return Text;
	}

	UButton* AddButton(UWidgetTree* Tree, UPanelWidget* Parent, const FName Name, const TCHAR* Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->bIsVariable = true;
		Parent->AddChild(Button);
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("%sLabel"), *Name.ToString()));
		Text->SetText(FText::FromString(Label));
		Text->SetJustification(ETextJustify::Center);
		Button->AddChild(Text);
		return Button;
	}
}

bool UMultiplayerWidgetEditorLibrary::BuildMultiplayerMenuWidgetTree(UWidgetBlueprint* WidgetBlueprint)
{
	if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
	{
		return false;
	}

	WidgetBlueprint->Modify();
	UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
	Tree->Modify();

	// 위젯 루트
	UBorder* Root = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RustMenuBackground"));
	Root->SetBrushColor(FLinearColor(0.018f, 0.02f, 0.019f, 0.98f));
	Root->SetPadding(FMargin(34.f, 28.f));
	Tree->RootWidget = Root;

	// 위젯 헤더
	UVerticalBox* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Page"));
	Root->AddChild(Page);
	UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
	Page->AddChildToVerticalBox(Header)->SetPadding(FMargin(0, 0, 0, 16));
	UTextBlock* Title = AddText(Tree, Header, TEXT("TitleText"), TEXT("PLAY  /  SERVERS"));
	Title->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 28));
	CastChecked<UHorizontalBoxSlot>(Title->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UTextBlock* Status = AddText(Tree, Header, TEXT("StatusText"), TEXT("Ready"));
	Status->bIsVariable = true;

	// 위젯 바디
	UHorizontalBox* Body = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Body"));
	Page->AddChildToVerticalBox(Body)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// 좌측 호스트 패널
	UBorder* HostPanel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HostPanel"));
	HostPanel->SetBrushColor(FLinearColor(0.055f, 0.058f, 0.055f, 1.f));
	HostPanel->SetPadding(FMargin(20.f));
	Body->AddChildToHorizontalBox(HostPanel)->SetPadding(FMargin(0, 0, 18, 0));
	UVerticalBox* HostBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HostBox"));
	HostPanel->AddChild(HostBox);
	AddText(Tree, HostBox, TEXT("HostTitleText"), TEXT("HOST SERVER"));
	AddText(Tree, HostBox, TEXT("ServerNameLabel"), TEXT("SERVER NAME"));
	UEditableTextBox* ServerName = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ServerNameInput"));
	ServerName->bIsVariable = true;
	ServerName->SetText(FText::FromString(TEXT("R1 Server")));
	ServerName->SetMinDesiredWidth(270.f);
	HostBox->AddChild(ServerName);
	AddText(Tree, HostBox, TEXT("MaxPlayersLabel"), TEXT("MAX PLAYERS"));
	UHorizontalBox* MaxPlayersControls = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MaxPlayersControls"));
	HostBox->AddChild(MaxPlayersControls);
	AddButton(Tree, MaxPlayersControls, TEXT("DecreaseMaxPlayersButton"), TEXT("-"));
	const FString DefaultMaxPlayersText = FString::FromInt(UMultiplayerMenuWidget::DefaultMaxPlayers);
	UTextBlock* MaxPlayersText = AddText(Tree, MaxPlayersControls, TEXT("MaxPlayersText"), *DefaultMaxPlayersText);
	MaxPlayersText->bIsVariable = true;
	MaxPlayersText->SetJustification(ETextJustify::Center);
	CastChecked<UHorizontalBoxSlot>(MaxPlayersText->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AddButton(Tree, MaxPlayersControls, TEXT("IncreaseMaxPlayersButton"), TEXT("+"));
	UButton* HostButton = AddButton(Tree, HostBox, TEXT("HostButton"), TEXT("HOST SERVER"));
	HostButton->SetBackgroundColor(FLinearColor(0.82f, 0.58f, 0.22f, 1.f));

	// 검색 도구
	UVerticalBox* Browser = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ServerBrowser"));
	Body->AddChildToHorizontalBox(Browser)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UHorizontalBox* Toolbar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Toolbar"));
	Browser->AddChild(Toolbar);
	UTextBlock* BrowserTitle = AddText(Tree, Toolbar, TEXT("BrowserTitleText"), TEXT("OFFICIAL SERVERS"));
	CastChecked<UHorizontalBoxSlot>(BrowserTitle->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AddButton(Tree, Toolbar, TEXT("RefreshButton"), TEXT("REFRESH"));
	UButton* JoinButton = AddButton(Tree, Toolbar, TEXT("JoinButton"), TEXT("JOIN SERVER"));
	JoinButton->SetBackgroundColor(FLinearColor(0.49f, 0.60f, 0.27f, 1.f));

	// 열 머리글
	UHorizontalBox* Columns = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ColumnHeader"));
	Browser->AddChild(Columns);
	UTextBlock* ServerColumn = AddText(Tree, Columns, TEXT("ServerColumnText"), TEXT("SERVER NAME"));
	CastChecked<UHorizontalBoxSlot>(ServerColumn->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AddText(Tree, Columns, TEXT("PlayersColumnText"), TEXT("PLAYERS"));
	AddText(Tree, Columns, TEXT("PingColumnText"), TEXT("PING"));

	// 세션 목록(런타임 생성 행)이 들어갈 스크롤 목록
	UScrollBox* SessionList = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SessionList"));
	SessionList->bIsVariable = true;
	Browser->AddChildToVerticalBox(SessionList)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// 새 트리가 에디터와 컴파일 결과에 즉시 반영되도록 구조 변경으로 표시한다.
	WidgetBlueprint->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	return true;
}

bool UMultiplayerWidgetEditorLibrary::BuildMultiplayerSessionRowWidgetTree(UWidgetBlueprint* WidgetBlueprint)
{
	if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
	{
		return false;
	}

	WidgetBlueprint->Modify();
	UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
	Tree->Modify();

	// 각 검색 결과는 전체 행 버튼 하나와 서버명/인원/핑 텍스트를 가짐
	UButton* RowButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RowButton"));
	RowButton->bIsVariable = true;
	RowButton->SetBackgroundColor(FLinearColor(0.075f, 0.078f, 0.074f, 1.f));
	Tree->RootWidget = RowButton;

	// 행 버튼화
	UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RowContent"));
	RowButton->AddChild(Row);

	// 방 이름 정보
	UTextBlock* ServerName = AddText(Tree, Row, TEXT("ServerNameText"), TEXT("Server Name"));
	ServerName->bIsVariable = true;
	ServerName->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 16));
	CastChecked<UHorizontalBoxSlot>(ServerName->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// 세션 참가자 수 정보
	UTextBlock* Players = AddText(Tree, Row, TEXT("PlayersText"), TEXT("0 / 4"));
	Players->bIsVariable = true;
	Players->SetMinDesiredWidth(110.f);

	// 세션 핑 정보
	UTextBlock* Ping = AddText(Tree, Row, TEXT("PingText"), TEXT("0 ms"));
	Ping->bIsVariable = true;
	Ping->SetMinDesiredWidth(80.f);
	Ping->SetColorAndOpacity(FSlateColor(FLinearColor(0.49f, 0.60f, 0.27f, 1.f)));

	WidgetBlueprint->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	return true;
}

bool UMultiplayerWidgetEditorLibrary::ConfigureMultiplayerMainMenuWorld(UWorld* World)
{
	AWorldSettings* WorldSettings = World ? World->GetWorldSettings() : nullptr;
	if (!WorldSettings)
	{
		return false;
	}

	return true;
}

#endif
