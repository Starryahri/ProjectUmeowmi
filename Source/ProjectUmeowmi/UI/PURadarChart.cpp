#include "PURadarChart.h"
#include "RadarChartStyle.h"
#include "RadarChartTypes.h"
#include "Engine/DataTable.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "../DishCustomization/PUDishBase.h"

UPURadarChart::UPURadarChart()
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
    // Set the number of segments based on the number of natural properties
    if (!SetSegmentCount(Ingredient.NaturalProperties.Num()))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromIngredient: Failed to set segment count"));
        return false;
    }

    // Get the values and display names from the ingredient's properties
    TArray<float> Values;
    TArray<FString> DisplayNames;
    
    for (const FIngredientProperty& Property : Ingredient.NaturalProperties)
    {
        // Get the display name from the property type
        FString DisplayName;
        if (Property.PropertyType == EIngredientPropertyType::Custom)
        {
            DisplayName = Property.CustomPropertyName.ToString();
        }
        else
        {
            DisplayName = UEnum::GetDisplayValueAsText(Property.PropertyType).ToString();
        }
        
        DisplayNames.Add(DisplayName);
        Values.Add(Property.Value);
    }

    // Set the segment names using the display names
    SetSegmentNames(DisplayNames);

    // Set the values
    SetValues(Values);
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
    
    // Debug: Log all unique ingredients
    for (const auto& Pair : IngredientQuantities)
    {
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Unique ingredient: %s (Qty: %d)"), 
            *Pair.Key.ToString(), Pair.Value);
    }
    
    // Set the number of segments based on unique ingredients
    int32 UniqueIngredientCount = IngredientQuantities.Num();
    if (UniqueIngredientCount == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: No valid ingredients found, using default segment count"));
        UniqueIngredientCount = 1; // Use at least 1 segment
    }
    
    if (!SetSegmentCount(UniqueIngredientCount))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: Failed to set segment count for %d ingredients"), UniqueIngredientCount);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Set up %d segments"), UniqueIngredientCount);
    
    // Prepare arrays for values and names
    TArray<float> Values;
    TArray<FString> DisplayNames;
    
    // Add values and names for each unique ingredient
    int32 SegmentIndex = 0;
    for (const auto& Pair : IngredientQuantities)
    {
        Values.Add(static_cast<float>(Pair.Value));
        
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
        DisplayNames.Add(DisplayName);
        
        // Set the segment's icon if we have a texture
        if (UTexture2D* const* FoundTexture = IngredientTextures.Find(Pair.Key))
        {
            if (UTexture2D* Texture = *FoundTexture)
            {
                // Set up the icon and its brush
                ChartStyle.Segments[SegmentIndex].Icon = Texture;
                ChartStyle.Segments[SegmentIndex].IconBrush.SetResourceObject(Texture);
                ChartStyle.Segments[SegmentIndex].IconBrush.DrawAs = ESlateBrushDrawType::Image;
                ChartStyle.Segments[SegmentIndex].IconBrush.TintColor = FSlateColor(FLinearColor::White);
                
                UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Set icon for segment %d (%s) with texture %p"), 
                    SegmentIndex, *DisplayName, Texture);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: Null texture for segment %d (%s)"), 
                    SegmentIndex, *DisplayName);
            }
        }
        
        SegmentIndex++;
    }
    
    // If we have no valid ingredients, add a default segment
    if (Values.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: No valid ingredients, adding default segment"));
        Values.Add(0.0f);
        DisplayNames.Add(TEXT("No Ingredients"));
    }
    
    // Set the segment names
    SetSegmentNames(DisplayNames);
    
    // Set the values
    SetValues(Values);
    
    // Make sure icons are enabled
    ShowIcons(true);
    
    // Force a rebuild of the chart to show the new icons
    ForceRebuild();
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Completed setup"));
    
    return true;
}

bool UPURadarChart::SetValuesFromDishFlavorProfile(const FPUDishBase& Dish)
{
    UE_LOG(LogTemp, Log, TEXT("=== PURadarChart::SetValuesFromDishFlavorProfile START ==="));
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Starting flavor profile analysis for dish with %d ingredients"), 
        Dish.IngredientInstances.Num());

    // Log initial chart state
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Initial chart state - Segments: %d, ValueLayers: %d"), 
        ChartStyle.Segments.Num(), ValueLayers.Num());

    // Validate that we have ingredients to analyze
    if (Dish.IngredientInstances.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: No ingredients found in dish"));
        return false;
    }

    // Initialize the full property sequence if not already done
    if (FullPropertySequence.Num() == 0)
    {
        FullPropertySequence = {
            EIngredientPropertyType::Sweetness,
            EIngredientPropertyType::Saltiness,
            EIngredientPropertyType::Sourness,
            EIngredientPropertyType::Bitterness,
            EIngredientPropertyType::Umami
        };
        
        // Build the position map
        PropertyPositionMap.Empty();
        for (int32 i = 0; i < FullPropertySequence.Num(); ++i)
        {
            PropertyPositionMap.Add(FullPropertySequence[i], i);
        }
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Initialized full property sequence with %d properties"), 
            FullPropertySequence.Num());
    }

    // Get values for all properties in the full sequence
    TArray<float> AllValues;
    TArray<FString> AllDisplayNames;
    
    for (EIngredientPropertyType PropertyType : FullPropertySequence)
    {
        // Get the property name using the same method as FIngredientProperty::GetPropertyName()
        FString PropertyName;
        FString EnumString = UEnum::GetValueAsString(PropertyType);
        if (EnumString.Split(TEXT("::"), nullptr, &PropertyName))
        {
            // Successfully extracted the value name
        }
        else
        {
            // Fallback to the full enum string
            PropertyName = EnumString;
        }

        // Use the display text for the segment name
        FString DisplayName = UEnum::GetDisplayValueAsText(PropertyType).ToString();
        AllDisplayNames.Add(DisplayName);

        // Get the total value for this property across all ingredients using the value name
        float TotalValue = Dish.GetTotalValueForProperty(FName(*PropertyName));
        AllValues.Add(TotalValue);

        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: %s (PropertyName: %s) = %.2f"), 
            *DisplayName, *PropertyName, TotalValue);
        
        // Additional debugging for 0 values
        if (TotalValue == 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: WARNING - Property '%s' has 0.0 value (likely missing from all ingredients)"), 
                *PropertyName);
        }
    }

    // Build active segments (only properties with values > 0)
    ActiveSegments.Empty();
    TArray<float> ActiveValues;
    TArray<FString> ActiveDisplayNames;
    
    for (int32 i = 0; i < FullPropertySequence.Num(); ++i)
    {
        if (AllValues[i] > 0.0f)
        {
            ActiveSegments.Add(FullPropertySequence[i]);
            ActiveValues.Add(AllValues[i]);
            ActiveDisplayNames.Add(AllDisplayNames[i]);
            
            UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Added active segment %s with value %.2f"), 
                *AllDisplayNames[i], AllValues[i]);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Skipped inactive segment %s (value: %.2f)"), 
                *AllDisplayNames[i], AllValues[i]);
        }
    }

    // Set the number of segments based on active segments
    int32 ActiveSegmentCount = ActiveSegments.Num();
    if (ActiveSegmentCount == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: No active segments found, using minimum segment count"));
        ActiveSegmentCount = 1;
        ActiveValues.Add(0.1f); // Small value for proper scaling
        ActiveDisplayNames.Add(TEXT("No Flavor Properties"));
    }

    if (!SetSegmentCount(ActiveSegmentCount))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Failed to set segment count"));
        return false;
    }

    // Set the segment names and values
    if (!SetSegmentNames(ActiveDisplayNames))
    {
        UE_LOG(LogTemp, Error, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Failed to set segment names"));
        return false;
    }

    SetValues(ActiveValues);

    // Disable icons for flavor profile (no ingredient icons needed)
    ShowIcons(false);

    // Force a rebuild to ensure the chart updates
    ForceRebuild();

    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Completed flavor profile setup with %d active segments"), ActiveSegmentCount);
    
    // Build a string representation of active values for logging
    FString ActiveValuesString = TEXT("[");
    for (int32 i = 0; i < ActiveValues.Num(); ++i)
    {
        if (i > 0) ActiveValuesString += TEXT(", ");
        ActiveValuesString += FString::Printf(TEXT("%.2f"), ActiveValues[i]);
    }
    ActiveValuesString += TEXT("]");
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Active values: %s"), *ActiveValuesString);
    
    // Additional debugging to verify the chart state
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: Chart state - Segments: %d, ValueLayers: %d"), 
        ChartStyle.Segments.Num(), ValueLayers.Num());
    
    if (ValueLayers.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishFlavorProfile: First layer has %d values"), 
            ValueLayers[0].RawValues.Num());
    }

    return true;
}

bool UPURadarChart::SetValuesFromDishTextureProfile(const FPUDishBase& Dish)
{
    UE_LOG(LogTemp, Log, TEXT("=== PURadarChart::SetValuesFromDishTextureProfile START ==="));
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Starting texture profile analysis for dish with %d ingredients"), 
        Dish.IngredientInstances.Num());

    // Log initial chart state
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Initial chart state - Segments: %d, ValueLayers: %d"), 
        ChartStyle.Segments.Num(), ValueLayers.Num());

    // Validate that we have ingredients to analyze
    if (Dish.IngredientInstances.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishTextureProfile: No ingredients found in dish"));
        return false;
    }

    // Initialize the full property sequence if not already done
    if (FullPropertySequence.Num() == 0)
    {
        FullPropertySequence = {
            EIngredientPropertyType::Watery,
            EIngredientPropertyType::Firm,
            EIngredientPropertyType::Crunchy,
            EIngredientPropertyType::Creamy,
            EIngredientPropertyType::Chewy,
            EIngredientPropertyType::Crumbly
        };
        
        // Build the position map
        PropertyPositionMap.Empty();
        for (int32 i = 0; i < FullPropertySequence.Num(); ++i)
        {
            PropertyPositionMap.Add(FullPropertySequence[i], i);
        }
        
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Initialized full property sequence with %d properties"), 
            FullPropertySequence.Num());
    }

    // Get values for all properties in the full sequence
    TArray<float> AllValues;
    TArray<FString> AllDisplayNames;
    
    for (EIngredientPropertyType PropertyType : FullPropertySequence)
    {
        // Get the property name using the same method as FIngredientProperty::GetPropertyName()
        FString PropertyName;
        FString EnumString = UEnum::GetValueAsString(PropertyType);
        if (EnumString.Split(TEXT("::"), nullptr, &PropertyName))
        {
            // Successfully extracted the value name
        }
        else
        {
            // Fallback to the full enum string
            PropertyName = EnumString;
        }

        // Use the display text for the segment name
        FString DisplayName = UEnum::GetDisplayValueAsText(PropertyType).ToString();
        AllDisplayNames.Add(DisplayName);

        // Get the total value for this property across all ingredients using the value name
        float TotalValue = Dish.GetTotalValueForProperty(FName(*PropertyName));
        AllValues.Add(TotalValue);

        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: %s (PropertyName: %s) = %.2f"), 
            *DisplayName, *PropertyName, TotalValue);
        
        // Additional debugging for 0 values
        if (TotalValue == 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishTextureProfile: WARNING - Property '%s' has 0.0 value (likely missing from all ingredients)"), 
                *PropertyName);
        }
    }

    // Build active segments (only properties with values > 0)
    ActiveSegments.Empty();
    TArray<float> ActiveValues;
    TArray<FString> ActiveDisplayNames;
    
    for (int32 i = 0; i < FullPropertySequence.Num(); ++i)
    {
        if (AllValues[i] > 0.0f)
        {
            ActiveSegments.Add(FullPropertySequence[i]);
            ActiveValues.Add(AllValues[i]);
            ActiveDisplayNames.Add(AllDisplayNames[i]);
            
            UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Added active segment %s with value %.2f"), 
                *AllDisplayNames[i], AllValues[i]);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Skipped inactive segment %s (value: %.2f)"), 
                *AllDisplayNames[i], AllValues[i]);
        }
    }

    // Set the number of segments based on active segments
    int32 ActiveSegmentCount = ActiveSegments.Num();
    if (ActiveSegmentCount == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishTextureProfile: No active segments found, using minimum segment count"));
        ActiveSegmentCount = 1;
        ActiveValues.Add(0.1f); // Small value for proper scaling
        ActiveDisplayNames.Add(TEXT("No Texture Properties"));
    }

    if (!SetSegmentCount(ActiveSegmentCount))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Failed to set segment count"));
        return false;
    }

    // Set the segment names and values
    if (!SetSegmentNames(ActiveDisplayNames))
    {
        UE_LOG(LogTemp, Error, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Failed to set segment names"));
        return false;
    }

    SetValues(ActiveValues);

    // Disable icons for texture profile (no ingredient icons needed)
    ShowIcons(false);

    // Force a rebuild to ensure the chart updates
    ForceRebuild();

    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Completed texture profile setup with %d active segments"), ActiveSegmentCount);
    
    // Build a string representation of active values for logging
    FString ActiveValuesString = TEXT("[");
    for (int32 i = 0; i < ActiveValues.Num(); ++i)
    {
        if (i > 0) ActiveValuesString += TEXT(", ");
        ActiveValuesString += FString::Printf(TEXT("%.2f"), ActiveValues[i]);
    }
    ActiveValuesString += TEXT("]");
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Active values: %s"), *ActiveValuesString);
    
    // Additional debugging to verify the chart state
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: Chart state - Segments: %d, ValueLayers: %d"), 
        ChartStyle.Segments.Num(), ValueLayers.Num());
    
    if (ValueLayers.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishTextureProfile: First layer has %d values"), 
            ValueLayers[0].RawValues.Num());
    }

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