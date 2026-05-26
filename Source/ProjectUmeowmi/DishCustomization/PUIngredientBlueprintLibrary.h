#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PUIngredientBase.h"
#include "PUIngredientType.h"
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

    /** Get the current display name of an ingredient (including preparation modifications). */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Preparation")
    static FText GetCurrentDisplayName(const FPUIngredientBase& Ingredient, UDataTable* PreparationTable = nullptr);

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

    UFUNCTION(BlueprintPure, Category = "Ingredient|Visual|Cut Minigame")
    static UTexture2D* GetCutVisualTexture(const FPUIngredientBase& Ingredient, EPUIngredientCutVisualTier CutTier);

    /** Whole tier = white (full-color art). Sliced+ = AverageTintColor with optional saturation boost (matches ingredient slot prep visuals). */
    UFUNCTION(BlueprintPure, Category = "Ingredient|Visual|Cut Minigame", meta = (AdvancedDisplay = "SaturationMultiplier,bApplySaturationBoost"))
    static FLinearColor GetMinigameTintColorForCutTier(
        const FPUIngredientBase& Ingredient,
        EPUIngredientCutVisualTier CutTier,
        float SaturationMultiplier = 1.5f,
        bool bApplySaturationBoost = true);

    /** Same as GetMinigameTintColorForCutTier(..., Whole) — no average tint on whole / to-be-chopped preview art. */
    UFUNCTION(BlueprintPure, Category = "Ingredient|Visual|Cut Minigame")
    static FLinearColor GetMinigameTintColor(const FPUIngredientBase& Ingredient);

    /** HSV saturation boost used by ingredient slots and cut-minigame tier tints. */
    UFUNCTION(BlueprintPure, Category = "Ingredient|Color")
    static FLinearColor BoostColorSaturation(FLinearColor Color, float SaturationMultiplier = 1.5f);

    /** True when RequiredTypes is empty, or any ingredient type hierarchically matches any required type (OR). */
    UFUNCTION(BlueprintPure, Category = "Ingredient|Type")
    static bool IngredientMatchesTypeFilter(const FPUIngredientBase& Ingredient, const FGameplayTagContainer& RequiredTypes);

    /** True when FilterTags is empty, or IngredientTag matches any filter tag (hierarchical OR). */
    UFUNCTION(BlueprintPure, Category = "Ingredient|Type")
    static bool IngredientMatchesParentTagFilter(const FPUIngredientBase& Ingredient, const FGameplayTagContainer& FilterTags);

    /** Lookup a type metadata row by tag (exact row TypeTag match, then row name). */
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Type")
    static bool TryGetIngredientTypeRow(UDataTable* TypeDataTable, FGameplayTag TypeTag, FPUIngredientTypeBase& OutRow);

    /** Icon for a type tag from DT_IngredientTypes; nullptr when missing. */
    UFUNCTION(BlueprintPure, Category = "Ingredient|Type")
    static UTexture2D* GetTypeIconForTag(UDataTable* TypeDataTable, FGameplayTag TypeTag);

    /** First matching icon for any tag in RequiredTypes (slot hint). */
    UFUNCTION(BlueprintPure, Category = "Ingredient|Type")
    static UTexture2D* GetTypeIconForRequiredTypes(UDataTable* TypeDataTable, const FGameplayTagContainer& RequiredTypes);
}; 