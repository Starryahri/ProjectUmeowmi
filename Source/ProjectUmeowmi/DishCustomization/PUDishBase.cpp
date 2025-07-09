#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"
#include "Engine/DataTable.h"

// Initialize the static counter
std::atomic<int32> FPUDishBase::GlobalInstanceCounter(0);

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
    if (!IngredientDataTable.IsValid())
    {
        return false;
    }
    
    UDataTable* LoadedIngredientDataTable = IngredientDataTable.LoadSynchronous();
    if (!LoadedIngredientDataTable)
    {
        return false;
    }
    
    // Get the ingredient name from the tag (everything after the last period) and convert to lowercase
    FString FullTag = IngredientTag.ToString();
    int32 LastPeriodIndex;
    if (FullTag.FindLastChar('.', LastPeriodIndex))
    {
        FString IngredientName = FullTag.RightChop(LastPeriodIndex + 1).ToLower();
        FName RowName = FName(*IngredientName);
        
        if (FPUIngredientBase* FoundIngredient = LoadedIngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("GetIngredient")))
        {
            OutIngredient = *FoundIngredient;
            
            // Load preparation data if available
            if (OutIngredient.PreparationDataTable.IsValid())
            {
                UDataTable* LoadedPreparationDataTable = OutIngredient.PreparationDataTable.LoadSynchronous();
                if (LoadedPreparationDataTable)
                {
                    // Apply any active preparations to the ingredient
                    for (const FGameplayTag& PrepTag : OutIngredient.ActivePreparations)
                    {
                        // Get the preparation name from the tag (everything after the last period) and convert to lowercase
                        FString PrepFullTag = PrepTag.ToString();
                        int32 PrepLastPeriodIndex;
                        if (PrepFullTag.FindLastChar('.', PrepLastPeriodIndex))
                        {
                            FString PrepName = PrepFullTag.RightChop(PrepLastPeriodIndex + 1).ToLower();
                            FName PrepRowName = FName(*PrepName);
                            
                            if (FPUPreparationBase* Preparation = LoadedPreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetIngredient")))
                            {
                                // Apply preparation modifiers
                                for (const FPropertyModifier& Modifier : Preparation->PropertyModifiers)
                                {
                                    FName PropertyName = Modifier.GetPropertyName();
                                    float CurrentValue = OutIngredient.GetPropertyValue(PropertyName);
                                    float NewValue = Modifier.ApplyModification(CurrentValue);
                                    OutIngredient.SetPropertyValue(PropertyName, NewValue);
                                }
                            }
                        }
                    }
                }
            }
            
            return true;
        }
    }
    
    return false;
}

bool FPUDishBase::GetIngredientForInstance(int32 InstanceIndex, FPUIngredientBase& OutIngredient) const
{
    if (!IngredientInstances.IsValidIndex(InstanceIndex))
    {
        return false;
    }

    // Simply return the ingredient data that's already stored in the instance
    OutIngredient = IngredientInstances[InstanceIndex].IngredientData;
    return true;
}

TArray<FPUIngredientBase> FPUDishBase::GetAllIngredients() const
{
    TArray<FPUIngredientBase> Ingredients;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        Ingredients.Add(Instance.IngredientData);
    }
    return Ingredients;
}

TArray<FIngredientInstance> FPUDishBase::GetAllIngredientInstances() const
{
    return IngredientInstances;
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
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        // Multiply the property value by the quantity of this ingredient
        TotalValue += Instance.IngredientData.GetPropertyValue(PropertyName) * Instance.Quantity;
    }
    
    return TotalValue;
}

TArray<FIngredientProperty> FPUDishBase::GetPropertiesWithTag(const FGameplayTag& Tag) const
{
    TArray<FIngredientProperty> Properties;
    
    // Collect properties from all ingredients
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        if (Instance.IngredientData.HasPropertiesWithTag(Tag))
        {
            // Add all properties with matching tag, multiplied by quantity
            for (const FIngredientProperty& Property : Instance.IngredientData.NaturalProperties)
            {
                if (Property.PropertyTags.HasTag(Tag))
                {
                    // Create a copy of the property with the value multiplied by quantity
                    FIngredientProperty QuantityAdjustedProperty = Property;
                    QuantityAdjustedProperty.Value = Property.Value * Instance.Quantity;
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
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        // Multiply the total value for the tag by the quantity of this ingredient
        TotalValue += Instance.IngredientData.GetTotalValueForTag(Tag) * Instance.Quantity;
    }
    
    return TotalValue;
}

bool FPUDishBase::HasIngredient(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        // Check both the convenient field and the data field for compatibility
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        if (InstanceTag == IngredientTag)
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
        // Use convenient field if available, fallback to data field
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        IngredientQuantities.FindOrAdd(InstanceTag) += Instance.Quantity;
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
                // Use convenient field if available, fallback to data field
                FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
                if (InstanceTag == MostCommonTag)
                {
                    // Apply the preparations from this instance (prefer convenient field, fallback to data field)
                    MostCommonIngredient.ActivePreparations = Instance.Preparations.Num() > 0 ? Instance.Preparations : Instance.IngredientData.ActivePreparations;
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

int32 FPUDishBase::FindInstanceIndexByID(int32 InstanceID) const
{
    for (int32 i = 0; i < IngredientInstances.Num(); ++i)
    {
        if (IngredientInstances[i].InstanceID == InstanceID)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 FPUDishBase::GenerateNewInstanceID() const
{
    int32 MaxID = 0;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        MaxID = FMath::Max(MaxID, Instance.InstanceID);
    }
    return MaxID + 1;
}

int32 FPUDishBase::GenerateUniqueInstanceID()
{
    return ++GlobalInstanceCounter;
}

bool FPUDishBase::GetIngredientForInstanceID(int32 InstanceID, FPUIngredientBase& OutIngredient) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        OutIngredient = IngredientInstances[InstanceIndex].IngredientData;
        return true;
    }
    return false;
}

bool FPUDishBase::GetIngredientInstanceByID(int32 InstanceID, FIngredientInstance& OutInstance) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        OutInstance = IngredientInstances[InstanceIndex];
        return true;
    }
    return false;
}

FGameplayTag FPUDishBase::GetIngredientTag(int32 InstanceID) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        const FIngredientInstance& Instance = IngredientInstances[InstanceIndex];
        // Use convenient field if available, fallback to data field
        return Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
    }
    return FGameplayTag();
}

FGameplayTagContainer FPUDishBase::GetPreparations(int32 InstanceID) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        const FIngredientInstance& Instance = IngredientInstances[InstanceIndex];
        // Use convenient field if available, fallback to data field
        return Instance.Preparations.Num() > 0 ? Instance.Preparations : Instance.IngredientData.ActivePreparations;
    }
    return FGameplayTagContainer();
}

int32 FPUDishBase::GetQuantity(int32 InstanceID) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        return IngredientInstances[InstanceIndex].Quantity;
    }
    return 0;
} 