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
    static FIngredientInstance AddIngredient(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag, const FGameplayTagContainer& Preparations = FGameplayTagContainer());

    // Remove an ingredient from the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool RemoveIngredient(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag);

    // Remove a specific ingredient instance by index
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool RemoveIngredientInstance(UPARAM(ref) FPUDishBase& Dish, int32 InstanceIndex);

    // Remove a specific quantity from an ingredient instance
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool RemoveIngredientQuantity(UPARAM(ref) FPUDishBase& Dish, int32 InstanceIndex, int32 Quantity);

    // Increment the quantity of an ingredient in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool IncrementIngredientAmount(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag, int32 Amount = 1);

    // Decrement the quantity of an ingredient in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool DecrementIngredientAmount(UPARAM(ref) FPUDishBase& Dish, const FGameplayTag& IngredientTag, int32 Amount = 1);

    // Get the current quantity of an ingredient in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static int32 GetIngredientQuantity(const FPUDishBase& Dish, const FGameplayTag& IngredientTag);

    // Apply a preparation to a specific ingredient instance
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool ApplyPreparation(UPARAM(ref) FPUDishBase& Dish, int32 InstanceIndex, const FGameplayTag& PreparationTag);

    // Remove a preparation from a specific ingredient instance
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool RemovePreparation(UPARAM(ref) FPUDishBase& Dish, int32 InstanceIndex, const FGameplayTag& PreparationTag);

    // Get the number of instances for a specific ingredient
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static int32 GetIngredientInstanceCount(const FPUDishBase& Dish, const FGameplayTag& IngredientTag);

    // Get all instance indices for a specific ingredient
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static TArray<int32> GetInstanceIndicesForIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag);

    // Get all instance IDs for a specific ingredient
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static TArray<int32> GetInstanceIDsForIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag);

    // Apply a preparation to a specific ingredient instance by ID
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool ApplyPreparationByID(UPARAM(ref) FPUDishBase& Dish, int32 InstanceID, const FGameplayTag& PreparationTag);

    // Remove a preparation from a specific ingredient instance by ID
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool RemovePreparationByID(UPARAM(ref) FPUDishBase& Dish, int32 InstanceID, const FGameplayTag& PreparationTag);

    // Remove a specific ingredient instance by ID
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool RemoveIngredientInstanceByID(UPARAM(ref) FPUDishBase& Dish, int32 InstanceID);

    // Remove a specific quantity from an ingredient instance by ID
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool RemoveIngredientQuantityByID(UPARAM(ref) FPUDishBase& Dish, int32 InstanceID, int32 Quantity);

    // Increment the quantity of a specific ingredient instance by ID
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool IncrementIngredientQuantityByID(UPARAM(ref) FPUDishBase& Dish, int32 InstanceID, int32 Amount = 1);

    // Decrement the quantity of a specific ingredient instance by ID
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool DecrementIngredientQuantityByID(UPARAM(ref) FPUDishBase& Dish, int32 InstanceID, int32 Amount = 1);

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
    static bool GetDishFromDataTable(UDataTable* DishDataTable, UDataTable* IngredientDataTable, const FGameplayTag& DishTag, FPUDishBase& OutDish);

    // Get a random dish tag from available dishes
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static FGameplayTag GetRandomDishTag();

    // Get the current display name of the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static FText GetCurrentDisplayName(const FPUDishBase& Dish);

    // Get all ingredients in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static TArray<FPUIngredientBase> GetAllIngredients(const FPUDishBase& Dish);

    // Get all ingredient instances in the dish (including instance IDs)
    UFUNCTION(BlueprintCallable, Category = "Dish|Ingredients")
    static TArray<FIngredientInstance> GetAllIngredientInstances(const FPUDishBase& Dish);

    // Get a specific ingredient from the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static bool GetIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient);

    // Get the total quantity of all ingredients in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    static int32 GetTotalIngredientQuantity(const FPUDishBase& Dish);

    // Get ingredient data for a specific instance
    UFUNCTION(BlueprintCallable, Category = "Dish|Ingredients")
    static bool GetIngredientForInstance(const FPUDishBase& Dish, int32 InstanceIndex, FPUIngredientBase& OutIngredient);

    // Get ingredient data for a specific instance ID
    UFUNCTION(BlueprintCallable, Category = "Dish|Ingredients")
    static bool GetIngredientForInstanceID(const FPUDishBase& Dish, int32 InstanceID, FPUIngredientBase& OutIngredient);

    // Plating-related functions
    UFUNCTION(BlueprintCallable, Category = "Dish|Plating")
    static bool HasPlatingData(const FPUDishBase& Dish);

    UFUNCTION(BlueprintCallable, Category = "Dish|Plating")
    static void SetIngredientPlating(UPARAM(ref) FPUDishBase& Dish, int32 InstanceID, const FVector& Position, const FRotator& Rotation, const FVector& Scale);

    UFUNCTION(BlueprintCallable, Category = "Dish|Plating")
    static void ClearIngredientPlating(UPARAM(ref) FPUDishBase& Dish, int32 InstanceID);

    UFUNCTION(BlueprintCallable, Category = "Dish|Plating")
    static bool GetIngredientPlating(const FPUDishBase& Dish, int32 InstanceID, FVector& OutPosition, FRotator& OutRotation, FVector& OutScale);
}; 