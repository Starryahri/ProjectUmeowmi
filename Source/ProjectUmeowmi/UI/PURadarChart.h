#pragma once

#include "CoreMinimal.h"
#include "RadarChart.h"
#include "RadarChartStyle.h"
#include "RadarChartTypes.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "../DishCustomization/PUDishBase.h"
#include "PURadarChart.generated.h"

/**
 * Custom radar chart widget that extends URadarChart with additional functionality
 * for managing segments and values.
 */
UCLASS(BlueprintType, meta = (DisableNativeTick, ShortTooltip = "Project Umeowmi Radar Chart", ToolTip = "A radar chart widget that can display ingredient properties"))
class PROJECTUMEOWMI_API UPURadarChart : public URadarChart
{
    GENERATED_BODY()

public:
    UPURadarChart();

    /**
     * Shows or hides the icons in the radar chart.
     * @param bShow - Whether to show the icons
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    void ShowIcons(bool bShow = true);

    /**
     * Sets the number of segments in the radar chart.
     * @param NewSegmentCount - The new number of segments (minimum 3)
     * @return True if the segment count was successfully changed
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool SetSegmentCount(int32 NewSegmentCount);

    /**
     * Gets the current number of segments in the radar chart.
     * @return The current number of segments
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    int32 GetSegmentCount() const;

    /**
     * Sets the values of the radar chart from an array.
     * @param InValues - The array of values to set
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    void SetValues(const TArray<float>& InValues);

    /**
     * Sets the names of the segments from an array of strings.
     * @param InNames - The array of names to set
     * @return True if the names were successfully set
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool SetSegmentNames(const TArray<FString>& InNames);

    /**
     * Sets values from an ingredient's properties.
     * @param Ingredient - The ingredient to get values from
     * @return True if the values were successfully set
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool SetValuesFromIngredient(const FPUIngredientBase& Ingredient);

    /**
     * Sets values from a dish's ingredient quantities.
     * @param Dish - The dish to get ingredient quantities from
     * @return True if the values were successfully set
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool SetValuesFromDishIngredients(const FPUDishBase& Dish);

    /**
     * Sets values from a dish's flavor profile properties.
     * @param Dish - The dish to get flavor profile from
     * @return True if the values were successfully set
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool SetValuesFromDishFlavorProfile(const FPUDishBase& Dish);

protected:
    /** Minimum number of segments allowed in the radar chart */
    static constexpr int32 MinSegmentCount = 3;

    /** Maximum number of segments allowed in the radar chart */
    static constexpr int32 MaxSegmentCount = 12;

    /** Initializes default segments with the current segment count */
    void InitializeSegments();

    /** Updates the value layers to match the current segment count */
    void UpdateValueLayers();
}; 