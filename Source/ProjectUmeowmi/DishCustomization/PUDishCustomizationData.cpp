#include "PUDishCustomizationData.h"

bool UPUDishCustomizationHelper::LoadDishTemplate(const UDataTable* DishDataTable, const FName& DishName, FDishCustomizationData& OutDishData)
{
    if (!DishDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Dish Data Table"));
        return false;
    }

    const FDishCustomizationData* FoundRow = DishDataTable->FindRow<FDishCustomizationData>(DishName, TEXT("LoadDishTemplate"));
    if (!FoundRow)
    {
        UE_LOG(LogTemp, Error, TEXT("Dish template not found: %s"), *DishName.ToString());
        return false;
    }

    OutDishData = *FoundRow;
    return true;
}

bool UPUDishCustomizationHelper::LoadIngredient(const UDataTable* IngredientDataTable, const FName& IngredientName, FIngredientData& OutIngredientData)
{
    if (!IngredientDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Ingredient Data Table"));
        return false;
    }

    const FIngredientData* FoundRow = IngredientDataTable->FindRow<FIngredientData>(IngredientName, TEXT("LoadIngredient"));
    if (!FoundRow)
    {
        UE_LOG(LogTemp, Error, TEXT("Ingredient not found: %s"), *IngredientName.ToString());
        return false;
    }

    OutIngredientData = *FoundRow;
    return true;
}

bool UPUDishCustomizationHelper::PopulateDishIngredients(const UDataTable* IngredientDataTable, FDishCustomizationData& DishData)
{
    if (!IngredientDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Ingredient Data Table"));
        return false;
    }

    DishData.AvailableIngredients.Empty();
    for (const FName& IngredientName : DishData.AvailableIngredientNames)
    {
        FIngredientData IngredientData;
        if (LoadIngredient(IngredientDataTable, IngredientName, IngredientData))
        {
            DishData.AvailableIngredients.Add(IngredientData);
        }
    }

    return true;
}

bool UPUDishCustomizationHelper::ValidateDishCustomization(const FDishCustomizationData& DishData)
{
    // Check if total ingredients exceed maximum
    int32 TotalIngredients = GetTotalIngredientCount(DishData);
    if (TotalIngredients > DishData.MaxTotalIngredients)
    {
        return false;
    }

    // Check if any ingredient exceeds its maximum quantity
    for (const FIngredientData& Ingredient : DishData.AvailableIngredients)
    {
        if (Ingredient.CurrentQuantity > Ingredient.MaxQuantity)
        {
            return false;
        }
    }

    return true;
}

TArray<FName> UPUDishCustomizationHelper::GetActiveSpecialEffects(const FIngredientData& IngredientData)
{
    TArray<FName> ActiveEffects;
    
    for (const auto& EffectPair : IngredientData.QuantitySpecialEffects)
    {
        if (IngredientData.CurrentQuantity >= EffectPair.Key)
        {
            ActiveEffects.Add(EffectPair.Value);
        }
    }

    return ActiveEffects;
}

void UPUDishCustomizationHelper::ResetDishToDefault(FDishCustomizationData& DishData)
{
    for (FIngredientData& Ingredient : DishData.AvailableIngredients)
    {
        Ingredient.CurrentQuantity = Ingredient.MinQuantity;
    }
}

TArray<FName> UPUDishCustomizationHelper::GetAvailableIngredients(const FDishCustomizationData& DishData)
{
    return DishData.AvailableIngredientNames;
}

bool UPUDishCustomizationHelper::CanAddIngredient(const FDishCustomizationData& DishData, const FName& IngredientName)
{
    // Check if ingredient is available for this dish
    if (!DishData.AvailableIngredientNames.Contains(IngredientName))
    {
        return false;
    }

    // Check if adding would exceed total ingredient limit
    int32 CurrentTotal = GetTotalIngredientCount(DishData);
    if (CurrentTotal >= DishData.MaxTotalIngredients)
    {
        return false;
    }

    return true;
}

int32 UPUDishCustomizationHelper::GetTotalIngredientCount(const FDishCustomizationData& DishData)
{
    int32 Total = 0;
    for (const FIngredientData& Ingredient : DishData.AvailableIngredients)
    {
        Total += Ingredient.CurrentQuantity;
    }
    return Total;
}

TSoftObjectPtr<UTexture2D> UPUDishCustomizationHelper::GetCurrentDishImage(const FDishCustomizationData& DishData)
{
    int32 TotalIngredients = GetTotalIngredientCount(DishData);

    // Clamp the total to the available images
    int32 ImageIndex = FMath::Clamp(TotalIngredients, 0, DishData.DishImages.Num() - 1);
    return DishData.DishImages[ImageIndex];
} 