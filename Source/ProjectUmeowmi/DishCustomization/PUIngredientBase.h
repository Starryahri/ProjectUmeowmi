#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUIngredientBase.generated.h"

// Forward declarations
class UTexture2D;
class UMaterialInterface;
struct FPUPreparationBase;

// Forward declare enums from PUPreparationBase (to avoid circular dependency)
enum class EAspectType : uint8;
enum class EModificationType : uint8;

/** Visual cut state for strip minigames (whole uses Preview/Prepped textures). */
UENUM(BlueprintType)
enum class EPUIngredientCutVisualTier : uint8
{
    Whole   UMETA(DisplayName = "Whole"),
    Sliced  UMETA(DisplayName = "Sliced"),
    Chopped UMETA(DisplayName = "Chopped"),
    Minced  UMETA(DisplayName = "Minced")
};

// Time state enum for discrete mapping
UENUM(BlueprintType)
enum class ETimeState : uint8
{
    None = 0,   // 0.0
    Low = 1,    // 0.33
    Mid = 2,    // 0.66
    Long = 3    // 1.0
};

// Temperature state enum for discrete mapping
UENUM(BlueprintType)
enum class ETemperatureState : uint8
{
    Raw = 0,    // 0.0
    Low = 1,    // 0.33
    Med = 2,    // 0.66
    Hot = 3     // 1.0
};

/** Flavor aspects - select from dropdown. */
UENUM(BlueprintType)
enum class EPUFlavorAspect : uint8
{
    Umami UMETA(DisplayName = "Umami"),
    Salt UMETA(DisplayName = "Salt"),
    Sweet UMETA(DisplayName = "Sweet"),
    Sour UMETA(DisplayName = "Sour"),
    Bitter UMETA(DisplayName = "Bitter"),
    Spicy UMETA(DisplayName = "Spicy")
};

/** Texture aspects - select from dropdown. */
UENUM(BlueprintType)
enum class EPUTextureAspect : uint8
{
    Rich UMETA(DisplayName = "Rich"),
    Juicy UMETA(DisplayName = "Juicy"),
    Tender UMETA(DisplayName = "Tender"),
    Chewy UMETA(DisplayName = "Chewy"),
    Crispy UMETA(DisplayName = "Crispy"),
    Crumbly UMETA(DisplayName = "Crumbly")
};

/** Whether modifier affects flavor or texture. */
UENUM(BlueprintType)
enum class EPAspectCategory : uint8
{
    Flavor,
    Texture
};

/** Shared helpers to convert aspect enums to FName. Used by FTimeTempModifier, FAspectModifier, FOrderAspectRequirement. */
namespace PUAspectHelpers
{
    inline FName FlavorAspectToName(EPUFlavorAspect A)
    {
        switch (A)
        {
            case EPUFlavorAspect::Umami:  return FName(TEXT("Umami"));
            case EPUFlavorAspect::Salt:   return FName(TEXT("Salt"));
            case EPUFlavorAspect::Sweet:  return FName(TEXT("Sweet"));
            case EPUFlavorAspect::Sour:   return FName(TEXT("Sour"));
            case EPUFlavorAspect::Bitter: return FName(TEXT("Bitter"));
            case EPUFlavorAspect::Spicy:  return FName(TEXT("Spicy"));
            default: return FName(TEXT("Umami"));
        }
    }
    inline FName TextureAspectToName(EPUTextureAspect A)
    {
        switch (A)
        {
            case EPUTextureAspect::Rich:    return FName(TEXT("Rich"));
            case EPUTextureAspect::Juicy:   return FName(TEXT("Juicy"));
            case EPUTextureAspect::Tender:  return FName(TEXT("Tender"));
            case EPUTextureAspect::Chewy:   return FName(TEXT("Chewy"));
            case EPUTextureAspect::Crispy:  return FName(TEXT("Crispy"));
            case EPUTextureAspect::Crumbly: return FName(TEXT("Crumbly"));
            default: return FName(TEXT("Tender"));
        }
    }
}

// Time/Temperature modifier entry - defines how a specific aspect changes at a specific time/temp state
USTRUCT(BlueprintType)
struct FTimeTempModifier
{
    GENERATED_BODY()

    // Time state this modifier applies to (None, Low, Mid, Long)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time/Temp Modifier")
    ETimeState TimeState = ETimeState::None;

    // Temperature state this modifier applies to (Raw, Low, Med, Hot)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time/Temp Modifier")
    ETemperatureState TemperatureState = ETemperatureState::Raw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time/Temp Modifier")
    EPAspectCategory AspectCategory = EPAspectCategory::Flavor;

    /** Flavor aspect to modify (when AspectCategory is Flavor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time/Temp Modifier", meta = (EditCondition = "AspectCategory == EPAspectCategory::Flavor"))
    EPUFlavorAspect FlavorAspect = EPUFlavorAspect::Umami;

    /** Texture aspect to modify (when AspectCategory is Texture). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time/Temp Modifier", meta = (EditCondition = "AspectCategory == EPAspectCategory::Texture"))
    EPUTextureAspect TextureAspect = EPUTextureAspect::Tender;

    // How to apply the modification (0 = Additive, 1 = Multiplicative)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time/Temp Modifier")
    uint8 ModificationType = 0; // 0 = Additive, 1 = Multiplicative (matches EModificationType)

    // Modification value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time/Temp Modifier")
    float ModificationValue = 0.0f;

    FName GetAspectName() const;
};

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

    /** Journal / codex flavor text — short description or lore for UI (e.g. ingredients journal right page). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Journal", meta = (MultiLine = "true"))
    FText IngredientFlavorText;

    // Visual Representation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    UTexture2D* PreviewTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    UTexture2D* PantryTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    UTexture2D* PreppedTexture;

    /** Grayscale or neutral art for the chopping minigame — tinted with AverageTintColor at runtime. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual|Cut Minigame")
    UTexture2D* SlicedTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual|Cut Minigame")
    UTexture2D* ChoppedTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual|Cut Minigame")
    UTexture2D* MincedTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    TSoftObjectPtr<UMaterialInterface> MaterialInstance;

    /** Pre-computed average tint color from texture. Used in packaged builds where runtime texture sampling fails.
     *  Set in editor (or leave default white). Run "Bake Average Color" in editor to populate from texture.
     *  Applied as a multiply tint on Sliced/Chopped/Minced textures during the chop minigame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Visual")
    FLinearColor AverageTintColor = FLinearColor::White;

    /** Texture for the given cut tier (whole → PreppedTexture / PreviewTexture). */
    UTexture2D* GetCutVisualTexture(EPUIngredientCutVisualTier CutTier) const;

    /** Tint for cut-minigame food images (from AverageTintColor). */
    FLinearColor GetMinigameTintColor() const { return AverageTintColor; }

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

    // Special Effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Effects")
    TMap<int32, FGameplayTagContainer> QuantitySpecialEffects;

    // Time/Temperature Modifiers (per-ingredient overrides)
    // If empty, will use default rules. If populated, these override defaults for this ingredient.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Time/Temperature")
    TArray<FTimeTempModifier> TimeTemperatureModifiers;

    // Whether to use custom time/temp modifiers (if false, uses default rules)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Time/Temperature")
    bool bUseCustomTimeTempModifiers = false;

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
    /** Pass the station/component prep table at runtime (not stored on data-table rows). */
    FText GetCurrentDisplayName(const UDataTable* PreparationTable = nullptr) const;

    // Time/Temperature Functions
    // Calculate modified aspects based on time and temperature values (0.0 to 1.0)
    void CalculateTimeTempModifiedAspects(float TimeValue, float TemperatureValue, FFlavorAspects& OutFlavor, FTextureAspects& OutTexture) const;
    
    // Get modified flavor aspects (includes time/temp calculations)
    FFlavorAspects GetModifiedFlavorAspects(float TimeValue, float TemperatureValue) const;
    
    // Get modified texture aspects (includes time/temp calculations)
    FTextureAspects GetModifiedTextureAspects(float TimeValue, float TemperatureValue) const;

    // Helper: Map slider value (0.0-1.0) to discrete time state
    static ETimeState MapTimeValueToState(float TimeValue);
    
    // Helper: Map slider value (0.0-1.0) to discrete temperature state
    static ETemperatureState MapTemperatureValueToState(float TemperatureValue);
}; 