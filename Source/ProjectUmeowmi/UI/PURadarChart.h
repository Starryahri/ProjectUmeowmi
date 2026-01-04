#pragma once

#include "CoreMinimal.h"
#include "RadarChart.h"
#include "RadarChartStyle.h"
#include "RadarChartTypes.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "../DishCustomization/PUDishBase.h"
#include "PURadarChart.generated.h"

/**
 * Delegate called when a fluctuation animation sequence completes.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFluctuationAnimationComplete);

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
     * Sets the values of the radar chart from an array with smooth animation.
     * @param InValues - The array of values to set
     * @param Duration - Duration of the animation in seconds (default: 1.0)
     * @param Fps - Frames per second for the animation (default: 18)
     * @param Ease - Easing function type (default: Linear)
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    void SetValuesAnimated(const TArray<float>& InValues, float Duration = 1.0f, uint8 Fps = 18, TEnumAsByte<EEasingFunc::Type> Ease = EEasingFunc::Linear);

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
     * Sets the values of the radar chart from an ingredient with time/temperature modifiers applied.
     * @param Ingredient The base ingredient data
     * @param TimeValue Time value (0.0 to 1.0)
     * @param TemperatureValue Temperature value (0.0 to 1.0)
     * @return True if successful, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool SetValuesFromIngredientWithTimeTemp(const FPUIngredientBase& Ingredient, float TimeValue, float TemperatureValue);

    /**
     * Sets the icon for a specific segment.
     * @param SegmentIndex - The index of the segment to set the icon for
     * @param IconTexture - The texture to use as the icon
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    void SetSegmentIcon(int32 SegmentIndex, UTexture2D* IconTexture);

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

    /**
     * Sets values from a dish's texture profile properties.
     * @param Dish - The dish to get texture profile from
     * @return True if the values were successfully set
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool SetValuesFromDishTextureProfile(const FPUDishBase& Dish);

    /**
     * Sets values from a dish's flavor profile with random fluctuations before settling.
     * @param Dish - The dish to get flavor profile from
     * @param InFluctuationIntensity - How much to vary from final values (0.0 to 1.0, default: 0.3)
     * @param NumFluctuations - Number of random fluctuation steps (default: 3)
     * @param InFluctuationDuration - Duration of each fluctuation animation in seconds (default: 0.2)
     * @param InSettleDuration - Duration of final settle animation in seconds (default: 0.5)
     * @return True if the values were successfully set
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool SetValuesFromDishFlavorProfileWithFluctuations(
        const FPUDishBase& Dish,
        float InFluctuationIntensity = 0.3f,
        int32 NumFluctuations = 3,
        float InFluctuationDuration = 0.2f,
        float InSettleDuration = 0.5f
    );

    /**
     * Sets values from a dish's texture profile with random fluctuations before settling.
     * @param Dish - The dish to get texture profile from
     * @param InFluctuationIntensity - How much to vary from final values (0.0 to 1.0, default: 0.3)
     * @param NumFluctuations - Number of random fluctuation steps (default: 3)
     * @param InFluctuationDuration - Duration of each fluctuation animation in seconds (default: 0.2)
     * @param InSettleDuration - Duration of final settle animation in seconds (default: 0.5)
     * @return True if the values were successfully set
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool SetValuesFromDishTextureProfileWithFluctuations(
        const FPUDishBase& Dish,
        float InFluctuationIntensity = 0.3f,
        int32 NumFluctuations = 3,
        float InFluctuationDuration = 0.2f,
        float InSettleDuration = 0.5f
    );

    /**
     * Sets the normalization scale with smooth animation.
     * @param InValue - The new normalization scale value
     * @param Duration - Duration of the animation in seconds (default: 1.0)
     * @param Fps - Frames per second for the animation (default: 18)
     * @param Ease - Easing function type (default: Linear)
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    void SetNormalizationScaleAnimated(float InValue, float Duration = 1.0f, uint8 Fps = 18, TEnumAsByte<EEasingFunc::Type> Ease = EEasingFunc::Linear);

    /**
     * Sets values with random fluctuations before settling on final values.
     * Creates a sequence: current -> random fluctuations -> final values.
     * @param InValues - The final array of values to settle on
     * @param FluctuationIntensity - How much to vary from final values (0.0 to 1.0, default: 0.3)
     * @param NumFluctuations - Number of random fluctuation steps (default: 3)
     * @param FluctuationDuration - Duration of each fluctuation animation in seconds (default: 0.2)
     * @param SettleDuration - Duration of final settle animation in seconds (default: 0.5)
     * @param Fps - Frames per second for animations (default: 18)
     * @param Ease - Easing function type (default: ExpoOut)
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    void SetValuesWithFluctuations(
        const TArray<float>& InValues,
        float FluctuationIntensity = 0.3f,
        int32 NumFluctuations = 3,
        float FluctuationDuration = 0.2f,
        float SettleDuration = 0.5f,
        uint8 Fps = 18,
        TEnumAsByte<EEasingFunc::Type> Ease = EEasingFunc::ExpoOut
    );

    /**
     * Cancels any ongoing fluctuation animation sequence.
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    void CancelFluctuationAnimation();

    /**
     * Checks if a fluctuation animation is currently in progress.
     * @return True if an animation is currently playing
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Chart")
    bool IsFluctuationAnimationInProgress() const;

    /**
     * Helper function to find a PURadarChart widget in a parent widget.
     * Useful for finding the radar chart from a parent widget blueprint.
     * @param ParentWidget - The parent widget to search in
     * @return The first PURadarChart found, or nullptr if not found
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Radar Chart|Helpers", meta = (CallInEditor = "true"))
    static class UPURadarChart* FindRadarChartInWidget(class UWidget* ParentWidget);

    /**
     * Gets a reference to this radar chart widget.
     * Useful for getting a reference in Blueprints when you have the widget but need to cast it.
     * @return A reference to this radar chart widget
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Radar Chart|Helpers")
    class UPURadarChart* GetRadarChart() { return this; }

    /**
     * Checks if all radar charts in the provided array have finished their animations.
     * Useful for waiting for multiple radar chart animations to complete.
     * @param RadarCharts - Array of radar chart widgets to check
     * @return True if all radar charts have finished their animations (or array is empty)
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Radar Chart|Helpers")
    static bool AreAllRadarChartsAnimationComplete(const TArray<class UPURadarChart*>& RadarCharts);

    /**
     * Event called when a fluctuation animation sequence completes.
     * Bind to this in Blueprints to execute code after the animation finishes.
     */
    UPROPERTY(BlueprintAssignable, Category = "Radar Chart|Events")
    FOnFluctuationAnimationComplete OnFluctuationAnimationComplete;

    /**
     * Blueprint event that can be overridden in widget blueprints.
     * Called when a fluctuation animation sequence completes.
     * Override this in your widget blueprint to handle the completion.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Radar Chart|Events")
    void OnFluctuationAnimationCompleteEvent();

protected:
    /** Minimum number of segments allowed in the radar chart */
    static const int32 MinSegmentCount = 1;
    
    /** Maximum number of segments allowed in the radar chart */
    static const int32 MaxSegmentCount = 12;

    /** Initializes default segments with the current segment count */
    void InitializeSegments();

    /** Updates the value layers to match the current segment count */
    void UpdateValueLayers();

    /** Internal function to process the next step in the fluctuation animation sequence */
    void ProcessFluctuationStep();

    /** Generates random fluctuation values based on final values and intensity */
    TArray<float> GenerateFluctuationValues(const TArray<float>& FinalValues, float Intensity);

private:
    /** Timer handle for fluctuation animation sequence */
    FTimerHandle FluctuationTimerHandle;

    /** Current step in the fluctuation sequence */
    int32 CurrentFluctuationStep;

    /** Total number of fluctuation steps */
    int32 TotalFluctuationSteps;

    /** Final values to settle on */
    TArray<float> FinalTargetValues;

    /** Parameters for fluctuation animation */
    float FluctuationIntensity;
    float FluctuationDuration;
    float SettleDuration;
    uint8 AnimationFps;
    TEnumAsByte<EEasingFunc::Type> AnimationEase;
}; 