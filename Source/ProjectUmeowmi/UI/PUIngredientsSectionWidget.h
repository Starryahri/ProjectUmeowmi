// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "PUJournalSectionWidget.h"
#include "PUIngredientsSectionWidget.generated.h"

class UUniformGridPanel;
class UScrollBox;
class UTextBlock;
class UImage;
class UPUJournalSlotWidget;
class UPUJournalAspectRowWidget;
class UPanelWidget;

/**
 * Ingredients journal section: fills a uniform grid with PUJournalSlotWidget cells from the
 * Game Instance ingredient data table. Fixed grids pack unlocked ingredients first (no gaps),
 * then locked, then empty padding. Hover/focus updates
 * the detail panel via ApplyIngredientDetail — override in Blueprint for full layout.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPUIngredientsSectionWidget : public UPUJournalSectionWidget
{
	GENERATED_BODY()

public:
	UPUIngredientsSectionWidget(const FObjectInitializer& ObjectInitializer);

	/** Rebuild the grid from the ingredient table. Safe to call when the tab is shown or inventory changes. */
	UFUNCTION(BlueprintCallable, Category = "Journal|Ingredients")
	void RefreshIngredientsGrid(bool bApplyDefaultDetailAndFocus = true);

	/** After @RefreshIngredientsGrid, move focus next tick to the unlocked grid cell matching this tag if any (no-op if locked-only or missing). */
	UFUNCTION(BlueprintCallable, Category = "Journal|Ingredients")
	void ScheduleFocusIngredientGridSlot(const FGameplayTag& IngredientTag);

	/**
	 * Look up an ingredient row and refresh the right-hand detail area.
	 * Called when a slot is hovered/focused and once after grid refresh (first entry).
	 */
	UFUNCTION(BlueprintCallable, Category = "Journal|Ingredients")
	void ShowIngredientDetail(const FGameplayTag& IngredientTag);

protected:
	virtual void OnSectionActivated_Implementation() override;

	/** Optional default wiring for title, description, and hero image. Override in Blueprint for custom UI. */
	UFUNCTION(BlueprintNativeEvent, Category = "Journal|Ingredients")
	void ApplyIngredientDetail(const FGameplayTag& IngredientTag, const FPUIngredientBase& IngredientData, bool bFoundInTable);
	virtual void ApplyIngredientDetail_Implementation(const FGameplayTag& IngredientTag, const FPUIngredientBase& IngredientData, bool bFoundInTable);

	UFUNCTION()
	void OnJournalSlotHovered(FGameplayTag EntryTag);

	UFUNCTION()
	void OnIngredientRecipeDishSlotHovered(FGameplayTag DishTag);

	void PopulateIngredientRecipesForIngredient(const FGameplayTag& IngredientTag);
	void ClearIngredientRecipesList();

	void RefreshIngredientJournalAspectRows(const FPUIngredientBase& IngredientData);
	void ClearIngredientJournalAspectRows();

	/** After refresh, move keyboard/gamepad focus to the first enabled grid slot (next tick so layout exists). D-pad moves between focusable slots via Slate navigation, not Enhanced Input. */
	void ScheduleFocusFirstIngredientsGridSlot();

	void TryFocusFirstInteractableIngredientsSlot();

	void TryFocusIngredientGridSlot(const FGameplayTag& IngredientTag);

	/** Slate default navigation escapes the grid to the next focusable widget (tab buttons). Wire cardinal neighbors + Stop at edges. */
	void SetupIngredientsGridNavigation();

	/** Grid for ingredient slots. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Ingredients|UI")
	TObjectPtr<UUniformGridPanel> IngredientsGrid;

	/** Widget class for each cell (e.g. Blueprint child of PUJournalSlotWidget). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Ingredients")
	TSubclassOf<UPUJournalSlotWidget> JournalSlotClass;

	/** Number of columns in the uniform grid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal|Ingredients", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridNumColumns = 5;

	/**
	 * Minimum cell width/height on IngredientsGrid so empty slots stay visible. Zero leaves the panel unchanged.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal|Ingredients", meta = (ClampMin = "0", UIMin = "0"))
	float GridCellMinWidth = 96.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal|Ingredients", meta = (ClampMin = "0", UIMin = "0"))
	float GridCellMinHeight = 96.f;

	/**
	 * Fixed number of grid cells. When greater than zero, exactly this many slots are created.
	 * Ingredients are packed without gaps: all unlocked rows first, then locked rows (each group sorted
	 * by display name). Remaining cells are empty padding. When zero, legacy mode: one slot per
	 * unlocked ingredient only (see bOnlyShowUnlockedIngredients).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal|Ingredients", meta = (ClampMin = "0", UIMin = "0"))
	int32 TotalGridSlots = 0;

	/**
	 * Legacy mode only (TotalGridSlots == 0). If true, only unlocked ingredients get a slot.
	 * Ignored when TotalGridSlots is greater than zero.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal|Ingredients")
	bool bOnlyShowUnlockedIngredients = true;

	/** After refresh, show the first ingredient in the detail panel without requiring hover. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal|Ingredients")
	bool bSelectFirstIngredientOnRefresh = true;

	/** When true (default), after refresh schedule focus on the first enabled slot so gamepad D-pad can navigate the grid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal|Ingredients")
	bool bFocusFirstGridSlotOnRefresh = true;

	/**
	 * When populating dishes that use the selected ingredient, only list dishes unlocked in the journal.
	 * When false, all matching dishes from the dish table are shown.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal|Ingredients")
	bool bOnlyShowUnlockedDishesInIngredientRecipeList = true;

	/** Scroll list of dishes that include the selected ingredient (journal slots with dish art). Name must match in Designer. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Ingredients|UI")
	TObjectPtr<UScrollBox> IngredientRecipesScrollBox;

	/**
	 * Optional panels that hold aspect rows (e.g. VerticalBoxes in Blueprint). C++ clears and adds one
	 * PUJournalAspectRowWidget per flavor/texture aspect — layout is entirely in the widget Blueprint.
	 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Ingredients|UI")
	TObjectPtr<UPanelWidget> IngredientFlavorAspectsContainer;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Ingredients|UI")
	TObjectPtr<UPanelWidget> IngredientTextureAspectsContainer;

	/** Row widget for journal aspect lines; defaults to native PUJournalAspectRowWidget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Ingredients")
	TSubclassOf<UPUJournalAspectRowWidget> JournalAspectRowClass;

	/**
	 * When true, all six flavor and six texture aspect rows are shown, including at 0.
	 * When false (default), aspects with a value of 0 are omitted.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Ingredients")
	bool bShowIngredientAspectRowsWithZeroValue = false;

	/** Optional detail bindings — filled by default C++ implementation when present. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Ingredients|UI")
	TObjectPtr<UTextBlock> IngredientTitleText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Ingredients|UI")
	TObjectPtr<UTextBlock> IngredientDescriptionText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Ingredients|UI")
	TObjectPtr<UImage> IngredientDetailImage;
};
