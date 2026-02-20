// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "PUCommonUserWidget.h"
#include "PUJournalTypes.h"
#include "PUJournalWidget.generated.h"

class UCommonActivatableWidgetSwitcher;
class UCommonButtonBase;
class UPUJournalTabListWidget;
class UVerticalBox;

/**
 * Main journal/recipe book widget - the open book with tabbed sections.
 * Contains the tab bar (vertical, right edge) and content switcher (book pages).
 * Sections: Recipes, Ingredients, People, Town, Settings. (Ingredients, People, Town are stubbed.)
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPUJournalWidget : public UPUCommonUserWidget
{
	GENERATED_BODY()

public:
	UPUJournalWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Open the journal (show widget, optionally restore last tab) */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void OpenJournal();

	/** Close the journal */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void CloseJournal();

	/** Switch to a specific section by type */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void SwitchToSection(EJournalSectionType SectionType);

	/** Get the currently active section */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Journal")
	EJournalSectionType GetActiveSection() const;

	/** Get the tab list widget */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Journal")
	UPUJournalTabListWidget* GetTabList() const { return TabList; }

	/** Get the content switcher */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Journal")
	UCommonActivatableWidgetSwitcher* GetContentSwitcher() const { return ContentSwitcher; }

	/** Get tab slot padding (read at add-time so Blueprint edits apply without restart) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Journal")
	FMargin GetTabSlotPadding() const { return TabSlotPadding; }

protected:
	/** Register all journal tabs and link tab list to switcher */
	void RegisterJournalTabs();

	/** Create a section widget from class and add to switcher */
	UUserWidget* CreateAndAddSectionWidget(TSubclassOf<UUserWidget> WidgetClass);

	/** Called when a tab button is created - sets the label text */
	UFUNCTION()
	void OnTabButtonCreated(FName TabId, UCommonButtonBase* TabButton);

	/** Get display text for a tab ID */
	FText GetTabDisplayText(FName TabId) const;

	// UI Elements - use BindWidget in Blueprint.
	// Place all inside a root container (Canvas Panel, Overlay, etc.) as siblings:
	//   Root Container
	//   ├── TabButtonsContainer (Vertical Box - tab buttons get added here at runtime)
	//   ├── TabList (no children - receives container via SetTabButtonsContainer)
	//   └── ContentSwitcher (book pages)
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Journal|UI")
	TObjectPtr<UVerticalBox> TabButtonsContainer;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Journal|UI")
	TObjectPtr<UPUJournalTabListWidget> TabList;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Journal|UI")
	TObjectPtr<UCommonActivatableWidgetSwitcher> ContentSwitcher;

	// Section widget classes - assign in Blueprint or defaults
	// Ingredients, People, Town are stubbed - add layouts when ready
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Sections")
	TSubclassOf<UUserWidget> RecipesSectionClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Sections")
	TSubclassOf<UUserWidget> IngredientsSectionClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Sections")
	TSubclassOf<UUserWidget> PeopleSectionClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Sections")
	TSubclassOf<UUserWidget> TownSectionClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Sections")
	TSubclassOf<UUserWidget> SettingsSectionClass;

	/** Tab button widget class - used for all tabs (Recipes, Ingredients, etc.) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Tabs")
	TSubclassOf<UCommonButtonBase> TabButtonClass;

	/** Optional: name of the TextBlock in the tab button that displays the label (e.g. "ButtonText", "TabLabel"). Leave empty to auto-detect. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Tabs", meta = (DisplayName = "Tab Label TextBlock Name"))
	FName TabButtonLabelWidgetName;

	/** Padding between tab buttons (Left, Top, Right, Bottom per slot). E.g. (0, 0, 0, 8) = 8px gap between tabs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Tabs")
	FMargin TabSlotPadding = FMargin(0.f, 0.f, 0.f, 8.f);

	/** Restore the last selected tab when opening the journal */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Behavior")
	bool bRestoreLastTabOnOpen = true;

	/** Last selected tab ID for restoration */
	UPROPERTY(Transient)
	FName LastSelectedTabID;

	/** Created section widgets for cleanup */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> SectionWidgets;
};
