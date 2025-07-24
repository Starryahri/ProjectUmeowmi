#include "PUIngredientBase.h"
#include "Engine/DataTable.h"
#include "GameplayTagsManager.h"
#include "PUPreparationBase.h"

FPUIngredientBase::FPUIngredientBase()
    : IngredientName(NAME_None)
    , DisplayName(FText::GetEmpty())
    , PreviewTexture(nullptr)
    , MaterialInstance(nullptr)
    , IngredientMesh(nullptr)
    , MinQuantity(0)
    , MaxQuantity(5)
    , CurrentQuantity(0)
    , PreparationDataTable(nullptr)
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
            // Get all preparation tags
            TArray<FGameplayTag> PrepTags;
            ActivePreparations.GetGameplayTagArray(PrepTags);
            
            if (PrepTags.Num() > 0)
            {
                // If more than 2 preparations, apply "Dubious" prefix instead of combining prefixes/suffixes
                if (PrepTags.Num() > 2)
                {
                    FString ModifiedName = TEXT("Dubious ");
                    ModifiedName += DisplayName.ToString();
                    return FText::FromString(ModifiedName);
                }
                
                FString CombinedPrefix;
                FString CombinedSuffix;
                FString SpecialOverrideName;
                bool bHasSpecialOverride = false;
                
                // Process all preparations to combine their prefixes and suffixes
                for (const FGameplayTag& PrepTag : PrepTags)
                {
                    // Get the preparation name from the tag (everything after the last period) and convert to lowercase
                    FString PrepFullTag = PrepTag.ToString();
                    int32 PrepLastPeriodIndex;
                    if (PrepFullTag.FindLastChar('.', PrepLastPeriodIndex))
                    {
                        FString PrepName = PrepFullTag.RightChop(PrepLastPeriodIndex + 1).ToLower();
                        FName PrepRowName = FName(*PrepName);
                        
                        if (FPUPreparationBase* Preparation = LoadedPreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetCurrentDisplayName")))
                        {
                            // If any preparation overrides the base name, use the special name
                            if (Preparation->OverridesBaseName)
                            {
                                SpecialOverrideName = Preparation->SpecialName.ToString();
                                bHasSpecialOverride = true;
                                break; // Special override takes precedence, stop processing
                            }
                            
                            // Combine prefixes and suffixes
                            if (!Preparation->NamePrefix.IsEmpty())
                            {
                                if (!CombinedPrefix.IsEmpty())
                                {
                                    CombinedPrefix += " ";
                                }
                                CombinedPrefix += Preparation->NamePrefix.ToString();
                            }
                            
                            if (!Preparation->NameSuffix.IsEmpty())
                            {
                                if (!CombinedSuffix.IsEmpty())
                                {
                                    CombinedSuffix = " " + CombinedSuffix;
                                }
                                CombinedSuffix = Preparation->NameSuffix.ToString() + CombinedSuffix;
                            }
                        }
                    }
                }
                
                // Return the appropriate modified name
                if (bHasSpecialOverride)
                {
                    return FText::FromString(SpecialOverrideName);
                }
                else if (!CombinedPrefix.IsEmpty() || !CombinedSuffix.IsEmpty())
                {
                    FString ModifiedName = CombinedPrefix;
                    if (!CombinedPrefix.IsEmpty())
                    {
                        ModifiedName += " ";
                    }
                    ModifiedName += DisplayName.ToString();
                    if (!CombinedSuffix.IsEmpty())
                    {
                        ModifiedName += " " + CombinedSuffix;
                    }
                    return FText::FromString(ModifiedName);
                }
            }
        }
    }
    
    // Return the base display name if no preparations or no data table
    return DisplayName;
} 