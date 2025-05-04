#include "PUPreparationBase.h"
#include "GameplayTagsManager.h"

bool FPUPreparationBase::CanApplyToIngredient(const FGameplayTagContainer& IngredientTags) const
{
    // Check if ingredient has incompatible tags
    if (IngredientTags.HasAny(IncompatibleTags))
    {
        return false;
    }

    // Check if ingredient has required tags
    if (!IngredientTags.HasAll(RequiredTags))
    {
        return false;
    }

    return true;
}

FText FPUPreparationBase::GetModifiedName(const FText& BaseName) const
{
    if (OverridesBaseName)
    {
        return SpecialName;
    }

    FString Result;
    if (!NamePrefix.IsEmpty())
    {
        Result += NamePrefix.ToString() + " ";
    }
    
    Result += BaseName.ToString();
    
    if (!NameSuffix.IsEmpty())
    {
        Result += " " + NameSuffix.ToString();
    }

    return FText::FromString(Result);
}

void FPUPreparationBase::ApplyModifiers(TArray<FIngredientProperty>& Properties) const
{
    for (const FPropertyModifier& Modifier : PropertyModifiers)
    {
        FName PropertyName = Modifier.GetPropertyName();
        
        // Find or create the property
        bool bFound = false;
        for (FIngredientProperty& Property : Properties)
        {
            if (Property.GetPropertyName() == PropertyName)
            {
                Property.Value += Modifier.ValueChange;
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            FIngredientProperty NewProperty;
            NewProperty.PropertyType = Modifier.PropertyType;
            if (Modifier.PropertyType == EIngredientPropertyType::Custom)
            {
                NewProperty.CustomPropertyName = Modifier.CustomPropertyName;
            }
            NewProperty.Value = Modifier.ValueChange;
            NewProperty.Description = Modifier.Description;
            NewProperty.PropertyTags = Modifier.ModifierTags;
            Properties.Add(NewProperty);
        }
    }
}

void FPUPreparationBase::RemoveModifiers(TArray<FIngredientProperty>& Properties) const
{
    for (const FPropertyModifier& Modifier : PropertyModifiers)
    {
        FName PropertyName = Modifier.GetPropertyName();
        
        for (FIngredientProperty& Property : Properties)
        {
            if (Property.GetPropertyName() == PropertyName)
            {
                Property.Value -= Modifier.ValueChange;
                break;
            }
        }
    }
} 