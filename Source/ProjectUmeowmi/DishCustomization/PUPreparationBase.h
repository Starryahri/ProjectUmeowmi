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

// Aspect type enum - determines if modifier affects flavor or texture
UENUM(BlueprintType)
enum class EAspectType : uint8
{
    Flavor,
    Texture
};

// Aspect modifier definition
USTRUCT(BlueprintType)
struct FAspectModifier
{
    GENERATED_BODY()

    // Whether this modifier affects flavor or texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    EAspectType AspectType = EAspectType::Flavor;

    // Name of the aspect to modify (e.g., "Umami", "Sweet", "Rich", "Tender")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FName AspectName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    EModificationType ModificationType = EModificationType::Additive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    float ModificationValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FText Description;

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
    UTexture2D* PreviewTexture;

    // Name Modification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Naming")
    FText NamePrefix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Naming")
    FText NameSuffix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Naming")
    bool OverridesBaseName = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Naming", meta = (EditCondition = "OverridesBaseName"))
    FText SpecialName;

    // Aspect Modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preparation|Aspects")
    TArray<FAspectModifier> AspectModifiers;

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
    void ApplyModifiers(FFlavorAspects& FlavorAspects, FTextureAspects& TextureAspects) const;
    void RemoveModifiers(FFlavorAspects& FlavorAspects, FTextureAspects& TextureAspects) const;
}; 