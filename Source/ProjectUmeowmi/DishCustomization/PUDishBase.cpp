#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"
#include "Engine/DataTable.h"

FPUDishBase::FPUDishBase()
    : DishName(NAME_None)
    , DisplayName(FText::GetEmpty())
    , CustomName(FText::GetEmpty())
    , IngredientDataTable(nullptr)
{
}

bool FPUDishBase::GetIngredient(const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient) const
{
    if (!IngredientDataTable)
    {
        return false;
    }

    // Get the full tag string and split at the last period
    FString FullTag = IngredientTag.ToString();
    int32 LastPeriodIndex;
    if (FullTag.FindLastChar('.', LastPeriodIndex))
    {
        // Get everything after the last period and convert to lowercase
        FString TagName = FullTag.RightChop(LastPeriodIndex + 1).ToLower();
        FName RowName = FName(*TagName);
        
        if (FPUIngredientBase* FoundIngredient = IngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("GetIngredient")))
        {
            OutIngredient = *FoundIngredient;
            return true;
        }
    }
    return false;
}

TArray<FPUIngredientBase> FPUDishBase::GetAllIngredients() const
{
    TArray<FPUIngredientBase> Ingredients;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        FPUIngredientBase Ingredient;
        if (GetIngredient(Instance.IngredientTag, Ingredient))
        {
            Ingredients.Add(Ingredient);
        }
    }
    return Ingredients;
}

float FPUDishBase::GetTotalValueForProperty(const FName& PropertyName) const
{
    float TotalValue = 0.0f;
    
    // Sum up values from all ingredients (including their preparation modifications)
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        FPUIngredientBase Ingredient;
        if (GetIngredient(Instance.IngredientTag, Ingredient))
        {
            TotalValue += Ingredient.GetPropertyValue(PropertyName);
        }
    }
    
    return TotalValue;
}

TArray<FIngredientProperty> FPUDishBase::GetPropertiesWithTag(const FGameplayTag& Tag) const
{
    TArray<FIngredientProperty> Properties;
    
    // Collect properties from all ingredients
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        FPUIngredientBase Ingredient;
        if (GetIngredient(Instance.IngredientTag, Ingredient) && Ingredient.HasPropertiesWithTag(Tag))
        {
            // Add all properties with matching tag
            for (const FIngredientProperty& Property : Ingredient.NaturalProperties)
            {
                if (Property.PropertyTags.HasTag(Tag))
                {
                    Properties.Add(Property);
                }
            }
        }
    }
    
    return Properties;
}

float FPUDishBase::GetTotalValueForTag(const FGameplayTag& Tag) const
{
    float TotalValue = 0.0f;
    
    // Sum up values from all ingredients (including their preparation modifications)
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        FPUIngredientBase Ingredient;
        if (GetIngredient(Instance.IngredientTag, Ingredient))
        {
            TotalValue += Ingredient.GetTotalValueForTag(Tag);
        }
    }
    
    return TotalValue;
}

bool FPUDishBase::HasIngredient(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag)
        {
            return true;
        }
    }
    return false;
}

FText FPUDishBase::GetCurrentDisplayName() const
{
    // If we have a custom name, use it
    if (!CustomName.IsEmpty())
    {
        return CustomName;
    }
    
    // Count occurrences of each ingredient
    TMap<FGameplayTag, int32> IngredientCounts;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        IngredientCounts.FindOrAdd(Instance.IngredientTag)++;
    }

    // Find the most common ingredient
    FGameplayTag MostCommonTag;
    int32 MaxCount = 0;
    for (const auto& Pair : IngredientCounts)
    {
        if (Pair.Value > MaxCount)
        {
            MaxCount = Pair.Value;
            MostCommonTag = Pair.Key;
        }
    }

    // If we have a most common ingredient, get its name and prefix it
    if (MaxCount > 0)
    {
        FPUIngredientBase MostCommonIngredient;
        if (GetIngredient(MostCommonTag, MostCommonIngredient))
        {
            // Find the first instance of the most common ingredient to get its preparations
            for (const FIngredientInstance& Instance : IngredientInstances)
            {
                if (Instance.IngredientTag == MostCommonTag)
                {
                    // Apply the preparations from this instance
                    MostCommonIngredient.ActivePreparations = Instance.Preparations;
                    break;
                }
            }
            
            FString IngredientName = MostCommonIngredient.GetCurrentDisplayName().ToString();
            FString BaseDishName = DisplayName.ToString();
            
            // If the dish name doesn't already start with the ingredient name
            if (!BaseDishName.StartsWith(IngredientName))
            {
                return FText::FromString(IngredientName + " " + BaseDishName);
            }
        }
    }
    
    // If no ingredients or no most common ingredient found, return base dish name
    return DisplayName;
} 