#include "PURadarChart.h"
#include "RadarChartStyle.h"
#include "RadarChartTypes.h"
#include "SRadarChart.h"
#include "Engine/DataTable.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "../DishCustomization/PUDishBase.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"

UPURadarChart::UPURadarChart()
    : CurrentFluctuationStep(0)
    , TotalFluctuationSteps(0)
    , FluctuationIntensity(0.3f)
    , FluctuationDuration(0.2f)
    , SettleDuration(0.5f)
    , AnimationFps(18)
    , AnimationEase(EEasingFunc::ExpoOut)
{
    // Initialize with default segment count
    InitializeSegments();

    // Show icons by default
    ShowIcons(true);
}

void UPURadarChart::ShowIcons(bool bShow)
{
    // Enable/disable icon display
    ChartStyle.bShowIcons = bShow;
    
    if (bShow)
    {
        // Configure icon settings
        ChartStyle.IconSize = FVector2D(32.0f, 32.0f);  // Set a reasonable icon size
        
        // Configure icon color
        ChartStyle.IconColor.Method = ERadarChartColorOverride::None;
        ChartStyle.IconColor.Color = FLinearColor::White;
        
        ChartStyle.IconPadding = FVector2D(5.0f, 5.0f);  // Add some padding around icons
        ChartStyle.bAlwaysUprightIcon = true;  // Keep icons upright for better readability
    }
    
    // Force a rebuild of the chart to apply changes
    ForceRebuild();
}

bool UPURadarChart::SetSegmentCount(int32 NewSegmentCount)
{
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetSegmentCount: Attempting to set %d segments (current: %d, min: %d, max: %d)"), 
        NewSegmentCount, ChartStyle.Segments.Num(), MinSegmentCount, MaxSegmentCount);

    // Validate the new segment count
    if (NewSegmentCount < MinSegmentCount || NewSegmentCount > MaxSegmentCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetSegmentCount: Invalid segment count %d (must be between %d and %d)"), 
            NewSegmentCount, MinSegmentCount, MaxSegmentCount);
        return false;
    }

    // Clear existing segments
    ChartStyle.Segments.Empty();
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetSegmentCount: Cleared existing segments"));

    // Add new segments
    for (int32 i = 0; i < NewSegmentCount; ++i)
    {
        ChartStyle.Segments.AddZeroed();
        ChartStyle.Segments.Last().Name = FText::FromString(FString::Printf(TEXT("Segment %d"), i));
        ChartStyle.Segments.Last().SegmentColor = FLinearColor::White;
    }

    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetSegmentCount: Created %d new segments"), ChartStyle.Segments.Num());

    // Update value layers to match new segment count
    UpdateValueLayers();

    // Force a rebuild of the chart
    ForceRebuild();

    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetSegmentCount: Successfully set %d segments"), NewSegmentCount);

    return true;
}

int32 UPURadarChart::GetSegmentCount() const
{
    return ChartStyle.Segments.Num();
}

void UPURadarChart::SetValues(const TArray<float>& InValues)
{
    // Validate input array size matches segment count
    if (InValues.Num() != ChartStyle.Segments.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValues: Input array size (%d) does not match segment count (%d)"), 
            InValues.Num(), ChartStyle.Segments.Num());
        return;
    }

    // Validate that we have valid values (not NaN or infinite)
    for (int32 i = 0; i < InValues.Num(); ++i)
    {
        if (!FMath::IsFinite(InValues[i]))
        {
            UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValues: Invalid value at index %d: %f"), i, InValues[i]);
            return;
        }
    }

    // Ensure we have at least one value layer
    if (ValueLayers.Num() == 0)
    {
        ValueLayers.AddZeroed();
    }

    // Set the values for the first layer
    SetValuesForLayer(0, InValues);

    // Log the values being set for debugging
    FString ValuesString = TEXT("[");
    for (int32 i = 0; i < InValues.Num(); ++i)
    {
        if (i > 0) ValuesString += TEXT(", ");
        ValuesString += FString::Printf(TEXT("%.2f"), InValues[i]);
    }
    ValuesString += TEXT("]");
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValues: Set values: %s"), *ValuesString);
}

void UPURadarChart::SetValuesAnimated(const TArray<float>& InValues, float Duration, uint8 Fps, TEnumAsByte<EEasingFunc::Type> Ease)
{
    // Validate input array size matches segment count
    if (InValues.Num() != ChartStyle.Segments.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesAnimated: Input array size (%d) does not match segment count (%d)"), 
            InValues.Num(), ChartStyle.Segments.Num());
        return;
    }

    // Validate that we have valid values (not NaN or infinite)
    for (int32 i = 0; i < InValues.Num(); ++i)
    {
        if (!FMath::IsFinite(InValues[i]))
        {
            UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesAnimated: Invalid value at index %d: %f"), i, InValues[i]);
            return;
        }
    }

    // Ensure we have at least one value layer
    if (ValueLayers.Num() == 0)
    {
        ValueLayers.AddZeroed();
    }

    // IMPORTANT: The widget and URadarChart share the same ValueLayers array
    // The animation system uses ValueLayers[0].RawValues as the starting point (OldValues)
    // We need to ensure RawValues are properly sized, but NOT overwrite them with zeros
    // If they're empty, the animation will start from 0 (which is correct for first time)
    // If they exist, they should already contain the current displayed values
    
    // Ensure RawValues array is properly sized to match segment count
    // But preserve existing values - don't initialize to zero if they already exist
    if (ValueLayers[0].RawValues.Num() != ChartStyle.Segments.Num())
    {
        // Resize to match segment count, preserving existing values where possible
        int32 OldSize = ValueLayers[0].RawValues.Num();
        ValueLayers[0].RawValues.SetNum(ChartStyle.Segments.Num());
        
        // Only initialize NEW slots to 0 (if segment count increased)
        // Don't touch existing values - they should already be current
        for (int32 i = OldSize; i < ValueLayers[0].RawValues.Num(); ++i)
        {
            ValueLayers[0].RawValues[i] = 0.0f;
        }
    }

    // CRITICAL: The widget and URadarChart share the same ValueLayers array pointer (see RadarChart.cpp line 57)
    // So ValueLayers[0].RawValues should already contain the current displayed values
    // However, we need to ensure they're properly initialized before animating
    
    TSharedPtr<SRadarChart> RadarWidget = GetRadarWidget();
    if (RadarWidget.IsValid())
    {
        // Log current RawValues for debugging
        FString CurrentValuesStr = TEXT("[");
        for (int32 i = 0; i < ValueLayers[0].RawValues.Num(); ++i)
        {
            if (i > 0) CurrentValuesStr += TEXT(", ");
            CurrentValuesStr += FString::Printf(TEXT("%.2f"), ValueLayers[0].RawValues[i]);
        }
        CurrentValuesStr += TEXT("]");
        
        // The widget's SetValuesAnimated will:
        // 1. Check if there's an ongoing animation and cancel it, preserving current RawValues
        // 2. Use the CURRENT ValueLayers[0].RawValues as OldValues (line 597 in SRadarChart.cpp)
        // 3. Animate from those current values to InValues
        
        // Since we share the same array, RawValues should already be current
        // But if they're empty/zero and this isn't the first call, something went wrong
        RadarWidget->SetValuesAnimated(0, InValues, Duration, Fps, Ease);
        
        FString NewValuesStr = TEXT("[");
        for (int32 i = 0; i < InValues.Num(); ++i)
        {
            if (i > 0) NewValuesStr += TEXT(", ");
            NewValuesStr += FString::Printf(TEXT("%.2f"), InValues[i]);
        }
        NewValuesStr += TEXT("]");
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesAnimated: Animating from %s to %s (duration %.2f, fps %d)"), 
            *CurrentValuesStr, *NewValuesStr, Duration, Fps);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesAnimated: Radar widget is not valid, falling back to non-animated SetValues"));
        SetValues(InValues);
    }
}

void UPURadarChart::SetNormalizationScaleAnimated(float InValue, float Duration, uint8 Fps, TEnumAsByte<EEasingFunc::Type> Ease)
{
    // Get the Slate widget and call the animated method
    TSharedPtr<SRadarChart> RadarWidget = GetRadarWidget();
    if (RadarWidget.IsValid())
    {
        RadarWidget->SetNormalizationScaleAnimated(InValue, Duration, Fps, Ease);
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetNormalizationScaleAnimated: Setting scale to %.2f with duration %.2f, fps %d"), 
            InValue, Duration, Fps);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetNormalizationScaleAnimated: Radar widget is not valid, falling back to non-animated SetNormalizationScale"));
        SetNormalizationScale(InValue);
    }
}

bool UPURadarChart::SetSegmentNames(const TArray<FString>& InNames)
{
    // Validate that we have enough segments
    if (InNames.Num() != ChartStyle.Segments.Num())
    {
        return false;
    }

    // Update segment names
    for (int32 i = 0; i < InNames.Num(); ++i)
    {
        ChartStyle.Segments[i].Name = FText::FromString(InNames[i]);
    }

    // Force a rebuild of the chart
    ForceRebuild();

    return true;
}

bool UPURadarChart::SetValuesFromIngredient(const FPUIngredientBase& Ingredient)
{
    // Set the number of segments based on the total number of aspects (12 total: 6 flavors + 6 textures)
    const int32 TotalAspects = 12;
    if (!SetSegmentCount(TotalAspects))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromIngredient: Failed to set segment count"));
        return false;
    }

    // Get the values and display names from the ingredient's aspects
    TArray<float> Values;
    TArray<FString> DisplayNames;
    
    // Add flavor aspects (in order: Umami, Salt, Sweet, Sour, Bitter, Spicy)
    Values.Add(Ingredient.FlavorAspects.Umami);
    DisplayNames.Add(TEXT("Umami"));
    Values.Add(Ingredient.FlavorAspects.Salt);
    DisplayNames.Add(TEXT("Salt"));
    Values.Add(Ingredient.FlavorAspects.Sweet);
    DisplayNames.Add(TEXT("Sweet"));
    Values.Add(Ingredient.FlavorAspects.Sour);
    DisplayNames.Add(TEXT("Sour"));
    Values.Add(Ingredient.FlavorAspects.Bitter);
    DisplayNames.Add(TEXT("Bitter"));
    Values.Add(Ingredient.FlavorAspects.Spicy);
    DisplayNames.Add(TEXT("Spicy"));
    
    // Add texture aspects
    Values.Add(Ingredient.TextureAspects.Rich);
    DisplayNames.Add(TEXT("Rich"));
    Values.Add(Ingredient.TextureAspects.Juicy);
    DisplayNames.Add(TEXT("Juicy"));
    Values.Add(Ingredient.TextureAspects.Tender);
    DisplayNames.Add(TEXT("Tender"));
    Values.Add(Ingredient.TextureAspects.Chewy);
    DisplayNames.Add(TEXT("Chewy"));
    Values.Add(Ingredient.TextureAspects.Crispy);
    DisplayNames.Add(TEXT("Crispy"));
    Values.Add(Ingredient.TextureAspects.Crumbly);
    DisplayNames.Add(TEXT("Crumbly"));
    
    // Set the segment names using the display names
    SetSegmentNames(DisplayNames);

    // Set the values
    SetValues(Values);
    return true;
}

bool UPURadarChart::SetValuesFromIngredientWithTimeTemp(const FPUIngredientBase& Ingredient, float TimeValue, float TemperatureValue)
{
    // Set the number of segments based on the total number of aspects (12 total: 6 flavors + 6 textures)
    const int32 TotalAspects = 12;
    if (!SetSegmentCount(TotalAspects))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromIngredientWithTimeTemp: Failed to set segment count"));
        return false;
    }

    // Get the MODIFIED values (with time/temp applied) and display names from the ingredient's aspects
    FFlavorAspects ModifiedFlavor = Ingredient.GetModifiedFlavorAspects(TimeValue, TemperatureValue);
    FTextureAspects ModifiedTexture = Ingredient.GetModifiedTextureAspects(TimeValue, TemperatureValue);
    
    TArray<float> Values;
    TArray<FString> DisplayNames;
    
    // Add flavor aspects (in order: Umami, Salt, Sweet, Sour, Bitter, Spicy)
    Values.Add(ModifiedFlavor.Umami);
    DisplayNames.Add(TEXT("Umami"));
    Values.Add(ModifiedFlavor.Salt);
    DisplayNames.Add(TEXT("Salt"));
    Values.Add(ModifiedFlavor.Sweet);
    DisplayNames.Add(TEXT("Sweet"));
    Values.Add(ModifiedFlavor.Sour);
    DisplayNames.Add(TEXT("Sour"));
    Values.Add(ModifiedFlavor.Bitter);
    DisplayNames.Add(TEXT("Bitter"));
    Values.Add(ModifiedFlavor.Spicy);
    DisplayNames.Add(TEXT("Spicy"));
    
    // Add texture aspects
    Values.Add(ModifiedTexture.Rich);
    DisplayNames.Add(TEXT("Rich"));
    Values.Add(ModifiedTexture.Juicy);
    DisplayNames.Add(TEXT("Juicy"));
    Values.Add(ModifiedTexture.Tender);
    DisplayNames.Add(TEXT("Tender"));
    Values.Add(ModifiedTexture.Chewy);
    DisplayNames.Add(TEXT("Chewy"));
    Values.Add(ModifiedTexture.Crispy);
    DisplayNames.Add(TEXT("Crispy"));
    Values.Add(ModifiedTexture.Crumbly);
    DisplayNames.Add(TEXT("Crumbly"));
    
    // Set the segment names using the display names
    SetSegmentNames(DisplayNames);

    // Set the values
    SetValues(Values);
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromIngredientWithTimeTemp: Updated with Time=%.2f, Temp=%.2f"), TimeValue, TemperatureValue);
    return true;
}

bool UPURadarChart::SetValuesFromDishIngredients(const FPUDishBase& Dish)
{
    // Track unique ingredients and their quantities
    TMap<FGameplayTag, int32> IngredientQuantities;
    TMap<FGameplayTag, FString> IngredientNames;
    TMap<FGameplayTag, UTexture2D*> IngredientTextures;
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Starting with %d ingredient instances"), Dish.IngredientInstances.Num());
    
    for (const FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        // Use convenient field if available, fallback to data field
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Processing instance with tag: %s, quantity: %d"), 
            *InstanceTag.ToString(), Instance.Quantity);
        
        // Add or update quantity for this ingredient
        IngredientQuantities.FindOrAdd(InstanceTag) += Instance.Quantity;
        
        // Get the display name and texture if we haven't already
        if (!IngredientNames.Contains(InstanceTag))
        {
            FPUIngredientBase Ingredient;
            if (Dish.GetIngredient(InstanceTag, Ingredient))
            {
                IngredientNames.Add(InstanceTag, Ingredient.DisplayName.ToString());
                if (Ingredient.PreviewTexture)
                {
                    IngredientTextures.Add(InstanceTag, Ingredient.PreviewTexture);
                    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Found ingredient %s with texture %p"), 
                        *Ingredient.DisplayName.ToString(), 
                        Ingredient.PreviewTexture);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: Failed to get ingredient data for tag: %s"), 
                    *InstanceTag.ToString());
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Found %d unique ingredients"), IngredientQuantities.Num());
    
    // Calculate scale based on duplicate slots (same ingredient in multiple slots)
    // Each slot can contain an ingredient, and scale increases when the same ingredient appears in multiple slots
    // Base scale is 5, then add 5 for each duplicate slot (ingredient appearing more than once)
    // Example: 4 different ingredients (Anchovy, Duck, Noodles, Bread) = 0 duplicates = 5 scale
    // Example: 2 Anchovy slots + 1 Noodles + 1 Bread = 1 duplicate = 5 + 5 = 10 scale
    const float BASE_SCALE = 5.0f;
    const float SCALE_PER_DUPLICATE_SLOT = 5.0f;
    const float MAX_SCALE = 60.0f;
    
    // Count how many times each ingredient appears in slots (not quantity, but slot count)
    // Count duplicate slots: for each ingredient that appears in multiple slots, count the extra slots
    int32 DuplicateSlots = 0;
    for (const auto& Pair : IngredientQuantities)
    {
        // Count how many slots this ingredient appears in
        int32 SlotCount = 0;
        for (const FIngredientInstance& Instance : Dish.IngredientInstances)
        {
            FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
            if (InstanceTag == Pair.Key && InstanceTag.IsValid())
            {
                SlotCount++;
            }
        }
        
        // If ingredient appears in more than 1 slot, count the extra slots as duplicates
        if (SlotCount > 1)
        {
            DuplicateSlots += (SlotCount - 1);
        }
    }
    
    // Calculate scale: base 5 + 5 per duplicate slot
    // Cap at max scale of 60
    float NormalizationScale = FMath::Min(BASE_SCALE + (static_cast<float>(DuplicateSlots) * SCALE_PER_DUPLICATE_SLOT), MAX_SCALE);
    
    // Set the normalization scale for the radar chart with smooth animation
    SetNormalizationScaleAnimated(NormalizationScale, 0.5f, 18, EEasingFunc::ExpoOut);
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Duplicate slots: %d, Scale: %.1f"), 
        DuplicateSlots, NormalizationScale);
    
    // Calculate total segments needed (minimum 3, or more if we have more ingredients)
    int32 TotalSegments = FMath::Max(3, IngredientQuantities.Num());
    
    // CRITICAL: Preserve current RawValues before changing segment count
    // SetSegmentCount calls UpdateValueLayers() which resets RawValues to zero
    // We need to preserve them so animation can start from current values, not zero
    TArray<float> PreservedRawValues;
    bool bSegmentCountChanged = (GetSegmentCount() != TotalSegments);
    if (!bSegmentCountChanged && ValueLayers.Num() > 0 && ValueLayers[0].RawValues.Num() == GetSegmentCount())
    {
        // Segment count is the same, preserve current RawValues
        PreservedRawValues = ValueLayers[0].RawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Preserving %d current RawValues"), PreservedRawValues.Num());
    }
    
    // Set the number of segments (this will reset RawValues if count changed)
    if (!SetSegmentCount(TotalSegments))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: Failed to set segment count to %d"), TotalSegments);
        return false;
    }
    
    // Restore preserved RawValues if segment count didn't change
    if (!bSegmentCountChanged && PreservedRawValues.Num() == TotalSegments && ValueLayers.Num() > 0)
    {
        ValueLayers[0].RawValues = PreservedRawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Restored preserved RawValues"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Set up %d segments"), TotalSegments);
    
    // Prepare arrays for values and names
    TArray<float> Values;
    TArray<FString> DisplayNames;
    TArray<UTexture2D*> IconTextures;
    
    // Initialize with placeholder values for all segments
    for (int32 i = 0; i < TotalSegments; ++i)
    {
        Values.Add(0.0f); // No value for placeholders
        DisplayNames.Add(TEXT("???")); // Placeholder text
        IconTextures.Add(nullptr); // No texture for placeholders
    }
    
    // Fill in the first slots with actual ingredients, leave remaining as placeholders
    int32 SegmentIndex = 0;
    for (const auto& Pair : IngredientQuantities)
    {
        if (SegmentIndex < TotalSegments)
        {
            Values[SegmentIndex] = static_cast<float>(Pair.Value);
            
            // Get the display name, defaulting to the tag name if not found
            FString DisplayName;
            if (const FString* FoundName = IngredientNames.Find(Pair.Key))
            {
                DisplayName = *FoundName;
            }
            else
            {
                DisplayName = Pair.Key.ToString();
            }
            DisplayNames[SegmentIndex] = DisplayName;
            
            // Get the texture
            const UTexture2D* const* FoundTexture = IngredientTextures.Find(Pair.Key);
            if (FoundTexture)
            {
                IconTextures[SegmentIndex] = const_cast<UTexture2D*>(*FoundTexture);
            }
            
            UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Set segment %d to %s (Qty: %d)"), 
                SegmentIndex, *DisplayName, Pair.Value);
            
            SegmentIndex++;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: Too many ingredients, skipping %s"), 
                *Pair.Key.ToString());
        }
    }
    
    // Ensure we have exactly the right number of segments with proper names
    while (DisplayNames.Num() < TotalSegments)
    {
        DisplayNames.Add(TEXT("???"));
        Values.Add(0.0f);
        IconTextures.Add(nullptr);
    }
    
    // Set the segment names
    if (!SetSegmentNames(DisplayNames))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: Failed to set segment names"));
        return false;
    }
    
    // Set the values with smooth animation
    SetValuesAnimated(Values, 0.5f, 18, EEasingFunc::ExpoOut);
    
    // Set the icons
    for (int32 i = 0; i < IconTextures.Num() && i < TotalSegments; ++i)
    {
        SetSegmentIcon(i, IconTextures[i]);
    }
    
    // Make sure icons are enabled
    ShowIcons(true);
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Completed setup with %d segments"), TotalSegments);
    return true;
}

void UPURadarChart::SetSegmentIcon(int32 SegmentIndex, UTexture2D* IconTexture)
{
    // Safety check: ensure segment index is valid
    if (SegmentIndex < 0 || SegmentIndex >= ChartStyle.Segments.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetSegmentIcon: Invalid segment index %d (max: %d)"), 
            SegmentIndex, ChartStyle.Segments.Num() - 1);
        return;
    }
    
    // Safety check: ensure the segment exists
    if (SegmentIndex >= ChartStyle.Segments.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetSegmentIcon: Segment %d does not exist"), SegmentIndex);
        return;
    }
    
    if (IconTexture)
    {
        // Set up the icon and its brush
        ChartStyle.Segments[SegmentIndex].Icon = IconTexture;
        ChartStyle.Segments[SegmentIndex].IconBrush.SetResourceObject(IconTexture);
        ChartStyle.Segments[SegmentIndex].IconBrush.DrawAs = ESlateBrushDrawType::Image;
        ChartStyle.Segments[SegmentIndex].IconBrush.TintColor = FSlateColor(FLinearColor::White);
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetSegmentIcon: Set icon for segment %d with texture %p"), 
            SegmentIndex, IconTexture);
    }
    else
    {
        // Clear the icon
        ChartStyle.Segments[SegmentIndex].Icon = nullptr;
        ChartStyle.Segments[SegmentIndex].IconBrush.SetResourceObject(nullptr);
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetSegmentIcon: Cleared icon for segment %d"), SegmentIndex);
    }
    
    // Force a rebuild of the chart to show the new icon
    ForceRebuild();
}

bool UPURadarChart::SetValuesFromDishFlavorProfile(const FPUDishBase& Dish)
{
    // Always show all 6 flavor aspects
    const int32 TOTAL_FLAVOR_ASPECTS = 6;
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Starting flavor profile setup"));
    
    // Define all flavor aspects in order (Umami, Salt, Sweet, Sour, Bitter, Spicy)
    TArray<FName> FlavorAspectNames = {
        TEXT("Umami"),
        TEXT("Salt"),
        TEXT("Sweet"),
        TEXT("Sour"),
        TEXT("Bitter"),
        TEXT("Spicy")
    };
    
    // Always show all 6 segments
    int32 TotalSegments = TOTAL_FLAVOR_ASPECTS;
    
    // CRITICAL: Preserve current RawValues before changing segment count
    // SetSegmentCount calls UpdateValueLayers() which resets RawValues to zero
    // We need to preserve them so animation can start from current values, not zero
    TArray<float> PreservedRawValues;
    bool bSegmentCountChanged = (GetSegmentCount() != TotalSegments);
    if (!bSegmentCountChanged && ValueLayers.Num() > 0 && ValueLayers[0].RawValues.Num() == GetSegmentCount())
    {
        // Segment count is the same, preserve current RawValues
        PreservedRawValues = ValueLayers[0].RawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Preserving %d current RawValues"), PreservedRawValues.Num());
    }
    
    // Set the number of segments (this will reset RawValues if count changed)
    if (!SetSegmentCount(TotalSegments))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Failed to set segment count to %d"), TotalSegments);
        return false;
    }
    
    // Restore preserved RawValues if segment count didn't change
    if (!bSegmentCountChanged && PreservedRawValues.Num() == TotalSegments && ValueLayers.Num() > 0)
    {
        ValueLayers[0].RawValues = PreservedRawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Restored preserved RawValues"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Set up %d segments"), TotalSegments);
    
    // Prepare arrays for values and names
    TArray<float> Values;
    TArray<FString> DisplayNames;
    
    // Always set all 6 flavor aspects, even if they have zero values
    for (const FName& AspectName : FlavorAspectNames)
    {
        float AspectValue = Dish.GetTotalFlavorAspect(AspectName);
        Values.Add(AspectValue);
        DisplayNames.Add(AspectName.ToString());
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Set segment %s (Value: %.2f)"), 
            *AspectName.ToString(), AspectValue);
    }
    
    // Calculate normalization scale based on maximum value
    // Scale increments by 25: 0-25 = scale 25, 25-50 = scale 50, etc., up to max 300
    const float SCALE_INCREMENT = 25.0f;
    const float MAX_SCALE = 300.0f;
    
    float MaxValue = 0.0f;
    for (float Value : Values)
    {
        if (Value > MaxValue)
        {
            MaxValue = Value;
        }
    }
    
    // Calculate scale: round up to next 25 increment, capped at 300
    float NormalizationScale = FMath::Min(FMath::CeilToFloat(MaxValue / SCALE_INCREMENT) * SCALE_INCREMENT, MAX_SCALE);
    
    // Ensure minimum scale of 25 if we have any values
    if (MaxValue > 0.0f && NormalizationScale < SCALE_INCREMENT)
    {
        NormalizationScale = SCALE_INCREMENT;
    }
    
    // Set the normalization scale for the radar chart with smooth animation
    SetNormalizationScaleAnimated(NormalizationScale, 0.5f, 18, EEasingFunc::ExpoOut);
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Max value: %.2f, Scale: %.1f"), 
        MaxValue, NormalizationScale);
    
    // Set the segment names
    if (!SetSegmentNames(DisplayNames))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Failed to set segment names"));
        return false;
    }
    
    // Set the values with smooth animation
    SetValuesAnimated(Values, 0.5f, 18, EEasingFunc::ExpoOut);
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Completed setup with %d segments"), TotalSegments);
    return true;
}

bool UPURadarChart::SetValuesFromDishTextureProfile(const FPUDishBase& Dish)
{
    // Always show all 6 texture aspects
    const int32 TOTAL_TEXTURE_ASPECTS = 6;
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Starting texture profile setup"));
    
    // Define all texture aspects in order
    TArray<FName> TextureAspectNames = {
        TEXT("Rich"),
        TEXT("Juicy"),
        TEXT("Tender"),
        TEXT("Chewy"),
        TEXT("Crispy"),
        TEXT("Crumbly")
    };
    
    // Always show all 6 segments
    int32 TotalSegments = TOTAL_TEXTURE_ASPECTS;
    
    // CRITICAL: Preserve current RawValues before changing segment count
    // SetSegmentCount calls UpdateValueLayers() which resets RawValues to zero
    // We need to preserve them so animation can start from current values, not zero
    TArray<float> PreservedRawValues;
    bool bSegmentCountChanged = (GetSegmentCount() != TotalSegments);
    if (!bSegmentCountChanged && ValueLayers.Num() > 0 && ValueLayers[0].RawValues.Num() == GetSegmentCount())
    {
        // Segment count is the same, preserve current RawValues
        PreservedRawValues = ValueLayers[0].RawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Preserving %d current RawValues"), PreservedRawValues.Num());
    }
    
    // Set the number of segments (this will reset RawValues if count changed)
    if (!SetSegmentCount(TotalSegments))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Failed to set segment count to %d"), TotalSegments);
        return false;
    }
    
    // Restore preserved RawValues if segment count didn't change
    if (!bSegmentCountChanged && PreservedRawValues.Num() == TotalSegments && ValueLayers.Num() > 0)
    {
        ValueLayers[0].RawValues = PreservedRawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Restored preserved RawValues"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Set up %d segments"), TotalSegments);
    
    // Prepare arrays for values and names
    TArray<float> Values;
    TArray<FString> DisplayNames;
    
    // Always set all 6 texture aspects, even if they have zero values
    for (const FName& AspectName : TextureAspectNames)
    {
        float AspectValue = Dish.GetTotalTextureAspect(AspectName);
        Values.Add(AspectValue);
        DisplayNames.Add(AspectName.ToString());
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Set segment %s (Value: %.2f)"), 
            *AspectName.ToString(), AspectValue);
    }
    
    // Calculate normalization scale based on maximum value
    // Scale increments by 25: 0-25 = scale 25, 25-50 = scale 50, etc., up to max 300
    const float SCALE_INCREMENT = 25.0f;
    const float MAX_SCALE = 300.0f;
    
    float MaxValue = 0.0f;
    for (float Value : Values)
    {
        if (Value > MaxValue)
        {
            MaxValue = Value;
        }
    }
    
    // Calculate scale: round up to next 25 increment, capped at 300
    float NormalizationScale = FMath::Min(FMath::CeilToFloat(MaxValue / SCALE_INCREMENT) * SCALE_INCREMENT, MAX_SCALE);
    
    // Ensure minimum scale of 25 if we have any values
    if (MaxValue > 0.0f && NormalizationScale < SCALE_INCREMENT)
    {
        NormalizationScale = SCALE_INCREMENT;
    }
    
    // Set the normalization scale for the radar chart with smooth animation
    SetNormalizationScaleAnimated(NormalizationScale, 0.5f, 18, EEasingFunc::ExpoOut);
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Max value: %.2f, Scale: %.1f"), 
        MaxValue, NormalizationScale);
    
    // Set the segment names
    if (!SetSegmentNames(DisplayNames))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Failed to set segment names"));
        return false;
    }
    
    // Set the values with smooth animation
    SetValuesAnimated(Values, 0.5f, 18, EEasingFunc::ExpoOut);
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Completed setup with %d segments"), TotalSegments);
    return true;
}

bool UPURadarChart::SetValuesFromDishFlavorProfileWithFluctuations(
    const FPUDishBase& Dish,
    float InFluctuationIntensity,
    int32 NumFluctuations,
    float InFluctuationDuration,
    float InSettleDuration)
{
    // Always show all 6 flavor aspects
    const int32 TOTAL_FLAVOR_ASPECTS = 6;
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfileWithFluctuations: Starting flavor profile setup with fluctuations"));
    
    // Define all flavor aspects in order (Umami, Salt, Sweet, Sour, Bitter, Spicy)
    TArray<FName> FlavorAspectNames = {
        TEXT("Umami"),
        TEXT("Salt"),
        TEXT("Sweet"),
        TEXT("Sour"),
        TEXT("Bitter"),
        TEXT("Spicy")
    };
    
    // Always show all 6 segments
    int32 TotalSegments = TOTAL_FLAVOR_ASPECTS;
    
    // CRITICAL: Preserve current RawValues before changing segment count
    TArray<float> PreservedRawValues;
    bool bSegmentCountChanged = (GetSegmentCount() != TotalSegments);
    if (!bSegmentCountChanged && ValueLayers.Num() > 0 && ValueLayers[0].RawValues.Num() == GetSegmentCount())
    {
        PreservedRawValues = ValueLayers[0].RawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfileWithFluctuations: Preserving %d current RawValues"), PreservedRawValues.Num());
    }
    
    // Set the number of segments
    if (!SetSegmentCount(TotalSegments))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishFlavorProfileWithFluctuations: Failed to set segment count to %d"), TotalSegments);
        return false;
    }
    
    // Restore preserved RawValues if segment count didn't change
    if (!bSegmentCountChanged && PreservedRawValues.Num() == TotalSegments && ValueLayers.Num() > 0)
    {
        ValueLayers[0].RawValues = PreservedRawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfileWithFluctuations: Restored preserved RawValues"));
    }
    
    // Prepare arrays for values and names
    TArray<float> Values;
    TArray<FString> DisplayNames;
    
    // Always set all 6 flavor aspects, even if they have zero values
    for (const FName& AspectName : FlavorAspectNames)
    {
        float AspectValue = Dish.GetTotalFlavorAspect(AspectName);
        Values.Add(AspectValue);
        DisplayNames.Add(AspectName.ToString());
    }
    
    // Calculate normalization scale based on maximum value
    const float SCALE_INCREMENT = 25.0f;
    const float MAX_SCALE = 300.0f;
    
    float MaxValue = 0.0f;
    for (float Value : Values)
    {
        if (Value > MaxValue)
        {
            MaxValue = Value;
        }
    }
    
    float NormalizationScale = FMath::Min(FMath::CeilToFloat(MaxValue / SCALE_INCREMENT) * SCALE_INCREMENT, MAX_SCALE);
    
    if (MaxValue > 0.0f && NormalizationScale < SCALE_INCREMENT)
    {
        NormalizationScale = SCALE_INCREMENT;
    }
    
    // Set the normalization scale for the radar chart with smooth animation
    SetNormalizationScaleAnimated(NormalizationScale, 0.5f, 18, EEasingFunc::ExpoOut);
    
    // Set the segment names
    if (!SetSegmentNames(DisplayNames))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishFlavorProfileWithFluctuations: Failed to set segment names"));
        return false;
    }
    
    // Set the values with fluctuations animation
    SetValuesWithFluctuations(Values, InFluctuationIntensity, NumFluctuations, InFluctuationDuration, InSettleDuration, 18, EEasingFunc::ExpoOut);
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfileWithFluctuations: Completed setup with %d segments"), TotalSegments);
    return true;
}

bool UPURadarChart::SetValuesFromDishTextureProfileWithFluctuations(
    const FPUDishBase& Dish,
    float InFluctuationIntensity,
    int32 NumFluctuations,
    float InFluctuationDuration,
    float InSettleDuration)
{
    // Always show all 6 texture aspects
    const int32 TOTAL_TEXTURE_ASPECTS = 6;
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfileWithFluctuations: Starting texture profile setup with fluctuations"));
    
    // Define all texture aspects in order
    TArray<FName> TextureAspectNames = {
        TEXT("Rich"),
        TEXT("Juicy"),
        TEXT("Tender"),
        TEXT("Chewy"),
        TEXT("Crispy"),
        TEXT("Crumbly")
    };
    
    // Always show all 6 segments
    int32 TotalSegments = TOTAL_TEXTURE_ASPECTS;
    
    // CRITICAL: Preserve current RawValues before changing segment count
    TArray<float> PreservedRawValues;
    bool bSegmentCountChanged = (GetSegmentCount() != TotalSegments);
    if (!bSegmentCountChanged && ValueLayers.Num() > 0 && ValueLayers[0].RawValues.Num() == GetSegmentCount())
    {
        PreservedRawValues = ValueLayers[0].RawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfileWithFluctuations: Preserving %d current RawValues"), PreservedRawValues.Num());
    }
    
    // Set the number of segments
    if (!SetSegmentCount(TotalSegments))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishTextureProfileWithFluctuations: Failed to set segment count to %d"), TotalSegments);
        return false;
    }
    
    // Restore preserved RawValues if segment count didn't change
    if (!bSegmentCountChanged && PreservedRawValues.Num() == TotalSegments && ValueLayers.Num() > 0)
    {
        ValueLayers[0].RawValues = PreservedRawValues;
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfileWithFluctuations: Restored preserved RawValues"));
    }
    
    // Prepare arrays for values and names
    TArray<float> Values;
    TArray<FString> DisplayNames;
    
    // Always set all 6 texture aspects, even if they have zero values
    for (const FName& AspectName : TextureAspectNames)
    {
        float AspectValue = Dish.GetTotalTextureAspect(AspectName);
        Values.Add(AspectValue);
        DisplayNames.Add(AspectName.ToString());
    }
    
    // Calculate normalization scale based on maximum value
    const float SCALE_INCREMENT = 25.0f;
    const float MAX_SCALE = 300.0f;
    
    float MaxValue = 0.0f;
    for (float Value : Values)
    {
        if (Value > MaxValue)
        {
            MaxValue = Value;
        }
    }
    
    float NormalizationScale = FMath::Min(FMath::CeilToFloat(MaxValue / SCALE_INCREMENT) * SCALE_INCREMENT, MAX_SCALE);
    
    if (MaxValue > 0.0f && NormalizationScale < SCALE_INCREMENT)
    {
        NormalizationScale = SCALE_INCREMENT;
    }
    
    // Set the normalization scale for the radar chart with smooth animation
    SetNormalizationScaleAnimated(NormalizationScale, 0.5f, 18, EEasingFunc::ExpoOut);
    
    // Set the segment names
    if (!SetSegmentNames(DisplayNames))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishTextureProfileWithFluctuations: Failed to set segment names"));
        return false;
    }
    
    // Set the values with fluctuations animation
    SetValuesWithFluctuations(Values, InFluctuationIntensity, NumFluctuations, InFluctuationDuration, InSettleDuration, 18, EEasingFunc::ExpoOut);
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfileWithFluctuations: Completed setup with %d segments"), TotalSegments);
    return true;
}

void UPURadarChart::InitializeSegments()
{
    // Clear existing segments
    ChartStyle.Segments.Empty();

    // Add minimum number of segments
    for (int32 i = 0; i < MinSegmentCount; ++i)
    {
        ChartStyle.Segments.AddZeroed();
        ChartStyle.Segments.Last().Name = FText::FromString(FString::Printf(TEXT("Segment %d"), i));
        ChartStyle.Segments.Last().SegmentColor = FLinearColor::White;
    }
}

void UPURadarChart::UpdateValueLayers()
{
    const int32 CurrentSegmentCount = ChartStyle.Segments.Num();

    // Update each value layer to match the current segment count
    for (FRadarChartValueLayer& Layer : ValueLayers)
    {
        // Clear existing values
        Layer.RawValues.Empty();

        // Add new values initialized to 0
        for (int32 i = 0; i < CurrentSegmentCount; ++i)
        {
            Layer.RawValues.Add(0.0f);
        }
    }
}

void UPURadarChart::SetValuesWithFluctuations(
    const TArray<float>& InValues,
    float InFluctuationIntensity,
    int32 NumFluctuations,
    float InFluctuationDuration,
    float InSettleDuration,
    uint8 Fps,
    TEnumAsByte<EEasingFunc::Type> Ease)
{
    // Validate input array size matches segment count
    if (InValues.Num() != ChartStyle.Segments.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesWithFluctuations: Input array size (%d) does not match segment count (%d)"), 
            InValues.Num(), ChartStyle.Segments.Num());
        return;
    }

    // Validate that we have valid values (not NaN or infinite)
    for (int32 i = 0; i < InValues.Num(); ++i)
    {
        if (!FMath::IsFinite(InValues[i]))
        {
            UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesWithFluctuations: Invalid value at index %d: %f"), i, InValues[i]);
            return;
        }
    }

    // Cancel any existing fluctuation animation
    CancelFluctuationAnimation();

    // Store parameters
    FinalTargetValues = InValues;
    FluctuationIntensity = FMath::Clamp(InFluctuationIntensity, 0.0f, 1.0f);
    TotalFluctuationSteps = FMath::Max(1, NumFluctuations);
    FluctuationDuration = FMath::Max(0.05f, InFluctuationDuration);
    SettleDuration = FMath::Max(0.05f, InSettleDuration);
    AnimationFps = Fps;
    AnimationEase = Ease;

    // Reset step counter
    CurrentFluctuationStep = 0;

    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesWithFluctuations: Starting fluctuation sequence with %d steps, intensity %.2f"), 
        TotalFluctuationSteps, FluctuationIntensity);

    // Start the first fluctuation step
    ProcessFluctuationStep();
}

void UPURadarChart::CancelFluctuationAnimation()
{
    // Clear the timer if it's active
    if (UWorld* World = GetWorld())
    {
        if (FluctuationTimerHandle.IsValid())
        {
            World->GetTimerManager().ClearTimer(FluctuationTimerHandle);
            UE_LOG(LogTemp, Log, TEXT("PURadarChart::CancelFluctuationAnimation: Cancelled ongoing fluctuation animation"));
        }
    }

    // Reset state
    CurrentFluctuationStep = 0;
    TotalFluctuationSteps = 0;
    FinalTargetValues.Empty();
}

bool UPURadarChart::IsFluctuationAnimationInProgress() const
{
    return FluctuationTimerHandle.IsValid() && TotalFluctuationSteps > 0;
}

void UPURadarChart::ProcessFluctuationStep()
{
    if (FinalTargetValues.Num() != ChartStyle.Segments.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::ProcessFluctuationStep: Final values array size mismatch"));
        CancelFluctuationAnimation();
        return;
    }

    // Get current values from the chart
    TArray<float> CurrentValues;
    if (ValueLayers.Num() > 0 && ValueLayers[0].RawValues.Num() == ChartStyle.Segments.Num())
    {
        CurrentValues = ValueLayers[0].RawValues;
    }
    else
    {
        // Initialize with zeros if no current values
        CurrentValues.SetNumZeroed(ChartStyle.Segments.Num());
    }

    TArray<float> TargetValues;
    float AnimationDuration;

    if (CurrentFluctuationStep < TotalFluctuationSteps)
    {
        // Generate random fluctuation values
        TargetValues = GenerateFluctuationValues(FinalTargetValues, FluctuationIntensity);
        AnimationDuration = FluctuationDuration;
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::ProcessFluctuationStep: Step %d/%d - Fluctuating"), 
            CurrentFluctuationStep + 1, TotalFluctuationSteps);
    }
    else
    {
        // Final step: settle on target values
        TargetValues = FinalTargetValues;
        AnimationDuration = SettleDuration;
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::ProcessFluctuationStep: Final step - Settling on target values"));
    }

    // Animate to the target values
    SetValuesAnimated(TargetValues, AnimationDuration, AnimationFps, AnimationEase);

    // Schedule next step
    CurrentFluctuationStep++;
    
    if (CurrentFluctuationStep <= TotalFluctuationSteps)
    {
        // Calculate delay: use the animation duration plus a small buffer
        float Delay = AnimationDuration + 0.05f;
        
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                FluctuationTimerHandle,
                this,
                &UPURadarChart::ProcessFluctuationStep,
                Delay,
                false
            );
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("PURadarChart::ProcessFluctuationStep: No valid world, cannot schedule next step"));
            CancelFluctuationAnimation();
        }
    }
    else
    {
        // Animation sequence complete
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::ProcessFluctuationStep: Fluctuation sequence complete"));
        
        // Broadcast the completion delegate
        OnFluctuationAnimationComplete.Broadcast();
        
        // Call the Blueprint implementable event
        OnFluctuationAnimationCompleteEvent();
        
        CancelFluctuationAnimation();
    }
}

TArray<float> UPURadarChart::GenerateFluctuationValues(const TArray<float>& FinalValues, float Intensity)
{
    TArray<float> FluctuationValues;
    FluctuationValues.Reserve(FinalValues.Num());

    for (int32 i = 0; i < FinalValues.Num(); ++i)
    {
        float FinalValue = FinalValues[i];
        
        // Calculate fluctuation range based on intensity
        // For zero values, allow positive fluctuation
        // For non-zero values, fluctuate around the value
        float FluctuationRange;
        if (FinalValue <= 0.0f)
        {
            // For zero values, allow fluctuation up to intensity * 50 (reasonable max)
            FluctuationRange = Intensity * 50.0f;
        }
        else
        {
            // For non-zero values, fluctuate by percentage of the value
            FluctuationRange = FinalValue * Intensity;
        }

        // Generate random fluctuation: -Range to +Range
        float RandomOffset = FMath::RandRange(-FluctuationRange, FluctuationRange);
        float FluctuatedValue = FinalValue + RandomOffset;

        // Clamp to ensure non-negative (radar chart values shouldn't be negative)
        FluctuatedValue = FMath::Max(0.0f, FluctuatedValue);

        FluctuationValues.Add(FluctuatedValue);
    }

    return FluctuationValues;
}

UPURadarChart* UPURadarChart::FindRadarChartInWidget(UWidget* ParentWidget)
{
    if (!ParentWidget)
    {
        return nullptr;
    }

    // Try to cast the parent widget directly
    if (UPURadarChart* RadarChart = Cast<UPURadarChart>(ParentWidget))
    {
        return RadarChart;
    }

    // If it's a UserWidget, search in its widget tree
    if (UUserWidget* UserWidget = Cast<UUserWidget>(ParentWidget))
    {
        if (UWidgetTree* WidgetTree = UserWidget->WidgetTree)
        {
            // Search all widgets in the tree
            TArray<UWidget*> AllWidgets;
            WidgetTree->GetAllWidgets(AllWidgets);
            
            for (UWidget* Widget : AllWidgets)
            {
                if (UPURadarChart* RadarChart = Cast<UPURadarChart>(Widget))
                {
                    return RadarChart;
                }
            }
        }
    }

    return nullptr;
}

bool UPURadarChart::AreAllRadarChartsAnimationComplete(const TArray<UPURadarChart*>& RadarCharts)
{
    // If array is empty, consider it "complete" (nothing to wait for)
    if (RadarCharts.Num() == 0)
    {
        return true;
    }

    // Check each radar chart - all must be complete
    for (UPURadarChart* RadarChart : RadarCharts)
    {
        if (RadarChart && RadarChart->IsFluctuationAnimationInProgress())
        {
            // At least one is still animating
            return false;
        }
    }

    // All are complete (or null)
    return true;
} 