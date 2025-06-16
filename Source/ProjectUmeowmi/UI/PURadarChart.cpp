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
    // Validate the new segment count
    if (NewSegmentCount < MinSegmentCount || NewSegmentCount > MaxSegmentCount)
    {
        return false;
    }

    // Clear existing segments
    ChartStyle.Segments.Empty();

    // Add new segments
    for (int32 i = 0; i < NewSegmentCount; ++i)
    {
        ChartStyle.Segments.AddZeroed();
        ChartStyle.Segments.Last().Name = FText::FromString(FString::Printf(TEXT("Segment %d"), i));
        ChartStyle.Segments.Last().SegmentColor = FLinearColor::White;
    }

    // Update value layers to match new segment count
    UpdateValueLayers();

    // Force a rebuild of the chart
    ForceRebuild();

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

    // Ensure we have at least one value layer
    if (ValueLayers.Num() == 0)
    {
        ValueLayers.AddZeroed();
    }

    // Set the values for the first layer
    SetValuesForLayer(0, InValues);
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
        // Add or update quantity for this ingredient
        IngredientQuantities.FindOrAdd(Instance.IngredientTag) += Instance.Quantity;
        
        // Get the display name and texture if we haven't already
        if (!IngredientNames.Contains(Instance.IngredientTag))
        {
            FPUIngredientBase Ingredient;
            if (Dish.GetIngredient(Instance.IngredientTag, Ingredient))
            {
                IngredientNames.Add(Instance.IngredientTag, Ingredient.DisplayName.ToString());
                if (Ingredient.PreviewTexture)
                {
                    IngredientTextures.Add(Instance.IngredientTag, Ingredient.PreviewTexture);
                }
                UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Found ingredient %s with texture %p"), 
                    *Ingredient.DisplayName.ToString(), 
                    Ingredient.PreviewTexture);
            }
        }
    }
    
    // Set the number of segments based on unique ingredients
    if (!SetSegmentCount(IngredientQuantities.Num()))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: Failed to set segment count"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("PURadarChart::SetValuesFromDishIngredients: Set up %d segments"), IngredientQuantities.Num());
    
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