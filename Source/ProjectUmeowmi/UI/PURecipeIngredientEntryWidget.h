// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUCommonUserWidget.h"
#include "PURecipeIngredientEntryWidget.generated.h"

class UCheckBox;
class UTextBlock;

/**
 * Single ingredient entry for the recipe book checklist.
 * Displays a checkbox and ingredient name. Used in a vertical list within the recipes section.
 *
 * Blueprint setup: Create a WBP that inherits from this class with:
 *   - CheckBox named "IngredientCheckBox"
 *   - TextBlock named "IngredientNameText"
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPURecipeIngredientEntryWidget : public UPUCommonUserWidget
{
	GENERATED_BODY()

public:
	UPURecipeIngredientEntryWidget(const FObjectInitializer& ObjectInitializer);

	/** Set the display data for this entry. bChecked defaults to false for recipe display. */
	UFUNCTION(BlueprintCallable, Category = "Recipe Ingredient Entry")
	void SetData(const FText& InIngredientName, bool bChecked = false);

	/** Get the current ingredient name */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Recipe Ingredient Entry")
	FText GetIngredientName() const { return IngredientName; }

	/** Get the current checked state */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Recipe Ingredient Entry")
	bool IsChecked() const;

	/** Set the checked state (for future progress tracking) */
	UFUNCTION(BlueprintCallable, Category = "Recipe Ingredient Entry")
	void SetChecked(bool bChecked);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Recipe Ingredient Entry")
	TObjectPtr<UCheckBox> IngredientCheckBox;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Recipe Ingredient Entry")
	TObjectPtr<UTextBlock> IngredientNameText;

private:
	FText IngredientName;

	void UpdateDisplay();
};
