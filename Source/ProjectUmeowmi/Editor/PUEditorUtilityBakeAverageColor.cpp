// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUEditorUtilityBakeAverageColor.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

namespace
{
	// Replicate pixel averaging logic from PUIngredientSlot::GetAverageColorFromIngredientTexture
	// Returns raw average (no saturation) - runtime applies saturation when using fallback
	bool ComputeAverageColorFromTexture(UTexture2D* Texture, FLinearColor& OutColor)
	{
		if (!Texture) return false;

		FImage SourceImage;
		if (!FImageUtils::GetTexture2DSourceImage(Texture, SourceImage))
		{
			return false;
		}

		const int32 Width = Texture->GetSizeX();
		const int32 Height = Texture->GetSizeY();
		if (Width <= 0 || Height <= 0) return false;

		const TArray64<uint8>& RawData = SourceImage.RawData;
		const int32 DataSize = RawData.Num();
		const int32 BytesPerPixel = 4;
		const int32 ExpectedDataSize = Width * Height * BytesPerPixel;

		int64 TotalR = 0, TotalG = 0, TotalB = 0;
		int32 PixelCount = 0;

		if (DataSize >= ExpectedDataSize)
		{
			for (int32 Y = 0; Y < Height; Y++)
			{
				for (int32 X = 0; X < Width; X++)
				{
					const int32 PixelIndex = (Y * Width + X) * BytesPerPixel;
					if (PixelIndex + 2 < DataSize)
					{
						uint8 B = RawData[PixelIndex];
						uint8 G = RawData[PixelIndex + 1];
						uint8 R = RawData[PixelIndex + 2];
						TotalR += R;
						TotalG += G;
						TotalB += B;
						PixelCount++;
					}
				}
			}
		}
		else
		{
			const int32 BytesPerPixelRGB = 3;
			const int32 ExpectedDataSizeRGB = Width * Height * BytesPerPixelRGB;
			if (DataSize >= ExpectedDataSizeRGB)
			{
				for (int32 Y = 0; Y < Height; Y++)
				{
					for (int32 X = 0; X < Width; X++)
					{
						const int32 PixelIndex = (Y * Width + X) * BytesPerPixelRGB;
						if (PixelIndex + 2 < DataSize)
						{
							uint8 B = RawData[PixelIndex];
							uint8 G = RawData[PixelIndex + 1];
							uint8 R = RawData[PixelIndex + 2];
							TotalR += R;
							TotalG += G;
							TotalB += B;
							PixelCount++;
						}
					}
				}
			}
		}

		if (PixelCount <= 0) return false;

		OutColor = FLinearColor(
			static_cast<uint8>(TotalR / PixelCount) / 255.0f,
			static_cast<uint8>(TotalG / PixelCount) / 255.0f,
			static_cast<uint8>(TotalB / PixelCount) / 255.0f,
			1.0f
		);
		return true;
	}
}

FLinearColor UPUEditorUtilityBakeAverageColor::BakeAverageColorForIngredient(const FPUIngredientBase& Ingredient)
{
	UTexture2D* Texture = Ingredient.PreppedTexture ? Ingredient.PreppedTexture : Ingredient.PreviewTexture;
	FLinearColor OutColor;
	if (ComputeAverageColorFromTexture(Texture, OutColor))
	{
		return OutColor;
	}
	return FLinearColor::White;
}

void UPUEditorUtilityBakeAverageColor::BakeAverageColors()
{
#if !WITH_EDITOR
	return;
#else
	if (!IngredientDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("PUEditorUtilityBakeAverageColor: IngredientDataTable is NOT SET. Open the Blueprint, set 'Ingredient Data Table' in the Details panel, then run again."));
		return;
	}

	const UScriptStruct* RowStruct = IngredientDataTable->GetRowStruct();
	if (!RowStruct)
	{
		UE_LOG(LogTemp, Error, TEXT("PUEditorUtilityBakeAverageColor: DataTable '%s' has no row struct."), *IngredientDataTable->GetName());
		return;
	}
	if (RowStruct != FPUIngredientBase::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("PUEditorUtilityBakeAverageColor: DataTable '%s' row struct is '%s', not FPUIngredientBase. Use a DataTable created with FPUIngredientBase as the row struct."), *IngredientDataTable->GetName(), *RowStruct->GetName());
		return;
	}

	FScopedTransaction Transaction(NSLOCTEXT("ProjectUmeowmi", "BakeAverageColors", "Bake Average Tint Colors"));

	TArray<FName> RowNames;
	IngredientDataTable->GetRowMap().GetKeys(RowNames);

	int32 BakedCount = 0;
	int32 SkippedNoTexture = 0;
	int32 SkippedTextureFail = 0;
	for (const FName& RowName : RowNames)
	{
		const FPUIngredientBase* Row = IngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("BakeAverageColors"));
		if (!Row) continue;

		UTexture2D* Texture = Row->PreppedTexture ? Row->PreppedTexture : Row->PreviewTexture;
		if (!Texture)
		{
			SkippedNoTexture++;
			continue;
		}

		FLinearColor BakedColor;
		if (ComputeAverageColorFromTexture(Texture, BakedColor))
		{
			FPUIngredientBase ModifiedRow = *Row;
			ModifiedRow.AverageTintColor = BakedColor;
			IngredientDataTable->AddRow(RowName, ModifiedRow);
			BakedCount++;
		}
		else
		{
			SkippedTextureFail++;
		}
	}

	IngredientDataTable->MarkPackageDirty();

	UE_LOG(LogTemp, Display, TEXT("PUEditorUtilityBakeAverageColor: Baked %d/%d ingredient colors in '%s'. Save the DataTable (Ctrl+S) to persist."), BakedCount, RowNames.Num(), *IngredientDataTable->GetName());
	if (SkippedNoTexture > 0 || SkippedTextureFail > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("  Skipped: %d rows had no texture, %d rows failed texture read (try reimporting textures as uncompressed)."), SkippedNoTexture, SkippedTextureFail);
	}
#endif
}
