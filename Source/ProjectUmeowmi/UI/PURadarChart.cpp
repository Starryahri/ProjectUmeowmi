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
    // Count unique ingredients and their quantities
    TMap<FGameplayTag, int32> IngredientCounts;
    TMap<FGameplayTag, FString> IngredientNames;
    
    for (const FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        // Increment count for this ingredient
        IngredientCounts.FindOrAdd(Instance.IngredientTag)++;
        
        // Get the display name if we haven't already
        if (!IngredientNames.Contains(Instance.IngredientTag))
        {
            FPUIngredientBase Ingredient;
            if (Dish.GetIngredient(Instance.IngredientTag, Ingredient))
            {
                IngredientNames.Add(Instance.IngredientTag, Ingredient.DisplayName.ToString());
            }
        }
    }
    
    // Set the number of segments based on unique ingredients
    if (!SetSegmentCount(IngredientCounts.Num()))
    {
        UE_LOG(LogTemp, Warning, TEXT("PURadarChart::SetValuesFromDishIngredients: Failed to set segment count"));
        return false;
    }
    
    // Prepare arrays for values and names
    TArray<float> Values;
    TArray<FString> DisplayNames;
    
    // Add values and names for each unique ingredient
    for (const auto& Pair : IngredientCounts)
    {
        Values.Add(static_cast<float>(Pair.Value));
        DisplayNames.Add(IngredientNames[Pair.Key]);
    }
    
    // Set the segment names
    SetSegmentNames(DisplayNames);
    
    // Set the values
    SetValues(Values);
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