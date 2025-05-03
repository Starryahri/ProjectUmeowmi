#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PUIngredientData.h"
#include "PUDishCustomizationData.generated.h"

USTRUCT(BlueprintType)
struct FDishCustomizationData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    FName DishTemplateName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    TArray<FName> AvailableIngredientNames;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    int32 MaxTotalIngredients = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    float BasePrice = 0.0f;

    // Array of dish images where index represents ingredient quantity
    // Each image should show the dish with that many total ingredients
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    TArray<UTexture2D*> DishImages;

    // Reference to the actual ingredients (populated at runtime)
    UPROPERTY(Transient)
    TArray<FIngredientData> AvailableIngredients;
};

UCLASS()
class PROJECTUMEOWMI_API UPUDishCustomizationHelper : public UObject
{
    GENERATED_BODY()

public:
    // Load and validate a dish template from Data Table
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static bool LoadDishTemplate(const UDataTable* DishDataTable, const FName& DishName, FDishCustomizationData& OutDishData);

    // Load and validate an ingredient from Data Table
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static bool LoadIngredient(const UDataTable* IngredientDataTable, const FName& IngredientName, FIngredientData& OutIngredientData);

    // Populate a dish's available ingredients from the ingredient Data Table
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static bool PopulateDishIngredients(const UDataTable* IngredientDataTable, FDishCustomizationData& DishData);


    // Check if a dish customization is valid
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static bool ValidateDishCustomization(const FDishCustomizationData& DishData);

    // Get special effects for current ingredient quantities
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static TArray<FName> GetActiveSpecialEffects(const FIngredientData& IngredientData);

    // Reset a dish to its default state
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static void ResetDishToDefault(FDishCustomizationData& DishData);

    // Get all available ingredients for a dish
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static TArray<FName> GetAvailableIngredients(const FDishCustomizationData& DishData);

    // Check if an ingredient can be added to a dish
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static bool CanAddIngredient(const FDishCustomizationData& DishData, const FName& IngredientName);

    // Get the current total number of ingredients in a dish
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static int32 GetTotalIngredientCount(const FDishCustomizationData& DishData);

    // Get the appropriate dish image based on current ingredient quantity
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static TSoftObjectPtr<UTexture2D> GetCurrentDishImage(const FDishCustomizationData& DishData);
}; 