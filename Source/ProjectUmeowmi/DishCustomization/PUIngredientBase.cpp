#include "PUIngredientBase.h"

float FPUIngredientBase::GetPropertyValue(const FName& PropertyName) const
{
    for (const FIngredientProperty& Property : NaturalProperties)
    {
        if (Property.GetPropertyName() == PropertyName)
        {
            return Property.Value;
        }
    }
    return 0.0f;
}

void FPUIngredientBase::SetPropertyValue(const FName& PropertyName, float Value)
{
    for (FIngredientProperty& Property : NaturalProperties)
    {
        if (Property.GetPropertyName() == PropertyName)
        {
            Property.Value = Value;
            return;
        }
    }
    // If property doesn't exist, create it
    FIngredientProperty NewProperty;
    NewProperty.PropertyType = EIngredientPropertyType::Custom;
    NewProperty.CustomPropertyName = PropertyName;
    NewProperty.Value = Value;
    NaturalProperties.Add(NewProperty);
}

bool FPUIngredientBase::HasProperty(const FName& PropertyName) const
{
    for (const FIngredientProperty& Property : NaturalProperties)
    {
        if (Property.GetPropertyName() == PropertyName)
        {
            return true;
        }
    }
    return false;
}

TArray<FIngredientProperty> FPUIngredientBase::GetPropertiesByTag(const FGameplayTag& Tag) const
{
    TArray<FIngredientProperty> MatchingProperties;
    for (const FIngredientProperty& Property : NaturalProperties)
    {
        if (Property.PropertyTags.HasTag(Tag))
        {
            MatchingProperties.Add(Property);
        }
    }
    return MatchingProperties;
}

bool FPUIngredientBase::HasPropertiesWithTag(const FGameplayTag& Tag) const
{
    for (const FIngredientProperty& Property : NaturalProperties)
    {
        if (Property.PropertyTags.HasTag(Tag))
        {
            return true;
        }
    }
    return false;
}

float FPUIngredientBase::GetTotalValueForTag(const FGameplayTag& Tag) const
{
    float TotalValue = 0.0f;
    for (const FIngredientProperty& Property : NaturalProperties)
    {
        if (Property.PropertyTags.HasTag(Tag))
        {
            TotalValue += Property.Value;
        }
    }
    return TotalValue;
}

void FPUIngredientBase::ApplyPreparation(const FName& PreparationName)
{
    // Check if preparation is already applied
    if (CurrentPreparations.Contains(PreparationName))
    {
        return;
    }

    // Find the preparation modifier
    for (const FPreparationModifier& Modifier : AvailablePreparations)
    {
        if (Modifier.ModifierName == PreparationName)
        {
            // Apply the preparation
            CurrentPreparations.Add(PreparationName);

            // Apply property modifiers
            for (const auto& PropertyModifier : Modifier.PropertyModifiers)
            {
                float CurrentValue = GetPropertyValue(PropertyModifier.Key);
                SetPropertyValue(PropertyModifier.Key, CurrentValue + PropertyModifier.Value);
            }
            return;
        }
    }
}

void FPUIngredientBase::RemovePreparation(const FName& PreparationName)
{
    // Check if preparation is applied
    if (!CurrentPreparations.Contains(PreparationName))
    {
        return;
    }

    // Find the preparation modifier
    for (const FPreparationModifier& Modifier : AvailablePreparations)
    {
        if (Modifier.ModifierName == PreparationName)
        {
            // Remove the preparation
            CurrentPreparations.Remove(PreparationName);

            // Remove property modifiers
            for (const auto& PropertyModifier : Modifier.PropertyModifiers)
            {
                float CurrentValue = GetPropertyValue(PropertyModifier.Key);
                SetPropertyValue(PropertyModifier.Key, CurrentValue - PropertyModifier.Value);
            }
            return;
        }
    }
}

TArray<FName> FPUIngredientBase::GetAvailablePreparations() const
{
    TArray<FName> AvailableNames;
    for (const FPreparationModifier& Modifier : AvailablePreparations)
    {
        AvailableNames.Add(Modifier.ModifierName);
    }
    return AvailableNames;
}

TArray<FGameplayTag> FPUIngredientBase::GetEffectsAtQuantity(int32 Quantity) const
{
    TArray<FGameplayTag> ActiveEffects;
    for (const auto& EffectPair : QuantitySpecialEffects)
    {
        if (Quantity >= EffectPair.Key)
        {
            // Convert GameplayTagContainer to array of tags
            TArray<FGameplayTag> Tags;
            EffectPair.Value.GetGameplayTagArray(Tags);
            ActiveEffects.Append(Tags);
        }
    }
    return ActiveEffects;
} 