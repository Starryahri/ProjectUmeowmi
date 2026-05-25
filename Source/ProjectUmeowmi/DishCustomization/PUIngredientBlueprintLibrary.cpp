#include "PUIngredientBlueprintLibrary.h"

bool UPUIngredientBlueprintLibrary::ApplyPreparation(FPUIngredientBase& Ingredient, const FPUPreparationBase& Preparation)
{
    return Ingredient.ApplyPreparation(Preparation);
}

bool UPUIngredientBlueprintLibrary::RemovePreparation(FPUIngredientBase& Ingredient, const FPUPreparationBase& Preparation)
{
    return Ingredient.RemovePreparation(Preparation);
}

bool UPUIngredientBlueprintLibrary::HasPreparation(const FPUIngredientBase& Ingredient, const FGameplayTag& PreparationTag)
{
    return Ingredient.HasPreparation(PreparationTag);
}

FText UPUIngredientBlueprintLibrary::GetCurrentDisplayName(const FPUIngredientBase& Ingredient, UDataTable* PreparationTable)
{
    return Ingredient.GetCurrentDisplayName(PreparationTable);
}

UTexture2D* UPUIngredientBlueprintLibrary::GetCutVisualTexture(
    const FPUIngredientBase& Ingredient,
    EPUIngredientCutVisualTier CutTier)
{
    return Ingredient.GetCutVisualTexture(CutTier);
}

FLinearColor UPUIngredientBlueprintLibrary::GetMinigameTintColorForCutTier(
    const FPUIngredientBase& Ingredient,
    EPUIngredientCutVisualTier CutTier,
    float SaturationMultiplier,
    bool bApplySaturationBoost)
{
    if (CutTier == EPUIngredientCutVisualTier::Whole)
    {
        return FLinearColor::White;
    }

    const FLinearColor BaseColor = Ingredient.AverageTintColor;
    if (!bApplySaturationBoost)
    {
        return BaseColor;
    }

    return BoostColorSaturation(BaseColor, SaturationMultiplier);
}

FLinearColor UPUIngredientBlueprintLibrary::GetMinigameTintColor(const FPUIngredientBase& Ingredient)
{
    return GetMinigameTintColorForCutTier(Ingredient, EPUIngredientCutVisualTier::Whole);
}

FLinearColor UPUIngredientBlueprintLibrary::BoostColorSaturation(FLinearColor Color, float SaturationMultiplier)
{
    const float Multiplier = FMath::Clamp(SaturationMultiplier, 1.0f, 3.0f);
    if (FMath::IsNearlyEqual(Multiplier, 1.0f))
    {
        return Color;
    }

    float R = Color.R;
    float G = Color.G;
    float B = Color.B;

    const float Max = FMath::Max3(R, G, B);
    const float Min = FMath::Min3(R, G, B);
    const float Delta = Max - Min;

    float H = 0.0f;
    float S = (Max > 0.0f) ? (Delta / Max) : 0.0f;
    const float V = Max;

    if (Delta > 0.0f)
    {
        if (FMath::IsNearlyEqual(Max, R))
        {
            H = 60.0f * FMath::Fmod(((G - B) / Delta), 6.0f);
        }
        else if (FMath::IsNearlyEqual(Max, G))
        {
            H = 60.0f * (((B - R) / Delta) + 2.0f);
        }
        else
        {
            H = 60.0f * (((R - G) / Delta) + 4.0f);
        }

        if (H < 0.0f)
        {
            H += 360.0f;
        }
    }

    S = FMath::Clamp(S * Multiplier, 0.0f, 1.0f);

    const float C = V * S;
    const float X = C * (1.0f - FMath::Abs(FMath::Fmod(H / 60.0f, 2.0f) - 1.0f));
    const float m = V - C;

    float NewR = 0.0f;
    float NewG = 0.0f;
    float NewB = 0.0f;

    if (H < 60.0f)
    {
        NewR = C;
        NewG = X;
        NewB = 0.0f;
    }
    else if (H < 120.0f)
    {
        NewR = X;
        NewG = C;
        NewB = 0.0f;
    }
    else if (H < 180.0f)
    {
        NewR = 0.0f;
        NewG = C;
        NewB = X;
    }
    else if (H < 240.0f)
    {
        NewR = 0.0f;
        NewG = X;
        NewB = C;
    }
    else if (H < 300.0f)
    {
        NewR = X;
        NewG = 0.0f;
        NewB = C;
    }
    else
    {
        NewR = C;
        NewG = 0.0f;
        NewB = X;
    }

    NewR = FMath::Clamp(NewR + m, 0.0f, 1.0f);
    NewG = FMath::Clamp(NewG + m, 0.0f, 1.0f);
    NewB = FMath::Clamp(NewB + m, 0.0f, 1.0f);

    return FLinearColor(NewR, NewG, NewB, Color.A);
}

float UPUIngredientBlueprintLibrary::GetFlavorAspect(const FPUIngredientBase& Ingredient, const FName& AspectName)
{
    return Ingredient.GetFlavorAspect(AspectName);
}

float UPUIngredientBlueprintLibrary::GetTextureAspect(const FPUIngredientBase& Ingredient, const FName& AspectName)
{
    return Ingredient.GetTextureAspect(AspectName);
}

void UPUIngredientBlueprintLibrary::SetFlavorAspect(FPUIngredientBase& Ingredient, const FName& AspectName, float Value)
{
    Ingredient.SetFlavorAspect(AspectName, Value);
}

void UPUIngredientBlueprintLibrary::SetTextureAspect(FPUIngredientBase& Ingredient, const FName& AspectName, float Value)
{
    Ingredient.SetTextureAspect(AspectName, Value);
}

float UPUIngredientBlueprintLibrary::GetTotalFlavorValue(const FPUIngredientBase& Ingredient)
{
    return Ingredient.GetTotalFlavorValue();
}

float UPUIngredientBlueprintLibrary::GetTotalTextureValue(const FPUIngredientBase& Ingredient)
{
    return Ingredient.GetTotalTextureValue();
}

TArray<FGameplayTag> UPUIngredientBlueprintLibrary::GetEffectsAtQuantity(const FPUIngredientBase& Ingredient, int32 Quantity)
{
    return Ingredient.GetEffectsAtQuantity(Quantity);
} 