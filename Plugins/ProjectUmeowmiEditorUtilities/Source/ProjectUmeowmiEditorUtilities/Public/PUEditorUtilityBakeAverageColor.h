// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "Engine/DataTable.h"
#include "PUEditorUtilityBakeAverageColor.generated.h"

struct FPUIngredientBase;

/**
 * Editor utility to bake average tint colors from ingredient textures into a DataTable.
 * Create a Blueprint based on this class, set IngredientDataTable, then run BakeAverageColors.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMIEDITORUTILITIES_API UPUEditorUtilityBakeAverageColor : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/** DataTable with FPUIngredientBase rows. Set this in the Blueprint Details panel before running BakeAverageColors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake Average Color")
	TObjectPtr<UDataTable> IngredientDataTable;

	/** Bakes average color from a single ingredient row. Returns White if row not found or texture read fails. */
	UFUNCTION(BlueprintCallable, Category = "Bake Average Color")
	FLinearColor BakeAverageColorForIngredientRow(UDataTable* DataTable, FName RowName);

	/** Legacy: Bakes average color from a single ingredient's texture. Use BakeAverageColors() or BakeAverageColorForIngredientRow instead. */
	FLinearColor BakeAverageColorForIngredient(const FPUIngredientBase& Ingredient);

	/** Bakes average tint colors for all rows in IngredientDataTable. Save the DataTable (Ctrl+S) to persist. */
	UFUNCTION(BlueprintCallable, Category = "Bake Average Color")
	void BakeAverageColors();
};
