#include "PUIngredientBase.h"
#include "Engine/DataTable.h"
#include "GameplayTagsManager.h"
#include "PUPreparationBase.h"

FPUIngredientBase::FPUIngredientBase()
    : IngredientName(NAME_None)
    , DisplayName(FText::GetEmpty())
    , MinQuantity(0)
    , MaxQuantity(5)
    , CurrentQuantity(0)
    , PreparationDataTable(nullptr)
    , PreviewTexture(nullptr)
    , MaterialInstance(nullptr)
    , IngredientMesh(nullptr)
{
}

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

TArray<FGameplayTag> FPUIngredientBase::GetEffectsAtQuantity(int32 Quantity) const
{
    TArray<FGameplayTag> Effects;
    if (QuantitySpecialEffects.Contains(Quantity))
    {
        const FGameplayTagContainer& Tags = QuantitySpecialEffects[Quantity];
        Tags.GetGameplayTagArray(Effects);
    }
    return Effects;
}

bool FPUIngredientBase::ApplyPreparation(const FPUPreparationBase& Preparation)
{
    // Check if preparation is already applied
    if (ActivePreparations.HasTag(Preparation.PreparationTag))
    {
        return false;
    }

    // Check if preparation can be applied
    if (!Preparation.CanApplyToIngredient(ActivePreparations))
    {
        return false;
    }

    // Apply the preparation
    ActivePreparations.AddTag(Preparation.PreparationTag);
    Preparation.ApplyModifiers(NaturalProperties);
    return true;
}

bool FPUIngredientBase::RemovePreparation(const FPUPreparationBase& Preparation)
{
    // Check if preparation is actually applied
    if (!ActivePreparations.HasTag(Preparation.PreparationTag))
    {
        return false;
    }

    // Remove the preparation
    ActivePreparations.RemoveTag(Preparation.PreparationTag);
    Preparation.RemoveModifiers(NaturalProperties);
    return true;
}

bool FPUIngredientBase::HasPreparation(const FGameplayTag& PreparationTag) const
{
    return ActivePreparations.HasTag(PreparationTag);
}

FText FPUIngredientBase::GetCurrentDisplayName() const
{
    FText CurrentName = DisplayName;
    
    if (!PreparationDataTable)
    {
        return CurrentName;
    }
    
    // Get all active preparation tags
    TArray<FGameplayTag> ActiveTags;
    ActivePreparations.GetGameplayTagArray(ActiveTags);
    
    // Sort preparations by their tag to ensure consistent order
    ActiveTags.Sort([](const FGameplayTag& A, const FGameplayTag& B) {
        return A.ToString() < B.ToString();
    });
    
    // Apply name modifications in order
    for (const FGameplayTag& Tag : ActiveTags)
    {
        // Get the full tag string and split at the last period
        FString FullTag = Tag.ToString();
        int32 LastPeriodIndex;
        if (FullTag.FindLastChar('.', LastPeriodIndex))
        {
            // Get everything after the last period and convert to lowercase
            FString TagName = FullTag.RightChop(LastPeriodIndex + 1).ToLower();
            FName RowName = FName(*TagName);
            FPUPreparationBase* Preparation = PreparationDataTable->FindRow<FPUPreparationBase>(RowName, TEXT("GetCurrentDisplayName"));
            
            if (Preparation)
            {
                CurrentName = Preparation->GetModifiedName(CurrentName);
            }
        }
    }
    
    return CurrentName;
} 