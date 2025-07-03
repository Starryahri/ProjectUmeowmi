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
        : Quantity(1)
    {}

    // The base ingredient tag
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient", meta = (Categories = "Ingredient"))
    FGameplayTag IngredientTag;

    // The preparations applied to this specific instance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient", meta = (Categories = "Prep"))
    FGameplayTagContainer Preparations;

    // The quantity of this ingredient
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    int32 Quantity;

    // Optional: Placement data for this instance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    FVector PlacementPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    FRotator PlacementRotation;
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
    UTexture2D* PreviewTexture;

    // Data Tables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Data")
    UDataTable* IngredientDataTable;

    // Array of ingredient instances in the dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Ingredients")
    TArray<FIngredientInstance> IngredientInstances;

    // Tags associated with this dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Tags", meta = (Categories = "Dish"))
    FGameplayTagContainer DishTags;

    // Custom name for the dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Naming")
    FText CustomName;

    // Get the total value for a specific property across all ingredients
    float GetTotalValueForProperty(const FName& PropertyName) const;

    // Get all properties that match a specific tag
    TArray<FIngredientProperty> GetPropertiesWithTag(const FGameplayTag& Tag) const;

    // Get the total value for all properties with a specific tag
    float GetTotalValueForTag(const FGameplayTag& Tag) const;

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

    // Helper function to get the total quantity of all ingredients
    int32 GetTotalIngredientQuantity() const;
}; 