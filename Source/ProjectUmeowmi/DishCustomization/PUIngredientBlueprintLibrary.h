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

    /** Get the value of a specific flavor aspect for an ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Aspects")
    static float GetFlavorAspect(const FPUIngredientBase& Ingredient, const FName& AspectName);

    /** Get the value of a specific texture aspect for an ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Aspects")
    static float GetTextureAspect(const FPUIngredientBase& Ingredient, const FName& AspectName);

    /** Set the value of a specific flavor aspect for an ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Aspects")
    static void SetFlavorAspect(UPARAM(ref) FPUIngredientBase& Ingredient, const FName& AspectName, float Value);

    /** Set the value of a specific texture aspect for an ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Aspects")
    static void SetTextureAspect(UPARAM(ref) FPUIngredientBase& Ingredient, const FName& AspectName, float Value);

    /** Get the total value of all flavor aspects */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Aspects")
    static float GetTotalFlavorValue(const FPUIngredientBase& Ingredient);

    /** Get the total value of all texture aspects */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Aspects")
    static float GetTotalTextureValue(const FPUIngredientBase& Ingredient);

    /** Get all special effects at a specific quantity */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Effects")
    static TArray<FGameplayTag> GetEffectsAtQuantity(const FPUIngredientBase& Ingredient, int32 Quantity);
}; 