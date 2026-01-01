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

// Map slider value (0.0-1.0) to discrete time state
ETimeState FPUIngredientBase::MapTimeValueToState(float TimeValue)
{
    TimeValue = FMath::Clamp(TimeValue, 0.0f, 1.0f);
    
    if (TimeValue < 0.25f)
        return ETimeState::None;
    else if (TimeValue < 0.5f)
        return ETimeState::Low;
    else if (TimeValue < 0.75f)
        return ETimeState::Mid;
    else
        return ETimeState::Long;
}

// Map slider value (0.0-1.0) to discrete temperature state
ETemperatureState FPUIngredientBase::MapTemperatureValueToState(float TemperatureValue)
{
    TemperatureValue = FMath::Clamp(TemperatureValue, 0.0f, 1.0f);
    
    if (TemperatureValue < 0.25f)
        return ETemperatureState::Raw;
    else if (TemperatureValue < 0.5f)
        return ETemperatureState::Low;
    else if (TemperatureValue < 0.75f)
        return ETemperatureState::Med;
    else
        return ETemperatureState::Hot;
}

// Get default time/temperature modifiers (universal rules)
// These are applied when an ingredient doesn't have custom modifiers
static TArray<FTimeTempModifier> GetDefaultTimeTempModifiers()
{
    TArray<FTimeTempModifier> DefaultModifiers;
    
    // Default rules: Cooking generally increases Umami, reduces Juicy, increases Tender/Crispy
    // These are examples - you can adjust these default rules as needed
    
    // Time modifiers - longer cooking increases Umami and Tender, reduces Juicy
    FTimeTempModifier Mod;
    
    // Low time + Low temp: slight Umami increase (multiplied by 10 for visibility)
    Mod.TimeState = ETimeState::Low;
    Mod.TemperatureState = ETemperatureState::Low;
    Mod.AspectName = FName("Umami");
    Mod.AspectType = 0; // Flavor
    Mod.ModificationType = 0; // Additive
    Mod.ModificationValue = 3.0f; // 0.3 * 10
    DefaultModifiers.Add(Mod);
    
    // Mid time + Med temp: moderate Umami increase, Tender increase (multiplied by 10)
    Mod.TimeState = ETimeState::Mid;
    Mod.TemperatureState = ETemperatureState::Med;
    Mod.AspectName = FName("Umami");
    Mod.ModificationValue = 6.0f; // 0.6 * 10
    DefaultModifiers.Add(Mod);
    
    Mod.AspectName = FName("Tender");
    Mod.AspectType = 1; // Texture
    Mod.ModificationValue = 5.0f; // 0.5 * 10
    DefaultModifiers.Add(Mod);
    
    // Long time + Hot temp: significant Umami increase, Tender increase, Juicy decrease (multiplied by 10)
    Mod.TimeState = ETimeState::Long;
    Mod.TemperatureState = ETemperatureState::Hot;
    Mod.AspectName = FName("Umami");
    Mod.AspectType = 0; // Flavor
    Mod.ModificationValue = 10.0f; // 1.0 * 10
    DefaultModifiers.Add(Mod);
    
    Mod.AspectName = FName("Tender");
    Mod.AspectType = 1; // Texture
    Mod.ModificationValue = 8.0f; // 0.8 * 10
    DefaultModifiers.Add(Mod);
    
    Mod.AspectName = FName("Juicy");
    Mod.ModificationValue = -5.0f; // -0.5 * 10 (Negative = reduction)
    DefaultModifiers.Add(Mod);
    
    // Hot temp increases Crispy (multiplied by 10)
    Mod.TimeState = ETimeState::None; // Any time
    Mod.TemperatureState = ETemperatureState::Hot;
    Mod.AspectName = FName("Crispy");
    Mod.ModificationValue = 7.0f; // 0.7 * 10
    DefaultModifiers.Add(Mod);
    
    return DefaultModifiers;
}

// Calculate modified aspects based on time and temperature
void FPUIngredientBase::CalculateTimeTempModifiedAspects(float TimeValue, float TemperatureValue, FFlavorAspects& OutFlavor, FTextureAspects& OutTexture) const
{
    // Start with base aspects (already includes preparations)
    OutFlavor = FlavorAspects;
    OutTexture = TextureAspects;
    
    // Map slider values to discrete states
    ETimeState TimeState = MapTimeValueToState(TimeValue);
    ETemperatureState TempState = MapTemperatureValueToState(TemperatureValue);
    
    // Get modifiers to apply (either custom or default)
    TArray<FTimeTempModifier> ModifiersToApply;
    
    if (bUseCustomTimeTempModifiers && TimeTemperatureModifiers.Num() > 0)
    {
        // Use custom modifiers for this ingredient
        ModifiersToApply = TimeTemperatureModifiers;
    }
    else
    {
        // Use default universal rules
        ModifiersToApply = GetDefaultTimeTempModifiers();
    }
    
    // Apply modifiers that match the current time/temp state
    UE_LOG(LogTemp, Display, TEXT("🔍 FPUIngredientBase::CalculateTimeTempModifiedAspects - TimeState: %d, TempState: %d, Checking %d modifiers"),
        (int32)TimeState, (int32)TempState, ModifiersToApply.Num());
    
    for (const FTimeTempModifier& Modifier : ModifiersToApply)
    {
        // Match exact states (for "any" state behavior, add multiple modifiers)
        bool bTimeMatches = (Modifier.TimeState == TimeState);
        bool bTempMatches = (Modifier.TemperatureState == TempState);
        
        UE_LOG(LogTemp, Display, TEXT("🔍   Modifier: Time=%d (match: %s), Temp=%d (match: %s), Aspect=%s, Value=%.2f"),
            (int32)Modifier.TimeState, bTimeMatches ? TEXT("YES") : TEXT("NO"),
            (int32)Modifier.TemperatureState, bTempMatches ? TEXT("YES") : TEXT("NO"),
            *Modifier.AspectName.ToString(), Modifier.ModificationValue);
        
        if (bTimeMatches && bTempMatches)
        {
            FString AspectStr = Modifier.AspectName.ToString().ToLower();
            float CurrentValue = 0.0f;
            bool bFound = false;
            
            // Get current aspect value from OutFlavor/OutTexture structs directly (allows chaining modifiers)
            if (Modifier.AspectType == 0) // Flavor
            {
                if (AspectStr == TEXT("umami"))
                    CurrentValue = OutFlavor.Umami;
                else if (AspectStr == TEXT("sweet"))
                    CurrentValue = OutFlavor.Sweet;
                else if (AspectStr == TEXT("salt"))
                    CurrentValue = OutFlavor.Salt;
                else if (AspectStr == TEXT("sour"))
                    CurrentValue = OutFlavor.Sour;
                else if (AspectStr == TEXT("bitter"))
                    CurrentValue = OutFlavor.Bitter;
                else if (AspectStr == TEXT("spicy"))
                    CurrentValue = OutFlavor.Spicy;
                bFound = true;
            }
            else if (Modifier.AspectType == 1) // Texture
            {
                if (AspectStr == TEXT("rich"))
                    CurrentValue = OutTexture.Rich;
                else if (AspectStr == TEXT("juicy"))
                    CurrentValue = OutTexture.Juicy;
                else if (AspectStr == TEXT("tender"))
                    CurrentValue = OutTexture.Tender;
                else if (AspectStr == TEXT("chewy"))
                    CurrentValue = OutTexture.Chewy;
                else if (AspectStr == TEXT("crispy"))
                    CurrentValue = OutTexture.Crispy;
                else if (AspectStr == TEXT("crumbly"))
                    CurrentValue = OutTexture.Crumbly;
                bFound = true;
            }
            
            if (bFound)
            {
                UE_LOG(LogTemp, Display, TEXT("🔍   Applying modifier: Current=%f, Modifier=%f, Type=%d"),
                    CurrentValue, Modifier.ModificationValue, (int32)Modifier.ModificationType);
                
                // Apply modification
                float ModifiedValue = CurrentValue;
                if (Modifier.ModificationType == 0) // Additive
                {
                    ModifiedValue = CurrentValue + Modifier.ModificationValue;
                }
                else if (Modifier.ModificationType == 1) // Multiplicative
                {
                    ModifiedValue = CurrentValue * Modifier.ModificationValue;
                }
                
                UE_LOG(LogTemp, Display, TEXT("🔍   After modification: %f"), ModifiedValue);
                
                // Only clamp minimum to 0 (allow values above 5.0 for visibility on radar chart)
                // Round to 0.5 increments
                ModifiedValue = FMath::Max(ModifiedValue, 0.0f);
                ModifiedValue = FMath::RoundToFloat(ModifiedValue * 2.0f) / 2.0f;
                
                UE_LOG(LogTemp, Display, TEXT("🔍   After rounding: %f"), ModifiedValue);
                
                // Set the modified value directly in output (don't modify const object)
                if (Modifier.AspectType == 0) // Flavor
                {
                    if (AspectStr == TEXT("umami"))
                        OutFlavor.Umami = ModifiedValue;
                    else if (AspectStr == TEXT("sweet"))
                        OutFlavor.Sweet = ModifiedValue;
                    else if (AspectStr == TEXT("salt"))
                        OutFlavor.Salt = ModifiedValue;
                    else if (AspectStr == TEXT("sour"))
                        OutFlavor.Sour = ModifiedValue;
                    else if (AspectStr == TEXT("bitter"))
                        OutFlavor.Bitter = ModifiedValue;
                    else if (AspectStr == TEXT("spicy"))
                        OutFlavor.Spicy = ModifiedValue;
                }
                else if (Modifier.AspectType == 1) // Texture
                {
                    if (AspectStr == TEXT("rich"))
                        OutTexture.Rich = ModifiedValue;
                    else if (AspectStr == TEXT("juicy"))
                        OutTexture.Juicy = ModifiedValue;
                    else if (AspectStr == TEXT("tender"))
                        OutTexture.Tender = ModifiedValue;
                    else if (AspectStr == TEXT("chewy"))
                        OutTexture.Chewy = ModifiedValue;
                    else if (AspectStr == TEXT("crispy"))
                        OutTexture.Crispy = ModifiedValue;
                    else if (AspectStr == TEXT("crumbly"))
                        OutTexture.Crumbly = ModifiedValue;
                }
            }
        }
    }
}

// Get modified flavor aspects
FFlavorAspects FPUIngredientBase::GetModifiedFlavorAspects(float TimeValue, float TemperatureValue) const
{
    FFlavorAspects ModifiedFlavor;
    FTextureAspects ModifiedTexture; // Dummy, not used but required by function
    CalculateTimeTempModifiedAspects(TimeValue, TemperatureValue, ModifiedFlavor, ModifiedTexture);
    return ModifiedFlavor;
}

// Get modified texture aspects
FTextureAspects FPUIngredientBase::GetModifiedTextureAspects(float TimeValue, float TemperatureValue) const
{
    FFlavorAspects ModifiedFlavor; // Dummy, not used but required by function
    FTextureAspects ModifiedTexture;
    CalculateTimeTempModifiedAspects(TimeValue, TemperatureValue, ModifiedFlavor, ModifiedTexture);
    return ModifiedTexture;
} 