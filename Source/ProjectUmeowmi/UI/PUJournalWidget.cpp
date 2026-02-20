// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUJournalWidget.h"
#include "PUJournalTabListWidget.h"
#include "PUJournalSectionWidget.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonAnimatedSwitcher.h"
#include "CommonButtonBase.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"

UPUJournalWidget::UPUJournalWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LastSelectedTabID(JournalTabNames::Recipes)
{
}

void UPUJournalWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RegisterJournalTabs();
}

void UPUJournalWidget::NativeDestruct()
{
	if (TabList)
	{
		TabList->OnTabButtonCreation.RemoveDynamic(this, &UPUJournalWidget::OnTabButtonCreated);
	}
	SectionWidgets.Empty();
	Super::NativeDestruct();
}

void UPUJournalWidget::OpenJournal()
{
	SetVisibility(ESlateVisibility::Visible);

	if (TabList && bRestoreLastTabOnOpen && LastSelectedTabID != NAME_None)
	{
		TabList->SelectTabByID(LastSelectedTabID, true);
	}
	else if (TabList)
	{
		TabList->SelectTabByID(JournalTabNames::Recipes, true);
	}
}

void UPUJournalWidget::CloseJournal()
{
	if (TabList)
	{
		LastSelectedTabID = TabList->GetActiveTab();
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPUJournalWidget::SwitchToSection(EJournalSectionType SectionType)
{
	FName TabID;
	switch (SectionType)
	{
	case EJournalSectionType::Recipes:     TabID = JournalTabNames::Recipes;     break;
	case EJournalSectionType::Ingredients: TabID = JournalTabNames::Ingredients; break;
	case EJournalSectionType::People:      TabID = JournalTabNames::People;      break;
	case EJournalSectionType::Town:        TabID = JournalTabNames::Town;        break;
	case EJournalSectionType::Settings:    TabID = JournalTabNames::Settings;    break;
	default:                                TabID = JournalTabNames::Recipes;     break;
	}

	if (TabList)
	{
		TabList->SelectTabByID(TabID, true);
	}
}

EJournalSectionType UPUJournalWidget::GetActiveSection() const
{
	if (!TabList) return EJournalSectionType::Recipes;

	const FName ActiveTab = TabList->GetActiveTab();
	if (ActiveTab == JournalTabNames::Recipes)     return EJournalSectionType::Recipes;
	if (ActiveTab == JournalTabNames::Ingredients) return EJournalSectionType::Ingredients;
	if (ActiveTab == JournalTabNames::People)      return EJournalSectionType::People;
	if (ActiveTab == JournalTabNames::Town)        return EJournalSectionType::Town;
	if (ActiveTab == JournalTabNames::Settings)    return EJournalSectionType::Settings;

	return EJournalSectionType::Recipes;
}

void UPUJournalWidget::RegisterJournalTabs()
{
	if (!TabList || !ContentSwitcher || !TabButtonClass)
	{
		return;
	}

	// Pass container and direct reference to this Journal - TabList reads padding from us at add-time
	if (TabButtonsContainer)
	{
		TabList->SetTabButtonsContainer(TabButtonsContainer);
	}
	TabList->SetOwningJournal(this);

	// Link tab list to switcher
	TabList->SetLinkedSwitcher(ContentSwitcher);

	// Disable transition animation so tab switch is instant (no fade-out-then-fade-in gap)
	ContentSwitcher->SetDisableTransitionAnimation(true);

	// Bind to set tab labels when buttons are created
	TabList->OnTabButtonCreation.AddDynamic(this, &UPUJournalWidget::OnTabButtonCreated);

	// Create section widgets and register tabs (Ingredients, People, Town are stubbed)
	struct FSectionConfig
	{
		FName TabID;
		TSubclassOf<UUserWidget> Class;
	};

	TArray<FSectionConfig> Sections;
	Sections.Add({ JournalTabNames::Recipes,     RecipesSectionClass });
	Sections.Add({ JournalTabNames::Ingredients, IngredientsSectionClass });
	Sections.Add({ JournalTabNames::People,      PeopleSectionClass });
	Sections.Add({ JournalTabNames::Town,        TownSectionClass });
	Sections.Add({ JournalTabNames::Settings,    SettingsSectionClass });

	for (int32 i = 0; i < Sections.Num(); ++i)
	{
		const FSectionConfig& Config = Sections[i];
		if (!Config.Class) continue;

		UUserWidget* SectionWidget = CreateAndAddSectionWidget(Config.Class);
		if (SectionWidget)
		{
			SectionWidgets.Add(SectionWidget);
			TabList->RegisterTab(Config.TabID, TabButtonClass, SectionWidget, i);
		}
	}

	// Select Recipes by default
	TabList->SelectTabByID(JournalTabNames::Recipes, true);
}

void UPUJournalWidget::OnTabButtonCreated(FName TabId, UCommonButtonBase* TabButton)
{
	if (!TabButton) return;

	const FText DisplayText = GetTabDisplayText(TabId);
	UUserWidget* ButtonWidget = Cast<UUserWidget>(TabButton);
	if (!ButtonWidget || !ButtonWidget->WidgetTree) return;

	auto TrySetTextBlock = [&](UTextBlock* TextBlock) -> bool
	{
		if (TextBlock)
		{
			TextBlock->SetText(DisplayText);
			return true;
		}
		return false;
	};

	bool bLabelSet = false;

	// 1. If TabButtonLabelWidgetName is set, find that specific widget
	if (TabButtonLabelWidgetName != NAME_None)
	{
		if (UWidget* NamedWidget = ButtonWidget->WidgetTree->FindWidget(TabButtonLabelWidgetName))
		{
			bLabelSet = TrySetTextBlock(Cast<UTextBlock>(NamedWidget));
		}
	}

	// 2. Try common TextBlock names
	if (!bLabelSet)
	{
		static const FName CommonNames[] = { TEXT("ButtonLabel"), TEXT("ButtonText"), TEXT("TabLabel"), TEXT("LabelText"), TEXT("TextBlock"), TEXT("Label") };
		for (const FName& Name : CommonNames)
		{
			if (UWidget* NamedWidget = ButtonWidget->WidgetTree->FindWidget(Name))
			{
				if (UTextBlock* TextBlock = Cast<UTextBlock>(NamedWidget))
				{
					bLabelSet = TrySetTextBlock(TextBlock);
					break;
				}
			}
		}
	}

	// 3. Fall back to first TextBlock in widget tree
	if (!bLabelSet)
	{
		TArray<UWidget*> AllWidgets;
		ButtonWidget->WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* Widget : AllWidgets)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
			{
				bLabelSet = TrySetTextBlock(TextBlock);
				break;
			}
		}
	}
}

FText UPUJournalWidget::GetTabDisplayText(FName TabId) const
{
	// Custom display names (override tab ID for display)
	if (TabId == JournalTabNames::People) return FText::FromString(TEXT("Villagers"));

	// Default: use tab ID with first letter capitalized
	FString Str = TabId.ToString();
	if (Str.Len() > 0)
	{
		Str[0] = FChar::ToUpper(Str[0]);
	}
	return FText::FromString(Str);
}

UUserWidget* UPUJournalWidget::CreateAndAddSectionWidget(TSubclassOf<UUserWidget> WidgetClass)
{
	if (!WidgetClass || !ContentSwitcher) return nullptr;

	UUserWidget* Widget = CreateWidget<UUserWidget>(GetOwningPlayer(), WidgetClass);
	if (!Widget) return nullptr;

	ContentSwitcher->AddChild(Widget);
	return Widget;
}
