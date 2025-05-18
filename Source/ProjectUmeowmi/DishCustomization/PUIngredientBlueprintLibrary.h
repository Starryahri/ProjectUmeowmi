#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PUIngredientBase.h"
#include "PUIngredientBlueprintLibrary.generated.h"

/**
 * Blueprint Function Library for ingredient-related operations
 */
UCLASS()
class PROJECTUMEOWMI_API UPUIngredientBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Apply a preparation to an ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Preparation")
    static bool ApplyPreparation(UPARAM(ref) FPUIngredientBase& Ingredient, const FPUPreparationBase& Preparation);

    /** Remove a preparation from an ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Preparation")
    static bool RemovePreparation(UPARAM(ref) FPUIngredientBase& Ingredient, const FPUPreparationBase& Preparation);

    /** Check if an ingredient has a specific preparation */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Preparation")
    static bool HasPreparation(const FPUIngredientBase& Ingredient, const FGameplayTag& PreparationTag);

    /** Get the current display name of an ingredient (including preparation modifications) */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Preparation")
    static FText GetCurrentDisplayName(const FPUIngredientBase& Ingredient);

    /** Get the value of a specific property for an ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Properties")
    static float GetPropertyValue(const FPUIngredientBase& Ingredient, const FName& PropertyName);

    /** Set the value of a specific property for an ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Properties")
    static void SetPropertyValue(UPARAM(ref) FPUIngredientBase& Ingredient, const FName& PropertyName, float Value);

    /** Check if an ingredient has a specific property */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Properties")
    static bool HasProperty(const FPUIngredientBase& Ingredient, const FName& PropertyName);

    /** Get all properties that have a specific tag */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Properties")
    static TArray<FIngredientProperty> GetPropertiesByTag(const FPUIngredientBase& Ingredient, const FGameplayTag& Tag);

    /** Check if an ingredient has any properties with a specific tag */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Properties")
    static bool HasPropertiesWithTag(const FPUIngredientBase& Ingredient, const FGameplayTag& Tag);

    /** Get the total value of all properties with a specific tag */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Properties")
    static float GetTotalValueForTag(const FPUIngredientBase& Ingredient, const FGameplayTag& Tag);

    /** Get all special effects at a specific quantity */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Effects")
    static TArray<FGameplayTag> GetEffectsAtQuantity(const FPUIngredientBase& Ingredient, int32 Quantity);
}; 