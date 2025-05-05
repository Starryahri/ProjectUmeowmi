#pragma once

#include "CoreMinimal.h"
#include "RadarChart.h"
#include "PURadarChart.generated.h"

/**
 * Custom Radar Chart widget that extends the base URadarChart functionality.
 * Allows for dynamic modification of segment amounts and provides additional customization options.
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTUMEOWMI_API UPURadarChart : public URadarChart
{
    GENERATED_BODY()

public:
    UPURadarChart();

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

protected:
    /** Minimum number of segments allowed in the radar chart */
    UPROPERTY(EditDefaultsOnly, Category = "Radar Chart")
    int32 MinSegmentCount = 3;

    /** Maximum number of segments allowed in the radar chart */
    UPROPERTY(EditDefaultsOnly, Category = "Radar Chart")
    int32 MaxSegmentCount = 32;

private:
    /** Initializes default segments with the current segment count */
    void InitializeSegments();

    /** Updates the value layers to match the current segment count */
    void UpdateValueLayers();
}; 