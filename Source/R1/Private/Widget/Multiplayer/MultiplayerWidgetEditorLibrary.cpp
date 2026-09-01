#include "Widget/Multiplayer/MultiplayerWidgetEditorLibrary.h"

#if WITH_EDITOR

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Framework/MultiplayerMenuGameMode.h"
#include "Styling/CoreStyle.h"
#include "WidgetBlueprint.h"
#include "GameFramework/WorldSettings.h"

namespace
{
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

	UBorder* Root = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RustMenuBackground"));
	Root->SetBrushColor(FLinearColor(0.018f, 0.020f, 0.019f, 0.98f));
	Root->SetPadding(FMargin(34.f, 28.f));
	Tree->RootWidget = Root;

	UVerticalBox* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Page"));
	Root->AddChild(Page);
	UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
	Page->AddChildToVerticalBox(Header)->SetPadding(FMargin(0, 0, 0, 16));
	UTextBlock* Title = AddText(Tree, Header, TEXT("TitleText"), TEXT("PLAY  /  SERVERS"));
	Title->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 28));
	CastChecked<UHorizontalBoxSlot>(Title->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UTextBlock* Status = AddText(Tree, Header, TEXT("StatusText"), TEXT("Ready"));
	Status->bIsVariable = true;

	UHorizontalBox* Body = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Body"));
	Page->AddChildToVerticalBox(Body)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
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
	USpinBox* MaxPlayers = Tree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("MaxPlayersInput"));
	MaxPlayers->bIsVariable = true;
	MaxPlayers->SetMinValue(1.f);
	MaxPlayers->SetMaxValue(64.f);
	MaxPlayers->SetValue(8.f);
	HostBox->AddChild(MaxPlayers);
	UButton* HostButton = AddButton(Tree, HostBox, TEXT("HostButton"), TEXT("HOST SERVER"));
	HostButton->SetBackgroundColor(FLinearColor(0.82f, 0.58f, 0.22f, 1.f));

	UVerticalBox* Browser = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ServerBrowser"));
	Body->AddChildToHorizontalBox(Browser)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UHorizontalBox* Toolbar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Toolbar"));
	Browser->AddChild(Toolbar);
	UTextBlock* BrowserTitle = AddText(Tree, Toolbar, TEXT("BrowserTitleText"), TEXT("OFFICIAL SERVERS"));
	CastChecked<UHorizontalBoxSlot>(BrowserTitle->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AddButton(Tree, Toolbar, TEXT("RefreshButton"), TEXT("REFRESH"));
	UButton* JoinButton = AddButton(Tree, Toolbar, TEXT("JoinButton"), TEXT("JOIN SERVER"));
	JoinButton->SetBackgroundColor(FLinearColor(0.49f, 0.60f, 0.27f, 1.f));

	UHorizontalBox* Columns = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ColumnHeader"));
	Browser->AddChild(Columns);
	UTextBlock* ServerColumn = AddText(Tree, Columns, TEXT("ServerColumnText"), TEXT("SERVER NAME"));
	CastChecked<UHorizontalBoxSlot>(ServerColumn->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AddText(Tree, Columns, TEXT("PlayersColumnText"), TEXT("PLAYERS"));
	AddText(Tree, Columns, TEXT("PingColumnText"), TEXT("PING"));
	UScrollBox* SessionList = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SessionList"));
	SessionList->bIsVariable = true;
	Browser->AddChildToVerticalBox(SessionList)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

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

	WorldSettings->Modify();
	WorldSettings->DefaultGameMode = AMultiplayerMenuGameMode::StaticClass();
	World->MarkPackageDirty();
	return true;
}

#endif
