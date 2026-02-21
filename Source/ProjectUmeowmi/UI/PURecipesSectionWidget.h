// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUJournalSectionWidget.h"
#include "PURecipesSectionWidget.generated.h"

class UVerticalBox;
class UPURecipeIngredientEntryWidget;
struct FPUDishBase;

/**
 * Recipes section of the journal - displays discovered recipes in a two-page spread layout.
 * Based on the reference: left page and right page each show a recipe with title, description,
 * difficulty stars, ingredients list, liked-by avatars, flavor profile, and illustration.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPURecipesSectionWidget : public UPUJournalSectionWidget
{
	GENERATED_BODY()

public:
	UPURecipesSectionWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * Populate a vertical box with ingredient checklist entries from the given dish.
	 * Each entry shows the ingredient name with an unchecked checkbox (for future progress tracking).
	 * @param Container - The vertical box to add entries to (will be cleared first)
	 * @param DishData - The dish whose ingredients to display
	 * @param EntryWidgetClass - Widget class for each row (defaults to PURecipeIngredientEntryWidget)
	 */
	UFUNCTION(BlueprintCallable, Category = "Journal|Recipes")
	void PopulateIngredientsList(UVerticalBox* Container, const FPUDishBase& DishData,
		TSubclassOf<UPURecipeIngredientEntryWidget> EntryWidgetClass = nullptr);
};
