// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "PUEditorUtilityBakeAverageColor.generated.h"

class UDataTable;
struct FPUIngredientBase;

/**
 * Editor utility to bake AverageTintColor from ingredient textures into DataTable rows.
 * Run this in the editor before packaging so packaged builds display correct material colors.
 * 
 * Usage:
 * 1. Create a Blueprint based on this class.
 * 2. Set IngredientDataTable in Class Defaults (Details when no node selected).
 * 3. Override the Run event: add Event Run -> Bake Average Colors.
 * 4. Right-click the Blueprint -> Run Editor Utility.
 * 5. Save the DataTable (Ctrl+S) to persist.
 */
UCLASS()
class PROJECTUMEOWMI_API UPUEditorUtilityBakeAverageColor : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/** The ingredient DataTable to bake. Set this in the Blueprint's Class Defaults (Details panel when no node selected). Must have FPUIngredientBase row struct. */
	UPROPERTY(EditAnywhere, Category = "Bake Average Color")
	TObjectPtr<UDataTable> IngredientDataTable;

	/** Bake AverageTintColor for all rows in IngredientDataTable from their PreppedTexture or PreviewTexture. */
	UFUNCTION(BlueprintCallable, Category = "Project Umeowmi|Editor")
	void BakeAverageColors();

	/** Bake a single ingredient row. Returns the computed color, or white if failed. */
	UFUNCTION(BlueprintCallable, Category = "Project Umeowmi|Editor")
	static FLinearColor BakeAverageColorForIngredient(const FPUIngredientBase& Ingredient);
};
