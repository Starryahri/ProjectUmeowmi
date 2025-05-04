#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PUIngredientBase.h"
#include "PUDishCustomizationData.generated.h"

USTRUCT(BlueprintType)
struct FDishCustomizationData : public FTableRowBase
{
    GENERATED_BODY()

public:
    // Basic dish information
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic")
    FName DishTemplateName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic")
    FText Description;

    // Available ingredients for this dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Ingredients")
    TArray<FPUIngredientBase> AvailableIngredients;

    // Quantity limits
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Quantity")
    int32 MaxTotalIngredients = 5;

    // Base price and images
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Visual")
    float BasePrice = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Visual")
    TArray<FSoftObjectPath> DishImages;

    // Transient data (not saved)
    UPROPERTY(Transient)
    TArray<FPUIngredientBase> CurrentIngredients;
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
    static bool LoadIngredient(const UDataTable* IngredientDataTable, const FName& IngredientName, FPUIngredientBase& OutIngredientData);

    // Populate a dish's available ingredients from the ingredient Data Table
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static bool PopulateDishIngredients(const UDataTable* IngredientDataTable, FDishCustomizationData& DishData);

    // Check if a dish customization is valid
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static bool ValidateDishCustomization(const FDishCustomizationData& DishData);

    // Get special effects for current ingredient quantities
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static TArray<FGameplayTag> GetActiveSpecialEffects(const FPUIngredientBase& IngredientData);

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