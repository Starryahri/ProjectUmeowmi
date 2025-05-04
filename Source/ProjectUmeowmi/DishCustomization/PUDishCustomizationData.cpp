#include "PUDishCustomizationData.h"
#include "Engine/DataTable.h"

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

bool UPUDishCustomizationHelper::LoadIngredient(const UDataTable* IngredientDataTable, const FName& IngredientName, FPUIngredientBase& OutIngredientData)
{
    if (!IngredientDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Ingredient Data Table"));
        return false;
    }

    const FPUIngredientBase* FoundRow = IngredientDataTable->FindRow<FPUIngredientBase>(IngredientName, TEXT("LoadIngredient"));
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
    for (const FPUIngredientBase& Ingredient : DishData.AvailableIngredients)
    {
        FPUIngredientBase LoadedIngredient;
        if (LoadIngredient(IngredientDataTable, Ingredient.IngredientName, LoadedIngredient))
        {
            DishData.AvailableIngredients.Add(LoadedIngredient);
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

    // Check if all current ingredients are valid
    for (const FPUIngredientBase& Ingredient : DishData.CurrentIngredients)
    {
        if (Ingredient.CurrentQuantity < Ingredient.MinQuantity || Ingredient.CurrentQuantity > Ingredient.MaxQuantity)
        {
            return false;
        }
    }

    return true;
}

TArray<FGameplayTag> UPUDishCustomizationHelper::GetActiveSpecialEffects(const FPUIngredientBase& IngredientData)
{
    return IngredientData.GetEffectsAtQuantity(IngredientData.CurrentQuantity);
}

void UPUDishCustomizationHelper::ResetDishToDefault(FDishCustomizationData& DishData)
{
    DishData.CurrentIngredients.Empty();
}

TArray<FName> UPUDishCustomizationHelper::GetAvailableIngredients(const FDishCustomizationData& DishData)
{
    TArray<FName> AvailableNames;
    for (const FPUIngredientBase& Ingredient : DishData.AvailableIngredients)
    {
        AvailableNames.Add(Ingredient.IngredientName);
    }
    return AvailableNames;
}

bool UPUDishCustomizationHelper::CanAddIngredient(const FDishCustomizationData& DishData, const FName& IngredientName)
{
    // Check if ingredient is available
    bool bIsAvailable = false;
    for (const FPUIngredientBase& Ingredient : DishData.AvailableIngredients)
    {
        if (Ingredient.IngredientName == IngredientName)
        {
            bIsAvailable = true;
            break;
        }
    }

    if (!bIsAvailable)
    {
        return false;
    }

    // Check if we can add more ingredients
    return GetTotalIngredientCount(DishData) < DishData.MaxTotalIngredients;
}

int32 UPUDishCustomizationHelper::GetTotalIngredientCount(const FDishCustomizationData& DishData)
{
    int32 TotalCount = 0;
    for (const FPUIngredientBase& Ingredient : DishData.CurrentIngredients)
    {
        TotalCount += Ingredient.CurrentQuantity;
    }
    return TotalCount;
}

TSoftObjectPtr<UTexture2D> UPUDishCustomizationHelper::GetCurrentDishImage(const FDishCustomizationData& DishData)
{
    int32 TotalIngredients = GetTotalIngredientCount(DishData);
    if (DishData.DishImages.IsValidIndex(TotalIngredients))
    {
        return TSoftObjectPtr<UTexture2D>(DishData.DishImages[TotalIngredients]);
    }
    return nullptr;
} 