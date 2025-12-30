#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUIngredientBase.h"
#include "PUDishBase.generated.h"

// Internal struct to track ingredient instances
USTRUCT(BlueprintType)
struct FIngredientInstance
{
    GENERATED_BODY()

    FIngredientInstance()
        : InstanceID(0)
        , Quantity(1)
        , TimeValue(0.0f)
        , TemperatureValue(0.0f)
    {}

    // Unique identifier for this instance (never changes)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    int32 InstanceID;

    // The quantity of this ingredient
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    int32 Quantity;

    // The ingredient data with preparations already applied
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    FPUIngredientBase IngredientData;

    // Ingredient tag for easy template creation (redundant with IngredientData.IngredientTag but convenient)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient", meta = (Categories = "Ingredient"))
    FGameplayTag IngredientTag;

    // Preparations for easy template creation (redundant with IngredientData.ActivePreparations but convenient)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient", meta = (Categories = "Preparation"))
    FGameplayTagContainer Preparations;

    // Time and Temperature values for cooking (0.0 to 1.0)
    // Time: 0.0 = None, 0.33 = Low, 0.66 = Mid, 1.0 = Long
    // Temperature: 0.0 = Raw, 0.33 = Low, 0.66 = Med, 1.0 = Hot
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Cooking", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float TimeValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Cooking", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float TemperatureValue = 0.0f;

    // Optional: Placement data for this instance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    FVector PlacementPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    FRotator PlacementRotation;

    // Plating data for this instance (3D positioning on dish)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Plating")
    FVector PlatingPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Plating")
    FRotator PlatingRotation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Plating")
    FVector PlatingScale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Plating")
    bool bIsPlated = false;
};

USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUDishBase : public FTableRowBase
{
    GENERATED_BODY()

public:
    FPUDishBase();

    // Basic Identification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic", meta = (Categories = "Dish"))
    FGameplayTag DishTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic")
    FName DishName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic")
    FText DisplayName;

    // Visual Representation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Visual")
    TSoftObjectPtr<UTexture2D> PreviewTexture;

    // Data Tables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Data")
    TSoftObjectPtr<UDataTable> IngredientDataTable;

    // Array of ingredient instances in the dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Ingredients")
    TArray<FIngredientInstance> IngredientInstances;

    // Tags associated with this dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Tags", meta = (Categories = "Dish"))
    FGameplayTagContainer DishTags;

    // Custom name for the dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Naming")
    FText CustomName;

    // Get the total value for a specific flavor aspect across all ingredients
    float GetTotalFlavorAspect(const FName& AspectName) const;

    // Get the total value for a specific texture aspect across all ingredients
    float GetTotalTextureAspect(const FName& AspectName) const;

    // Check if the dish has a specific ingredient
    bool HasIngredient(const FGameplayTag& IngredientTag) const;

    // Get the current display name of the dish
    FText GetCurrentDisplayName() const;

    // Helper function to get an ingredient from the data table
    bool GetIngredient(const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient) const;

    // Helper function to get ingredient data for a specific instance
    bool GetIngredientForInstance(int32 InstanceIndex, FPUIngredientBase& OutIngredient) const;

    // Helper function to get all ingredients in the dish
    TArray<FPUIngredientBase> GetAllIngredients() const;

    // Helper function to get all ingredient instances (including IDs)
    TArray<FIngredientInstance> GetAllIngredientInstances() const;

    // Helper function to get the total quantity of all ingredients
    int32 GetTotalIngredientQuantity() const;

    // Helper function to get ingredient data for a specific instance ID
    bool GetIngredientForInstanceID(int32 InstanceID, FPUIngredientBase& OutIngredient) const;

    // Helper function to get ingredient instance by ID
    bool GetIngredientInstanceByID(int32 InstanceID, FIngredientInstance& OutInstance) const;

    // Helper function to find instance index by ID
    int32 FindInstanceIndexByID(int32 InstanceID) const;

    // Helper functions for easy access to common properties
    FGameplayTag GetIngredientTag(int32 InstanceID) const;
    FGameplayTagContainer GetPreparations(int32 InstanceID) const;
    int32 GetQuantity(int32 InstanceID) const;

    // Helper function to generate a new unique instance ID
    int32 GenerateNewInstanceID() const;

    // Plating-related functions (internal use only)
    bool HasPlatingData() const;
    void SetIngredientPlating(int32 InstanceID, const FVector& Position, const FRotator& Rotation, const FVector& Scale);
    void ClearIngredientPlating(int32 InstanceID);
    bool GetIngredientPlating(int32 InstanceID, FVector& OutPosition, FRotator& OutRotation, FVector& OutScale) const;

private:
    // Static counter for generating unique instance IDs
    static std::atomic<int32> GlobalInstanceCounter;

public:
    // Generate a globally unique instance ID
    static int32 GenerateUniqueInstanceID();
};

// Planning stage data - ingredients selected for cooking without quantities
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUPlanningData
{
    GENERATED_BODY()

    // Selected ingredients for this dish (without quantities)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planning Data")
    TArray<FPUIngredientBase> SelectedIngredients;

    // Target dish being planned
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planning Data")
    FPUDishBase TargetDish;

    // Planning stage completed flag
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planning Data")
    bool bPlanningCompleted = false;

    FPUPlanningData()
    {
        bPlanningCompleted = false;
    }
}; 