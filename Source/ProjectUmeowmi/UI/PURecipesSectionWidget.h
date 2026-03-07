// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PUJournalSectionWidget.h"
#include "PURecipesSectionWidget.generated.h"

class UVerticalBox;
class UImage;
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

	/**
	 * Set the recipe illustration image from dish data. Use this instead of binding directly to
	 * DishData.JournalTexture - the journal texture is a soft reference and must be loaded
	 * explicitly (like prep stage does with LoadSynchronous for materials). This ensures the
	 * texture displays correctly without having to open it in the editor first.
	 */
	UFUNCTION(BlueprintCallable, Category = "Journal|Recipes")
	void SetRecipeIllustration(UImage* Image, const FPUDishBase& DishData);

	/**
	 * Display a dish by its gameplay tag. Called when cycling dishes with bumper keys.
	 * Override in Blueprint to populate your layout: get dish data from Game Instance's
	 * GetDishDataForTag, then call PopulateIngredientsList and SetRecipeIllustration.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Journal|Recipes")
	void DisplayDishByTag(const FGameplayTag& DishTag);
	virtual void DisplayDishByTag_Implementation(const FGameplayTag& DishTag);

protected:
	virtual void OnSectionActivated_Implementation() override;

	/** Optional: if set in Blueprint, DisplayDishByTag will auto-populate these. Override DisplayDishByTag for custom layouts. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> IngredientsContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> RecipeIllustrationImage;
};
