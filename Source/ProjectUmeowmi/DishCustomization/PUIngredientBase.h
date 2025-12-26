#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUIngredientBase.generated.h"

// Forward declarations
class UTexture2D;
class UMaterialInterface;
class UDataTable;
class UStaticMesh;
struct FPUPreparationBase;

// Flavor aspects - the six basic flavors
// Range: 0.0 to 5.0, increments of 0.5
USTRUCT(BlueprintType)
struct FFlavorAspects
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flavor", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Umami = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flavor", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Salt = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flavor", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Sweet = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flavor", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Sour = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flavor", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Bitter = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flavor", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Spicy = 0.0f;
};

// Texture aspects - the six texture properties
// Range: 0.0 to 5.0, increments of 0.5
USTRUCT(BlueprintType)
struct FTextureAspects
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Rich = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Juicy = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Tender = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Chewy = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Crispy = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
    float Crumbly = 0.0f;
};

// Main ingredient struct
USTRUCT(BlueprintType)
struct FPUIngredientBase : public FTableRowBase
{
    GENERATED_BODY()

public:
    FPUIngredientBase();

    // Basic Identification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Basic", meta = (Categories = "Ingredient"))
    FGameplayTag IngredientTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Basic")
    FName IngredientName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Basic")
    FText DisplayName;

    // Visual Representation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    UTexture2D* PreviewTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    UTexture2D* PantryTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    TSoftObjectPtr<UMaterialInterface> MaterialInstance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    TSoftObjectPtr<UStaticMesh> IngredientMesh;

    // Flavor Aspects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Aspects")
    FFlavorAspects FlavorAspects;

    // Texture Aspects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Aspects")
    FTextureAspects TextureAspects;

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Preparation", meta = (Categories = "Preparation"))
    FGameplayTagContainer ActivePreparations;

    // Data Tables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Data")
    TSoftObjectPtr<UDataTable> PreparationDataTable;

    // Special Effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Effects")
    TMap<int32, FGameplayTagContainer> QuantitySpecialEffects;

    // Helper Functions
    // Get flavor aspect value by name
    float GetFlavorAspect(const FName& AspectName) const;
    // Get texture aspect value by name
    float GetTextureAspect(const FName& AspectName) const;
    // Set flavor aspect value by name
    void SetFlavorAspect(const FName& AspectName, float Value);
    // Set texture aspect value by name
    void SetTextureAspect(const FName& AspectName, float Value);
    // Get total flavor value (sum of all flavor aspects)
    float GetTotalFlavorValue() const;
    // Get total texture value (sum of all texture aspects)
    float GetTotalTextureValue() const;
    TArray<FGameplayTag> GetEffectsAtQuantity(int32 Quantity) const;

    // Preparation Functions
    bool ApplyPreparation(const FPUPreparationBase& Preparation);
    bool RemovePreparation(const FPUPreparationBase& Preparation);
    bool HasPreparation(const FGameplayTag& PreparationTag) const;
    FText GetCurrentDisplayName() const;
}; 