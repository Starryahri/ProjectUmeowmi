// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUEditorUtilityImportTimeTempModifiers.h"
#include "DishCustomization/PUIngredientBase.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

namespace
{
	/** Convert display name (e.g. "BBQ Duck") to row name (e.g. "bbqduck") */
	FName IngredientDisplayNameToRowName(const FString& DisplayName)
	{
		FString Result = DisplayName;
		Result.ToLowerInline();
		Result.ReplaceInline(TEXT(" "), TEXT(""));
		Result.ReplaceInline(TEXT("."), TEXT(""));
		return FName(*Result);
	}

	ETimeState ParseTimeState(const FString& Str)
	{
		if (Str.Equals(TEXT("None"), ESearchCase::IgnoreCase)) return ETimeState::None;
		if (Str.Equals(TEXT("Low"), ESearchCase::IgnoreCase)) return ETimeState::Low;
		if (Str.Equals(TEXT("Mid"), ESearchCase::IgnoreCase)) return ETimeState::Mid;
		if (Str.Equals(TEXT("Long"), ESearchCase::IgnoreCase)) return ETimeState::Long;
		return ETimeState::None;
	}

	ETemperatureState ParseTempState(const FString& Str)
	{
		if (Str.Equals(TEXT("Raw"), ESearchCase::IgnoreCase)) return ETemperatureState::Raw;
		if (Str.Equals(TEXT("Low"), ESearchCase::IgnoreCase)) return ETemperatureState::Low;
		if (Str.Equals(TEXT("Med"), ESearchCase::IgnoreCase)) return ETemperatureState::Med;
		if (Str.Equals(TEXT("Hot"), ESearchCase::IgnoreCase)) return ETemperatureState::Hot;
		return ETemperatureState::Raw;
	}

	bool ParseAspect(const FString& Str, EPAspectCategory& OutCategory, EPUFlavorAspect& OutFlavor, EPUTextureAspect& OutTexture)
	{
		const FString Lower = Str.ToLower();
		if (Lower == TEXT("umami")) { OutCategory = EPAspectCategory::Flavor; OutFlavor = EPUFlavorAspect::Umami; return true; }
		if (Lower == TEXT("salt")) { OutCategory = EPAspectCategory::Flavor; OutFlavor = EPUFlavorAspect::Salt; return true; }
		if (Lower == TEXT("sweet")) { OutCategory = EPAspectCategory::Flavor; OutFlavor = EPUFlavorAspect::Sweet; return true; }
		if (Lower == TEXT("sour")) { OutCategory = EPAspectCategory::Flavor; OutFlavor = EPUFlavorAspect::Sour; return true; }
		if (Lower == TEXT("bitter")) { OutCategory = EPAspectCategory::Flavor; OutFlavor = EPUFlavorAspect::Bitter; return true; }
		if (Lower == TEXT("spicy")) { OutCategory = EPAspectCategory::Flavor; OutFlavor = EPUFlavorAspect::Spicy; return true; }
		if (Lower == TEXT("rich")) { OutCategory = EPAspectCategory::Texture; OutTexture = EPUTextureAspect::Rich; return true; }
		if (Lower == TEXT("juicy")) { OutCategory = EPAspectCategory::Texture; OutTexture = EPUTextureAspect::Juicy; return true; }
		if (Lower == TEXT("tender")) { OutCategory = EPAspectCategory::Texture; OutTexture = EPUTextureAspect::Tender; return true; }
		if (Lower == TEXT("chewy")) { OutCategory = EPAspectCategory::Texture; OutTexture = EPUTextureAspect::Chewy; return true; }
		if (Lower == TEXT("crispy")) { OutCategory = EPAspectCategory::Texture; OutTexture = EPUTextureAspect::Crispy; return true; }
		if (Lower == TEXT("crumbly")) { OutCategory = EPAspectCategory::Texture; OutTexture = EPUTextureAspect::Crumbly; return true; }
		return false;
	}

	uint8 ParseModificationType(const FString& Str)
	{
		if (Str.Equals(TEXT("Multiplicative"), ESearchCase::IgnoreCase)) return 1;
		return 0; // Additive default
	}
}

void UPUEditorUtilityImportTimeTempModifiers::ImportFromCSV()
{
#if !WITH_EDITOR
	return;
#else
	if (!IngredientDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTimeTempModifiers: IngredientDataTable is NOT SET. Set it in the Details panel."));
		return;
	}
	if (CSVFilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTimeTempModifiers: CSVFilePath is empty. Set the path to your CSV file."));
		return;
	}

	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *CSVFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTimeTempModifiers: Failed to load file '%s'. Check the path."), *CSVFilePath);
		return;
	}

	const UScriptStruct* RowStruct = IngredientDataTable->GetRowStruct();
	if (!RowStruct || RowStruct != FPUIngredientBase::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTimeTempModifiers: DataTable must use FPUIngredientBase as row struct."));
		return;
	}

	FScopedTransaction Transaction(NSLOCTEXT("ProjectUmeowmi", "ImportTimeTempModifiers", "Import Time/Temp Modifiers from CSV"));

	// Parse CSV: Ingredient,Time,Temp,Aspect,Type,Value,Rationale
	TArray<FString> Lines;
	CSVContent.ParseIntoArray(Lines, TEXT("\n"), true);

	if (Lines.Num() < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTimeTempModifiers: CSV has no data rows (only header or empty)."));
		return;
	}

	// Map: RowName -> TArray<FTimeTempModifier>
	TMap<FName, TArray<FTimeTempModifier>> ModifiersByIngredient;

	int32 ParsedCount = 0;
	int32 SkippedInvalid = 0;
	int32 SkippedUnknownAspect = 0;

	for (int32 i = 1; i < Lines.Num(); ++i) // Skip header
	{
		TArray<FString> Fields;
		Lines[i].ParseIntoArray(Fields, TEXT(","), true);

		if (Fields.Num() < 6)
		{
			SkippedInvalid++;
			continue;
		}

		FString Ingredient = Fields[0].TrimStartAndEnd();
		FString TimeStr = Fields[1].TrimStartAndEnd();
		FString TempStr = Fields[2].TrimStartAndEnd();
		FString AspectStr = Fields[3].TrimStartAndEnd();
		FString TypeStr = Fields[4].TrimStartAndEnd();
		float Value = FCString::Atof(*Fields[5].TrimStartAndEnd());

		EPAspectCategory AspectCategory;
		EPUFlavorAspect FlavorAspect = EPUFlavorAspect::Umami;
		EPUTextureAspect TextureAspect = EPUTextureAspect::Tender;
		if (!ParseAspect(AspectStr, AspectCategory, FlavorAspect, TextureAspect))
		{
			SkippedUnknownAspect++;
			continue;
		}

		FTimeTempModifier Mod;
		Mod.TimeState = ParseTimeState(TimeStr);
		Mod.TemperatureState = ParseTempState(TempStr);
		Mod.AspectCategory = AspectCategory;
		Mod.FlavorAspect = FlavorAspect;
		Mod.TextureAspect = TextureAspect;
		Mod.ModificationType = ParseModificationType(TypeStr);
		Mod.ModificationValue = Value;

		FName RowName = IngredientDisplayNameToRowName(Ingredient);
		TArray<FTimeTempModifier>& Arr = ModifiersByIngredient.FindOrAdd(RowName);
		Arr.Add(Mod);
		ParsedCount++;
	}

	// Apply to data table
	int32 UpdatedCount = 0;
	int32 SkippedNoRow = 0;

	for (const auto& Pair : ModifiersByIngredient)
	{
		const FName RowName = Pair.Key;
		const TArray<FTimeTempModifier>& Modifiers = Pair.Value;

		FPUIngredientBase* Row = IngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("ImportTimeTempModifiers"));
		if (!Row)
		{
			SkippedNoRow++;
			UE_LOG(LogTemp, Warning, TEXT("ImportTimeTempModifiers: No row found for '%s' (from CSV ingredient). Skipping."), *RowName.ToString());
			continue;
		}

		FPUIngredientBase ModifiedRow = *Row;
		ModifiedRow.TimeTemperatureModifiers = Modifiers;
		ModifiedRow.bUseCustomTimeTempModifiers = true;
		IngredientDataTable->AddRow(RowName, ModifiedRow);
		UpdatedCount++;
	}

	IngredientDataTable->MarkPackageDirty();

	UE_LOG(LogTemp, Display, TEXT("ImportTimeTempModifiers: Imported %d modifiers into %d ingredients. Save the DataTable (Ctrl+S) to persist."), ParsedCount, UpdatedCount);
	if (SkippedInvalid > 0) UE_LOG(LogTemp, Warning, TEXT("  Skipped %d rows (invalid format)."), SkippedInvalid);
	if (SkippedUnknownAspect > 0) UE_LOG(LogTemp, Warning, TEXT("  Skipped %d rows (unknown aspect name)."), SkippedUnknownAspect);
	if (SkippedNoRow > 0) UE_LOG(LogTemp, Warning, TEXT("  %d CSV ingredients had no matching DataTable row."), SkippedNoRow);
#endif
}
