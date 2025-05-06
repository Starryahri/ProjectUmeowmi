#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUIngredientBase.generated.h"

// Forward declarations
class UTexture2D;
class UMaterialInterface;
class UDataTable;
struct FPUPreparationBase;

// Property types enum
UENUM(BlueprintType)
enum class EIngredientPropertyType : uint8
{
    Sweetness,
    Saltiness,
    Sourness,
    Bitterness,
    Umami,
    Spiciness,
    Thickness,
    Smoothness,
    Cohesion,
    Temperature,
    Custom UMETA(DisplayName = "Custom Property")
};

// Property definition with value and tags
USTRUCT(BlueprintType)
struct FIngredientProperty
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
    EIngredientPropertyType PropertyType = EIngredientPropertyType::Custom;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property", meta = (EditCondition = "PropertyType == EIngredientPropertyType::Custom"))
    FName CustomPropertyName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
    float Value = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
    FGameplayTagContainer PropertyTags;

    // Helper function to get the property name
    FName GetPropertyName() const
    {
        if (PropertyType == EIngredientPropertyType::Custom)
        {
            return CustomPropertyName;
        }
        return FName(*UEnum::GetValueAsString(PropertyType));
    }
};

// Preparation modifier
USTRUCT(BlueprintType)
struct FPreparationModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FName ModifierName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    TMap<FName, float> PropertyModifiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FGameplayTagContainer ModifierTags;
};

// Main ingredient struct
USTRUCT(BlueprintType)
struct FPUIngredientBase : public FTableRowBase
{
    GENERATED_BODY()

public:
    FPUIngredientBase();

    // Basic Identification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Basic")
    FGameplayTag IngredientTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Basic")
    FName IngredientName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Basic")
    FText DisplayName;

    // Visual Representation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    TSoftObjectPtr<UTexture2D> PreviewTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    TSoftObjectPtr<UMaterialInterface> MaterialInstance;

    // Natural Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Properties")
    TArray<FIngredientProperty> NaturalProperties;

    // Quantity Management
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Quantity")
    int32 MinQuantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Quantity")
    int32 MaxQuantity = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Quantity")
    int32 CurrentQuantity = 0;

    // Placement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Placement")
    TArray<FVector> DefaultPlacementPositions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Placement")
    TArray<FRotator> DefaultPlacementRotations;

    // Active Preparations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Preparation")
    FGameplayTagContainer ActivePreparations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Preparation")
    UDataTable* PreparationDataTable;

    // Special Effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Effects")
    TMap<int32, FGameplayTagContainer> QuantitySpecialEffects;

    // Helper Functions
    float GetPropertyValue(const FName& PropertyName) const;
    void SetPropertyValue(const FName& PropertyName, float Value);
    bool HasProperty(const FName& PropertyName) const;
    TArray<FIngredientProperty> GetPropertiesByTag(const FGameplayTag& Tag) const;
    bool HasPropertiesWithTag(const FGameplayTag& Tag) const;
    float GetTotalValueForTag(const FGameplayTag& Tag) const;
    TArray<FGameplayTag> GetEffectsAtQuantity(int32 Quantity) const;

    // Preparation Functions
    bool ApplyPreparation(const FPUPreparationBase& Preparation);
    bool RemovePreparation(const FPUPreparationBase& Preparation);
    bool HasPreparation(const FGameplayTag& PreparationTag) const;
    FText GetCurrentDisplayName() const;
}; 