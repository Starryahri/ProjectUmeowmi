#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.generated.h"

// Forward declarations
class UTexture2D;

// Modification type enum
UENUM(BlueprintType)
enum class EModificationType : uint8
{
    Additive,
    Multiplicative
};

// Property modifier definition
USTRUCT(BlueprintType)
struct FPropertyModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    EIngredientPropertyType PropertyType = EIngredientPropertyType::Custom;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier", meta = (EditCondition = "PropertyType == EIngredientPropertyType::Custom"))
    FName CustomPropertyName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    EModificationType ModificationType = EModificationType::Additive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    float ModificationValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FGameplayTagContainer ModifierTags;

    // Helper function to get the property name
    FName GetPropertyName() const
    {
        if (PropertyType == EIngredientPropertyType::Custom)
        {
            return CustomPropertyName;
        }
        return FName(*UEnum::GetValueAsString(PropertyType));
    }

    // Helper function to apply the modification
    float ApplyModification(float BaseValue) const
    {
        switch (ModificationType)
        {
            case EModificationType::Additive:
                return BaseValue + ModificationValue;
            case EModificationType::Multiplicative:
                return BaseValue * ModificationValue;
            default:
                return BaseValue;
        }
    }

    // Helper function to remove the modification
    float RemoveModification(float ModifiedValue) const
    {
        switch (ModificationType)
        {
            case EModificationType::Additive:
                return ModifiedValue - ModificationValue;
            case EModificationType::Multiplicative:
                return ModifiedValue / ModificationValue;
            default:
                return ModifiedValue;
        }
    }
};

// Preparation types enum
UENUM(BlueprintType)
enum class EPreparationType : uint8
{
    Physical,
    Heat,
    Chemical,
    Custom UMETA(DisplayName = "Custom Preparation")
};

// Main preparation struct
USTRUCT(BlueprintType)
struct FPUPreparationBase : public FTableRowBase
{
    GENERATED_BODY()

public:
    FPUPreparationBase();

    // Basic Identification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Basic", meta = (Categories = "Prep"))
    FGameplayTag PreparationTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Basic")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Basic")
    FText Description;

    // Visual Representation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Visual")
    TSoftObjectPtr<UTexture2D> PreviewTexture;

    // Name Modification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Naming")
    FText NamePrefix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Naming")
    FText NameSuffix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Naming")
    bool OverridesBaseName = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Naming", meta = (EditCondition = "OverridesBaseName"))
    FText SpecialName;

    // Property Modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Properties")
    TArray<FPropertyModifier> PropertyModifiers;

    // Tags
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Tags")
    FGameplayTagContainer RequiredTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Tags")
    FGameplayTagContainer IncompatibleTags;

    // Special Effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Effects")
    FGameplayTagContainer SpecialEffects;

    // Helper Functions
    bool CanApplyToIngredient(const FGameplayTagContainer& IngredientTags) const;
    FText GetModifiedName(const FText& BaseName) const;
    void ApplyModifiers(TArray<FIngredientProperty>& Properties) const;
    void RemoveModifiers(TArray<FIngredientProperty>& Properties) const;
}; 