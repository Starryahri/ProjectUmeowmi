// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "Engine/DataTable.h"
#include "PUEditorUtilityImportTimeTempModifiers.generated.h"

struct FPUIngredientBase;
struct FTimeTempModifier;

/**
 * Editor utility to import Time/Temperature modifiers from a CSV file into the Ingredient Data Table.
 * 
 * CSV format: Ingredient,Time,Temp,Aspect,Type,Value,Rationale
 * - Ingredient: Display name (e.g. "BBQ Duck", "Noodle Buckwheat") - matched to row name (lowercase, no spaces)
 * - Time: None, Low, Mid, Long
 * - Temp: Raw, Low, Med, Hot
 * - Aspect: Umami, Salt, Sweet, Sour, Bitter, Spicy, Rich, Juicy, Tender, Chewy, Crispy, Crumbly
 * - Type: Additive, Multiplicative
 * - Value: Float (-5 to 5)
 * - Rationale: (ignored on import)
 * 
 * Create a Blueprint based on this class, set IngredientDataTable and CSV path, then run ImportFromCSV.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMIEDITORUTILITIES_API UPUEditorUtilityImportTimeTempModifiers : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/** DataTable with FPUIngredientBase rows. Set this in the Blueprint Details panel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Time/Temp Modifiers")
	TObjectPtr<UDataTable> IngredientDataTable;

	/** Full path to the CSV file (e.g. D:/Game Projects/Unreal/ProjectUmeowmi/Ingredient_TimeTemp_Modifiers.csv) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Time/Temp Modifiers", meta = (ContentDir))
	FString CSVFilePath;

	/** Imports Time/Temperature modifiers from the CSV file into IngredientDataTable. Save the DataTable (Ctrl+S) to persist. */
	UFUNCTION(BlueprintCallable, Category = "Import Time/Temp Modifiers")
	void ImportFromCSV();
};
