#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PUDishBase.h"

#include "PUDishBlueprintLibrary.generated.h"

UCLASS()
class PROJECTUMEOWMI_API UPUDishBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Add an ingredient to the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool AddIngredient(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag);

    // Remove an ingredient from the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool RemoveIngredient(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag);

    // Increment the quantity of an ingredient in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool IncrementIngredientAmount(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag, int32 Amount = 1);

    // Decrement the quantity of an ingredient in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool DecrementIngredientAmount(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag, int32 Amount = 1);

    // Get the current quantity of an ingredient in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static int32 GetIngredientQuantity(const FPUDishBase& Dish, const FGameplayTag& IngredientTag);

    // Apply a preparation to an ingredient in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool ApplyPreparation(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag, const FGameplayTag& PreparationTag);

    // Remove a preparation from an ingredient in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool RemovePreparation(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag, const FGameplayTag& PreparationTag);

    // Get the total value for a specific property
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static float GetTotalValueForProperty(const FPUDishBase& Dish, const FName& PropertyName);

    // Get all properties that match a specific tag
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static TArray<FIngredientProperty> GetPropertiesWithTag(const FPUDishBase& Dish, const FGameplayTag& Tag);

    // Get the total value for all properties with a specific tag
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static float GetTotalValueForTag(const FPUDishBase& Dish, const FGameplayTag& Tag);

    // Check if the dish has a specific ingredient
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool HasIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag);

    // Get a dish from a data table by tag
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool GetDishFromDataTable(UDataTable* DishDataTable, const FGameplayTag& DishTag, FPUDishBase& OutDish);

    // Get a random dish tag from available dishes
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static FGameplayTag GetRandomDishTag();

    // Get the current display name of the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static FText GetCurrentDisplayName(const FPUDishBase& Dish);

    // Get all ingredients in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static TArray<FPUIngredientBase> GetAllIngredients(const FPUDishBase& Dish);

    // Get a specific ingredient from the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool GetIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient);
}; 