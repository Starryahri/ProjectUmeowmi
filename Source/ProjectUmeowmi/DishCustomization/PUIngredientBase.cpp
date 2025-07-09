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
    // If we have active preparations, try to get a modified name
    if (ActivePreparations.Num() > 0 && PreparationDataTable.IsValid())
    {
        UDataTable* LoadedPreparationDataTable = PreparationDataTable.LoadSynchronous();
        if (LoadedPreparationDataTable)
        {
            // Get the first preparation to modify the name
            TArray<FGameplayTag> PrepTags;
            ActivePreparations.GetGameplayTagArray(PrepTags);
            
            if (PrepTags.Num() > 0)
            {
                // Get the preparation name from the tag (everything after the last period) and convert to lowercase
                FString PrepFullTag = PrepTags[0].ToString();
                int32 PrepLastPeriodIndex;
                if (PrepFullTag.FindLastChar('.', PrepLastPeriodIndex))
                {
                    FString PrepName = PrepFullTag.RightChop(PrepLastPeriodIndex + 1).ToLower();
                    FName PrepRowName = FName(*PrepName);
                    
                    if (FPUPreparationBase* Preparation = LoadedPreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetCurrentDisplayName")))
                    {
                        // If the preparation overrides the base name, use the special name
                        if (Preparation->OverridesBaseName)
                        {
                            return Preparation->SpecialName;
                        }
                        
                        // Otherwise, apply prefix and suffix
                        FString ModifiedName = Preparation->NamePrefix.ToString() + DisplayName.ToString() + Preparation->NameSuffix.ToString();
                        return FText::FromString(ModifiedName);
                    }
                }
            }
        }
    }
    
    // Return the base display name if no preparations or no data table
    return DisplayName;
} 