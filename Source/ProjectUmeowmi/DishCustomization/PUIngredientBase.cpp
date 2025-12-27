#include "PUIngredientBase.h"
#include "Engine/DataTable.h"
#include "GameplayTagsManager.h"
#include "PUPreparationBase.h"

FPUIngredientBase::FPUIngredientBase()
    : IngredientName(NAME_None)
    , DisplayName(FText::GetEmpty())
    , PreviewTexture(nullptr)
    , PantryTexture(nullptr)
    , MaterialInstance(nullptr)
    , IngredientMesh(nullptr)
    , MinQuantity(0)
    , MaxQuantity(5)
    , CurrentQuantity(0)
    , PreparationDataTable(nullptr)
{
}

float FPUIngredientBase::GetFlavorAspect(const FName& AspectName) const
{
    FString AspectStr = AspectName.ToString().ToLower();
    
    if (AspectStr == TEXT("umami"))
        return FlavorAspects.Umami;
    else if (AspectStr == TEXT("sweet"))
        return FlavorAspects.Sweet;
    else if (AspectStr == TEXT("salt"))
        return FlavorAspects.Salt;
    else if (AspectStr == TEXT("sour"))
        return FlavorAspects.Sour;
    else if (AspectStr == TEXT("bitter"))
        return FlavorAspects.Bitter;
    else if (AspectStr == TEXT("spicy"))
        return FlavorAspects.Spicy;
    
    return 0.0f;
}

float FPUIngredientBase::GetTextureAspect(const FName& AspectName) const
{
    FString AspectStr = AspectName.ToString().ToLower();
    
    if (AspectStr == TEXT("rich"))
        return TextureAspects.Rich;
    else if (AspectStr == TEXT("juicy"))
        return TextureAspects.Juicy;
    else if (AspectStr == TEXT("tender"))
        return TextureAspects.Tender;
    else if (AspectStr == TEXT("chewy"))
        return TextureAspects.Chewy;
    else if (AspectStr == TEXT("crispy"))
        return TextureAspects.Crispy;
    else if (AspectStr == TEXT("crumbly"))
        return TextureAspects.Crumbly;
    
    return 0.0f;
}

void FPUIngredientBase::SetFlavorAspect(const FName& AspectName, float Value)
{
    // Clamp value to 0.0-5.0 range and round to nearest 0.5 increment
    Value = FMath::Clamp(Value, 0.0f, 5.0f);
    Value = FMath::RoundToFloat(Value * 2.0f) / 2.0f; // Round to nearest 0.5
    
    FString AspectStr = AspectName.ToString().ToLower();
    
    if (AspectStr == TEXT("umami"))
        FlavorAspects.Umami = Value;
    else if (AspectStr == TEXT("sweet"))
        FlavorAspects.Sweet = Value;
    else if (AspectStr == TEXT("salt"))
        FlavorAspects.Salt = Value;
    else if (AspectStr == TEXT("sour"))
        FlavorAspects.Sour = Value;
    else if (AspectStr == TEXT("bitter"))
        FlavorAspects.Bitter = Value;
    else if (AspectStr == TEXT("spicy"))
        FlavorAspects.Spicy = Value;
}

void FPUIngredientBase::SetTextureAspect(const FName& AspectName, float Value)
{
    // Clamp value to 0.0-5.0 range and round to nearest 0.5 increment
    Value = FMath::Clamp(Value, 0.0f, 5.0f);
    Value = FMath::RoundToFloat(Value * 2.0f) / 2.0f; // Round to nearest 0.5
    
    FString AspectStr = AspectName.ToString().ToLower();
    
    if (AspectStr == TEXT("rich"))
        TextureAspects.Rich = Value;
    else if (AspectStr == TEXT("juicy"))
        TextureAspects.Juicy = Value;
    else if (AspectStr == TEXT("tender"))
        TextureAspects.Tender = Value;
    else if (AspectStr == TEXT("chewy"))
        TextureAspects.Chewy = Value;
    else if (AspectStr == TEXT("crispy"))
        TextureAspects.Crispy = Value;
    else if (AspectStr == TEXT("crumbly"))
        TextureAspects.Crumbly = Value;
}

float FPUIngredientBase::GetTotalFlavorValue() const
{
    return FlavorAspects.Umami + FlavorAspects.Salt + FlavorAspects.Sweet + 
           FlavorAspects.Sour + FlavorAspects.Bitter + FlavorAspects.Spicy;
}

float FPUIngredientBase::GetTotalTextureValue() const
{
    return TextureAspects.Rich + TextureAspects.Juicy + TextureAspects.Tender + 
           TextureAspects.Chewy + TextureAspects.Crispy + TextureAspects.Crumbly;
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
    Preparation.ApplyModifiers(FlavorAspects, TextureAspects);
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
    Preparation.RemoveModifiers(FlavorAspects, TextureAspects);
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
                        
                        UE_LOG(LogTemp, Display, TEXT("🔍 FPUIngredientBase::GetCurrentDisplayName - Looking up preparation: Tag=%s, RowName=%s"), 
                            *PrepFullTag, *PrepRowName.ToString());
                        
                        if (FPUPreparationBase* Preparation = LoadedPreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetCurrentDisplayName")))
                        {
                            UE_LOG(LogTemp, Display, TEXT("🔍 FPUIngredientBase::GetCurrentDisplayName - Found preparation: DisplayName=%s, NamePrefix=%s, NameSuffix=%s"), 
                                *Preparation->DisplayName.ToString(), 
                                *Preparation->NamePrefix.ToString(), 
                                *Preparation->NameSuffix.ToString());
                            
                            // If any preparation overrides the base name, use the special name
                            if (Preparation->OverridesBaseName)
                            {
                                SpecialOverrideName = Preparation->SpecialName.ToString();
                                bHasSpecialOverride = true;
                                UE_LOG(LogTemp, Display, TEXT("🔍 FPUIngredientBase::GetCurrentDisplayName - Preparation overrides base name with: %s"), 
                                    *SpecialOverrideName);
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
                                UE_LOG(LogTemp, Display, TEXT("🔍 FPUIngredientBase::GetCurrentDisplayName - Added prefix '%s', CombinedPrefix now: '%s'"), 
                                    *Preparation->NamePrefix.ToString(), *CombinedPrefix);
                            }
                            
                            if (!Preparation->NameSuffix.IsEmpty())
                            {
                                if (!CombinedSuffix.IsEmpty())
                                {
                                    CombinedSuffix = " " + CombinedSuffix;
                                }
                                CombinedSuffix = Preparation->NameSuffix.ToString() + CombinedSuffix;
                                UE_LOG(LogTemp, Display, TEXT("🔍 FPUIngredientBase::GetCurrentDisplayName - Added suffix '%s', CombinedSuffix now: '%s'"), 
                                    *Preparation->NameSuffix.ToString(), *CombinedSuffix);
                            }
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("⚠️ FPUIngredientBase::GetCurrentDisplayName - Could not find preparation row '%s' in data table!"), 
                                *PrepRowName.ToString());
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