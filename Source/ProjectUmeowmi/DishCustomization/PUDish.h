#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PUDishBase.h"

#include "PUDish.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIngredientAdded, const FGameplayTag&, IngredientTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIngredientRemoved, const FGameplayTag&, IngredientTag);

UCLASS()
class PROJECTUMEOWMI_API APUDish : public AActor
{
    GENERATED_BODY()

public:
    APUDish();

    // Ingredient management
    UFUNCTION(BlueprintCallable, Category = "Dish|Ingredients")
    FIngredientInstance AddIngredient(const FGameplayTag& IngredientTag);

    UFUNCTION(BlueprintCallable, Category = "Dish|Ingredients")
    bool RemoveIngredient(const FGameplayTag& IngredientTag);

    // Property queries
    UFUNCTION(BlueprintCallable, Category = "Dish|Properties")
    float GetTotalValueForProperty(const FName& PropertyName) const;

    UFUNCTION(BlueprintCallable, Category = "Dish|Properties")
    TArray<FIngredientProperty> GetPropertiesWithTag(const FGameplayTag& Tag) const;

    UFUNCTION(BlueprintCallable, Category = "Dish|Properties")
    float GetTotalValueForTag(const FGameplayTag& Tag) const;

    UFUNCTION(BlueprintCallable, Category = "Dish|Ingredients")
    bool HasIngredient(const FGameplayTag& IngredientTag) const;

    // Get the current display name of the dish
    UFUNCTION(BlueprintCallable, Category = "Dish")
    FText GetCurrentDisplayName() const;

    // Get all ingredients in the dish
    UFUNCTION(BlueprintCallable, Category = "Dish|Ingredients")
    TArray<FPUIngredientBase> GetAllIngredients() const;

    // Get all ingredient instances in the dish (including instance IDs)
    UFUNCTION(BlueprintCallable, Category = "Dish|Ingredients")
    TArray<FIngredientInstance> GetAllIngredientInstances() const;

    // Get a specific ingredient from the dish
    UFUNCTION(BlueprintCallable, Category = "Dish|Ingredients")
    bool GetIngredient(const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient) const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Dish|Events")
    FOnIngredientAdded OnIngredientAdded;

    UPROPERTY(BlueprintAssignable, Category = "Dish|Events")
    FOnIngredientRemoved OnIngredientRemoved;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dish")
    FPUDishBase DishData;
}; 