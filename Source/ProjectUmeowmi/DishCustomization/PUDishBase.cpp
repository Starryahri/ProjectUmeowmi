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

// realizing that get ingredient and logically the get all ingredient is not applying the preparations when called here.
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
            // Create a copy of the base ingredient
            OutIngredient = *FoundIngredient;

            // Find the first instance of this ingredient to get its preparations
            for (const FIngredientInstance& Instance : IngredientInstances)
            {
                if (Instance.IngredientTag == IngredientTag)
                {
                    // Apply the preparations from this instance
                    OutIngredient.ActivePreparations = Instance.Preparations;

                    // Apply each preparation's modifiers
                    if (OutIngredient.PreparationDataTable)
                    {
                        TArray<FGameplayTag> PreparationTags;
                        Instance.Preparations.GetGameplayTagArray(PreparationTags);

                        for (const FGameplayTag& PrepTag : PreparationTags)
                        {
                            // Get the preparation data
                            FString PrepTagName = PrepTag.ToString();
                            if (PrepTagName.FindLastChar('.', LastPeriodIndex))
                            {
                                FString PrepName = PrepTagName.RightChop(LastPeriodIndex + 1).ToLower();
                                FName PrepRowName = FName(*PrepName);
                                
                                if (FPUPreparationBase* Preparation = OutIngredient.PreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetIngredient")))
                                {
                                    // Apply the preparation's modifiers to the ingredient's properties
                                    Preparation->ApplyModifiers(OutIngredient.NaturalProperties);
                                }
                            }
                        }
                    }
                    break;
                }
            }
            return true;
        }
    }
    return false;
}

bool FPUDishBase::GetIngredientForInstance(int32 InstanceIndex, FPUIngredientBase& OutIngredient) const
{
    if (!IngredientDataTable || !IngredientInstances.IsValidIndex(InstanceIndex))
    {
        return false;
    }

    const FIngredientInstance& Instance = IngredientInstances[InstanceIndex];
    
    // Get the full tag string and split at the last period
    FString FullTag = Instance.IngredientTag.ToString();
    int32 LastPeriodIndex;
    if (FullTag.FindLastChar('.', LastPeriodIndex))
    {
        // Get everything after the last period and convert to lowercase
        FString TagName = FullTag.RightChop(LastPeriodIndex + 1).ToLower();
        FName RowName = FName(*TagName);
        
        if (FPUIngredientBase* FoundIngredient = IngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("GetIngredientForInstance")))
        {
            // Create a copy of the base ingredient
            OutIngredient = *FoundIngredient;

            // Apply the preparations from this specific instance
            OutIngredient.ActivePreparations = Instance.Preparations;

            // Apply each preparation's modifiers
            if (OutIngredient.PreparationDataTable)
            {
                TArray<FGameplayTag> PreparationTags;
                Instance.Preparations.GetGameplayTagArray(PreparationTags);

                for (const FGameplayTag& PrepTag : PreparationTags)
                {
                    // Get the preparation data
                    FString PrepTagName = PrepTag.ToString();
                    if (PrepTagName.FindLastChar('.', LastPeriodIndex))
                    {
                        FString PrepName = PrepTagName.RightChop(LastPeriodIndex + 1).ToLower();
                        FName PrepRowName = FName(*PrepName);
                        
                        if (FPUPreparationBase* Preparation = OutIngredient.PreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetIngredientForInstance")))
                        {
                            // Apply the preparation's modifiers to the ingredient's properties
                            Preparation->ApplyModifiers(OutIngredient.NaturalProperties);
                        }
                    }
                }
            }
            return true;
        }
    }
    return false;
}

TArray<FPUIngredientBase> FPUDishBase::GetAllIngredients() const
{
    TArray<FPUIngredientBase> Ingredients;
    for (int32 i = 0; i < IngredientInstances.Num(); ++i)
    {
        FPUIngredientBase Ingredient;
        if (GetIngredientForInstance(i, Ingredient))
        {
            Ingredients.Add(Ingredient);
        }
    }
    return Ingredients;
}

int32 FPUDishBase::GetTotalIngredientQuantity() const
{
    int32 TotalQuantity = 0;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        TotalQuantity += Instance.Quantity;
    }
    return TotalQuantity;
}

float FPUDishBase::GetTotalValueForProperty(const FName& PropertyName) const
{
    float TotalValue = 0.0f;
    
    // Sum up values from all ingredients (including their preparation modifications)
    for (int32 i = 0; i < IngredientInstances.Num(); ++i)
    {
        FPUIngredientBase Ingredient;
        if (GetIngredientForInstance(i, Ingredient))
        {
            // Multiply the property value by the quantity of this ingredient
            TotalValue += Ingredient.GetPropertyValue(PropertyName) * IngredientInstances[i].Quantity;
        }
    }
    
    return TotalValue;
}

TArray<FIngredientProperty> FPUDishBase::GetPropertiesWithTag(const FGameplayTag& Tag) const
{
    TArray<FIngredientProperty> Properties;
    
    // Collect properties from all ingredients
    for (int32 i = 0; i < IngredientInstances.Num(); ++i)
    {
        FPUIngredientBase Ingredient;
        if (GetIngredientForInstance(i, Ingredient) && Ingredient.HasPropertiesWithTag(Tag))
        {
            // Add all properties with matching tag, multiplied by quantity
            for (const FIngredientProperty& Property : Ingredient.NaturalProperties)
            {
                if (Property.PropertyTags.HasTag(Tag))
                {
                    // Create a copy of the property with the value multiplied by quantity
                    FIngredientProperty QuantityAdjustedProperty = Property;
                    QuantityAdjustedProperty.Value = Property.Value * IngredientInstances[i].Quantity;
                    Properties.Add(QuantityAdjustedProperty);
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
    for (int32 i = 0; i < IngredientInstances.Num(); ++i)
    {
        FPUIngredientBase Ingredient;
        if (GetIngredientForInstance(i, Ingredient))
        {
            // Multiply the total value for the tag by the quantity of this ingredient
            TotalValue += Ingredient.GetTotalValueForTag(Tag) * IngredientInstances[i].Quantity;
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
    
    // Track quantities of each ingredient
    TMap<FGameplayTag, int32> IngredientQuantities;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        IngredientQuantities.FindOrAdd(Instance.IngredientTag) += Instance.Quantity;
    }

    // Find the ingredient with the highest quantity
    FGameplayTag MostCommonTag;
    int32 MaxQuantity = 0;
    for (const auto& Pair : IngredientQuantities)
    {
        if (Pair.Value > MaxQuantity)
        {
            MaxQuantity = Pair.Value;
            MostCommonTag = Pair.Key;
        }
    }

    // If we have an ingredient with the highest quantity, get its name and prefix it
    if (MaxQuantity > 0)
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