// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "PUCommonUserWidget.h"
#include "PUJournalTypes.h"
#include "PUJournalWidget.generated.h"

class UPURecipesSectionWidget;

class UCommonActivatableWidgetSwitcher;
class UCommonButtonBase;
class UPUJournalTabListWidget;
class UVerticalBox;

/**
 * Main journal/recipe book widget - the open book with tabbed sections.
 * Contains the tab bar (vertical, right edge) and content switcher (book pages).
 * Tab list is driven by the Journal Tabs array (class defaults on the journal widget Blueprint).
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

	/** Switch to the tab with this id (must match a Tab Id from the journal tab list). */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void SwitchToTabById(FName TabId);

	/** Active tab id (FName from your journal tab configuration). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Journal")
	FName GetActiveTabId() const;

	/** Cycle journal section tabs in Journal Tabs order. Direction: +1 next, -1 previous (wraps). Returns false if fewer than two tabs or tab unchanged. */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	bool CycleJournalTab(int32 Direction);

	/** Cycle the displayed dish in the Recipes tab. Direction: +1 next, -1 previous. Returns true if a dish was cycled. */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	bool CycleRecipesDish(int32 Direction);

	/** Switch to the Recipes tab and show the given dish (updates current dish tag on the game instance). */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void ShowDishInRecipesTab(const FGameplayTag& DishTag);

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

	UPURecipesSectionWidget* GetRecipesSection() const;

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

	/**
	 * Tab order and labels. Add one row per tab (unique Tab Id, section widget class, optional display name).
	 * EditAnywhere: set on this widget's Class Defaults, or on a placed instance in another widget (EditDefaultsOnly hid this on instances before).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Tabs", meta = (TitleProperty = "TabId"))
	TArray<FPUJournalTabEntry> JournalTabs;

	/**
	 * Optional. Tab Id of the dishes/recipes section — must match a Journal Tabs row if set.
	 * If None, the first registered UPURecipesSectionWidget is used for Get Recipes Section / dish cycling.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Tabs")
	FName RecipesTabId;

	/**
	 * Optional. Tab to select when opening if last-tab restore does not apply.
	 * If None or not in the list, the first tab in Journal Tabs is used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Tabs")
	FName DefaultTabId;

	/** Tab button widget class - used for all tabs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Tabs")
	TSubclassOf<UCommonButtonBase> TabButtonClass;

	/** Optional: name of the TextBlock in the tab button that displays the label (e.g. "ButtonText", "TabLabel"). Leave empty to auto-detect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Tabs", meta = (DisplayName = "Tab Label TextBlock Name"))
	FName TabButtonLabelWidgetName;

	/** Padding between tab buttons (Left, Top, Right, Bottom per slot). E.g. (0, 0, 0, 8) = 8px gap between tabs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Tabs")
	FMargin TabSlotPadding = FMargin(0.f, 0.f, 0.f, 8.f);

	/**
	 * Viewport Z when opening (dish customization UI uses ~250). Ensures the journal paints on top after UnhideCollapsedAncestors / AddToViewport.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Viewport")
	int32 JournalViewportZOrderWhenOpen = 300;

	/** Restore the last selected tab when opening the journal */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Behavior")
	bool bRestoreLastTabOnOpen = true;

	/** Last selected tab ID for restoration */
	UPROPERTY(Transient)
	FName LastSelectedTabID;

	/** Created section widgets for cleanup (order matches the tab list). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> SectionWidgets;

	/** Tab id per section widget (same index as SectionWidgets). */
	UPROPERTY(Transient)
	TArray<FName> SectionTabIds;

	/** Entries actually registered (for label lookup); mirrors Journal Tabs after validation. */
	UPROPERTY(Transient)
	TArray<FPUJournalTabEntry> ResolvedTabEntries;

	bool HasTabId(FName TabId) const;
	FName GetEffectiveDefaultTabId() const;

	/** Tab id for the recipes/dishes section: Recipes Tab Id if set, else first UPURecipesSectionWidget's tab. */
	FName ResolveRecipesTabId() const;
};
