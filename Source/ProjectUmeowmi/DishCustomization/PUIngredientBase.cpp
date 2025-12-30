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

ETimeState FPUIngredientBase::GetTimeStateFromValue(float TimeValue)
{
    // Clamp to 0.0-1.0 range
    TimeValue = FMath::Clamp(TimeValue, 0.0f, 1.0f);
    
    // Map to discrete states with thresholds
    // None: 0.0-0.25, Low: 0.25-0.5, Mid: 0.5-0.75, Long: 0.75-1.0
    if (TimeValue < 0.25f)
        return ETimeState::None;
    else if (TimeValue < 0.5f)
        return ETimeState::Low;
    else if (TimeValue < 0.75f)
        return ETimeState::Mid;
    else
        return ETimeState::Long;
}

ETemperatureState FPUIngredientBase::GetTemperatureStateFromValue(float TemperatureValue)
{
    // Clamp to 0.0-1.0 range
    TemperatureValue = FMath::Clamp(TemperatureValue, 0.0f, 1.0f);
    
    // Map to discrete states with thresholds
    // Raw: 0.0-0.25, Low: 0.25-0.5, Med: 0.5-0.75, Hot: 0.75-1.0
    if (TemperatureValue < 0.25f)
        return ETemperatureState::Raw;
    else if (TemperatureValue < 0.5f)
        return ETemperatureState::Low;
    else if (TemperatureValue < 0.75f)
        return ETemperatureState::Med;
    else
        return ETemperatureState::Hot;
}

float FPUIngredientBase::InterpolateModifierValue(float SliderValue, float LowerValue, float UpperValue, float LowerThreshold, float UpperThreshold)
{
    // Clamp slider value to threshold range
    SliderValue = FMath::Clamp(SliderValue, LowerThreshold, UpperThreshold);
    
    // Calculate interpolation factor (0.0 at LowerThreshold, 1.0 at UpperThreshold)
    float Alpha = (SliderValue - LowerThreshold) / (UpperThreshold - LowerThreshold);
    
    // Interpolate between lower and upper values
    return FMath::Lerp(LowerValue, UpperValue, Alpha);
}

void FPUIngredientBase::CalculateTimeTempModifiedAspects(
    float TimeValue, 
    float TemperatureValue, 
    FFlavorAspects& OutFlavor, 
    FTextureAspects& OutTexture
) const
{
    // Start with the current aspects (which already include preparations)
    // Work on copies so we don't modify the base ingredient
    OutFlavor = FlavorAspects;
    OutTexture = TextureAspects;
    
    // Clamp input values
    TimeValue = FMath::Clamp(TimeValue, 0.0f, 1.0f);
    TemperatureValue = FMath::Clamp(TemperatureValue, 0.0f, 1.0f);
    
    // Get discrete states for exact matches
    ETimeState TimeState = GetTimeStateFromValue(TimeValue);
    ETemperatureState TempState = GetTemperatureStateFromValue(TemperatureValue);
    
    // Also get adjacent states for interpolation
    ETimeState LowerTimeState = TimeState;
    ETimeState UpperTimeState = TimeState;
    ETemperatureState LowerTempState = TempState;
    ETemperatureState UpperTempState = TempState;
    
    // Determine interpolation thresholds and adjacent states
    float TimeLowerThreshold = 0.0f;
    float TimeUpperThreshold = 0.25f;
    float TempLowerThreshold = 0.0f;
    float TempUpperThreshold = 0.25f;
    
    // Calculate time thresholds and adjacent states
    switch (TimeState)
    {
        case ETimeState::None:
            TimeLowerThreshold = 0.0f;
            TimeUpperThreshold = 0.25f;
            LowerTimeState = ETimeState::None;
            UpperTimeState = ETimeState::Low;
            break;
        case ETimeState::Low:
            TimeLowerThreshold = 0.25f;
            TimeUpperThreshold = 0.5f;
            LowerTimeState = ETimeState::None;
            UpperTimeState = ETimeState::Mid;
            break;
        case ETimeState::Mid:
            TimeLowerThreshold = 0.5f;
            TimeUpperThreshold = 0.75f;
            LowerTimeState = ETimeState::Low;
            UpperTimeState = ETimeState::Long;
            break;
        case ETimeState::Long:
            TimeLowerThreshold = 0.75f;
            TimeUpperThreshold = 1.0f;
            LowerTimeState = ETimeState::Mid;
            UpperTimeState = ETimeState::Long;
            break;
    }
    
    // Calculate temperature thresholds and adjacent states
    switch (TempState)
    {
        case ETemperatureState::Raw:
            TempLowerThreshold = 0.0f;
            TempUpperThreshold = 0.25f;
            LowerTempState = ETemperatureState::Raw;
            UpperTempState = ETemperatureState::Low;
            break;
        case ETemperatureState::Low:
            TempLowerThreshold = 0.25f;
            TempUpperThreshold = 0.5f;
            LowerTempState = ETemperatureState::Raw;
            UpperTempState = ETemperatureState::Med;
            break;
        case ETemperatureState::Med:
            TempLowerThreshold = 0.5f;
            TempUpperThreshold = 0.75f;
            LowerTempState = ETemperatureState::Low;
            UpperTempState = ETemperatureState::Hot;
            break;
        case ETemperatureState::Hot:
            TempLowerThreshold = 0.75f;
            TempUpperThreshold = 1.0f;
            LowerTempState = ETemperatureState::Med;
            UpperTempState = ETemperatureState::Hot;
            break;
    }
    
    // Process all modifiers
    // We'll collect modifiers for each aspect and apply them with interpolation
    TMap<FName, float> FlavorModifiers;
    TMap<FName, float> TextureModifiers;
    
    for (const FTimeTemperatureModifier& Modifier : TimeTemperatureModifiers)
    {
        // Check if this modifier matches our time/temperature states
        bool bMatchesTime = (Modifier.TimeState == TimeState) || 
                           (Modifier.TimeState == LowerTimeState) || 
                           (Modifier.TimeState == UpperTimeState);
        bool bMatchesTemp = (Modifier.TemperatureState == TempState) || 
                           (Modifier.TemperatureState == LowerTempState) || 
                           (Modifier.TemperatureState == UpperTempState);
        
        if (!bMatchesTime || !bMatchesTemp)
            continue;
        
        // Calculate interpolation weight based on how close the states are
        float TimeWeight = 1.0f;
        float TempWeight = 1.0f;
        
        // If modifier is at exact state, use full weight
        // If modifier is at adjacent state, interpolate
        if (Modifier.TimeState == TimeState)
            TimeWeight = 1.0f;
        else if (Modifier.TimeState == LowerTimeState)
            TimeWeight = 1.0f - ((TimeValue - TimeLowerThreshold) / (TimeUpperThreshold - TimeLowerThreshold));
        else if (Modifier.TimeState == UpperTimeState)
            TimeWeight = (TimeValue - TimeLowerThreshold) / (TimeUpperThreshold - TimeLowerThreshold);
        else
            TimeWeight = 0.0f;
        
        if (Modifier.TemperatureState == TempState)
            TempWeight = 1.0f;
        else if (Modifier.TemperatureState == LowerTempState)
            TempWeight = 1.0f - ((TemperatureValue - TempLowerThreshold) / (TempUpperThreshold - TempLowerThreshold));
        else if (Modifier.TemperatureState == UpperTempState)
            TempWeight = (TemperatureValue - TempLowerThreshold) / (TempUpperThreshold - TempLowerThreshold);
        else
            TempWeight = 0.0f;
        
        // Combined weight (multiply time and temp weights)
        float CombinedWeight = TimeWeight * TempWeight;
        
        if (CombinedWeight <= 0.0f)
            continue;
        
        // Calculate weighted modifier value
        float WeightedValue = Modifier.ModifierValue * CombinedWeight;
        
        // Store modifier for this aspect
        FName AspectName = Modifier.AspectName;
        if (Modifier.AspectType == EAspectType::Flavor)
        {
            if (!FlavorModifiers.Contains(AspectName))
                FlavorModifiers.Add(AspectName, 0.0f);
            FlavorModifiers[AspectName] += WeightedValue;
        }
        else // Texture
        {
            if (!TextureModifiers.Contains(AspectName))
                TextureModifiers.Add(AspectName, 0.0f);
            TextureModifiers[AspectName] += WeightedValue;
        }
    }
    
    // Apply flavor modifiers to output (working on copies)
    for (const auto& Pair : FlavorModifiers)
    {
        FName AspectName = Pair.Key;
        float ModifierValue = Pair.Value;
        
        // Get current value from output
        float CurrentValue = 0.0f;
        FString AspectStr = AspectName.ToString().ToLower();
        
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
        
        // Apply modifier based on type
        // Find the modifier to get its type
        EModificationType ModType = EModificationType::Additive;
        for (const FTimeTemperatureModifier& Modifier : TimeTemperatureModifiers)
        {
            if (Modifier.AspectName == AspectName && 
                Modifier.AspectType == EAspectType::Flavor &&
                (Modifier.TimeState == TimeState || Modifier.TimeState == LowerTimeState || Modifier.TimeState == UpperTimeState) &&
                (Modifier.TemperatureState == TempState || Modifier.TemperatureState == LowerTempState || Modifier.TemperatureState == UpperTempState))
            {
                ModType = Modifier.ModificationType;
                break;
            }
        }
        
        float NewValue = 0.0f;
        if (ModType == EModificationType::Additive)
            NewValue = CurrentValue + ModifierValue;
        else // Multiplicative
            NewValue = CurrentValue * (1.0f + ModifierValue); // ModifierValue as multiplier (e.g., 0.1 = +10%)
        
        NewValue = FMath::Clamp(NewValue, 0.0f, 5.0f);
        
        // Update output
        if (AspectStr == TEXT("umami"))
            OutFlavor.Umami = NewValue;
        else if (AspectStr == TEXT("sweet"))
            OutFlavor.Sweet = NewValue;
        else if (AspectStr == TEXT("salt"))
            OutFlavor.Salt = NewValue;
        else if (AspectStr == TEXT("sour"))
            OutFlavor.Sour = NewValue;
        else if (AspectStr == TEXT("bitter"))
            OutFlavor.Bitter = NewValue;
        else if (AspectStr == TEXT("spicy"))
            OutFlavor.Spicy = NewValue;
    }
    
    // Apply texture modifiers to output (working on copies)
    for (const auto& Pair : TextureModifiers)
    {
        FName AspectName = Pair.Key;
        float ModifierValue = Pair.Value;
        
        // Get current value from output
        float CurrentValue = 0.0f;
        FString AspectStr = AspectName.ToString().ToLower();
        
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
        
        // Apply modifier based on type
        // Find the modifier to get its type
        EModificationType ModType = EModificationType::Additive;
        for (const FTimeTemperatureModifier& Modifier : TimeTemperatureModifiers)
        {
            if (Modifier.AspectName == AspectName && 
                Modifier.AspectType == EAspectType::Texture &&
                (Modifier.TimeState == TimeState || Modifier.TimeState == LowerTimeState || Modifier.TimeState == UpperTimeState) &&
                (Modifier.TemperatureState == TempState || Modifier.TemperatureState == LowerTempState || Modifier.TemperatureState == UpperTempState))
            {
                ModType = Modifier.ModificationType;
                break;
            }
        }
        
        float NewValue = 0.0f;
        if (ModType == EModificationType::Additive)
            NewValue = CurrentValue + ModifierValue;
        else // Multiplicative
            NewValue = CurrentValue * (1.0f + ModifierValue); // ModifierValue as multiplier (e.g., 0.1 = +10%)
        
        NewValue = FMath::Clamp(NewValue, 0.0f, 5.0f);
        
        // Update output
        if (AspectStr == TEXT("rich"))
            OutTexture.Rich = NewValue;
        else if (AspectStr == TEXT("juicy"))
            OutTexture.Juicy = NewValue;
        else if (AspectStr == TEXT("tender"))
            OutTexture.Tender = NewValue;
        else if (AspectStr == TEXT("chewy"))
            OutTexture.Chewy = NewValue;
        else if (AspectStr == TEXT("crispy"))
            OutTexture.Crispy = NewValue;
        else if (AspectStr == TEXT("crumbly"))
            OutTexture.Crumbly = NewValue;
    }
} 