#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUIngredientBase.h"
#include "PUDishBase.generated.h"

/** Legacy stage kinds — used by widget navigation and camera/plating hooks until data pipeline fully replaces hardcoded flows. */
UENUM(BlueprintType)
enum class EDishCustomizationStageType : uint8
{
    Planning     UMETA(DisplayName = "Planning"),
    Cooking      UMETA(DisplayName = "Cooking"),
    Plating      UMETA(DisplayName = "Plating"),
    Ending       UMETA(DisplayName = "Ending")
};

/** Shell workspace layout for this pipeline step (see DishCustomizationRoadmap Phase 0–3). */
UENUM(BlueprintType)
enum class EDishCustomizationWorkspaceMode : uint8
{
    Gather       UMETA(DisplayName = "Gather (counter grid + pantry)"),
    RailVignette UMETA(DisplayName = "Rail + stage vignette"),
};

/** One ordered step in a dish customization pipeline (`FPUDishBase::CustomizationStages`). */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUDishCustomizationStageDescriptor
{
    GENERATED_BODY()

    /** Stage type identity for modules / routing — use hierarchical tags such as `Stage.Gather`, `Stage.Chopping` (same tag can appear once per pipeline order; disambiguate with index when needed). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (Categories = "Stage"))
    FGameplayTag StageId;

    /** Title for shell / banners. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
    FText StageDisplayName;

    /** When advancing from the previous pipeline step, skip the triptych/cover animation (e.g. Gather → first chop). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Transition")
    bool bSkipTriptychOnEnter = false;

    /** Shell-owned ingredient rail (`IngredientRailSlot`). When false, the rail is collapsed for vignette-only stages. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Shell")
    bool bIngredientRailVisible = true;

    /** Bridges existing camera / plating / planning behavior until shell replaces `GoToStage`. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
    EDishCustomizationStageType LegacyStageKind = EDishCustomizationStageType::Cooking;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
    EDishCustomizationWorkspaceMode WorkspaceMode = EDishCustomizationWorkspaceMode::RailVignette;

    /** Widget for this step (`UPUDishCustomizationWidget` subclass or shell slot content). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
    TSubclassOf<UUserWidget> StageWidgetClass;

    /** Pantry shows ingredients matching these tags (OR). Empty = no extra tag filter beyond unlock rules. Superseded when filling a typed slot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Pantry", meta = (Categories = "Ingredient"))
    FGameplayTagContainer PantryIngredientParentTags;

    /**
     * Per-slot Ingredient.Type tag for the ingredient rail / gather grid (index 0 = first slot).
     * Shorter array → remaining slots accept any ingredient. Example: Protein + MaxSlots 4 → slot 0 protein, slots 1–3 open.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Slots", meta = (Categories = "Ingredient.Type"))
    TArray<FGameplayTag> SlotRequiredTypeTags;

    /** Rail/gather slot count for this stage (1–12). When <= 0, uses max(4, instance count, SlotRequiredTypeTags count). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Slots", meta = (ClampMin = "0", ClampMax = "12"))
    int32 IngredientRailMaxSlots = 0;

    /** Optional gate — BP/gameplay can require this tag before advancing (Phase 5+). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Advance", meta = (Categories = "Dish"))
    FGameplayTag AdvanceGateTag;
};

// Internal struct to track ingredient instances
USTRUCT(BlueprintType)
struct FIngredientInstance
{
    GENERATED_BODY()

    FIngredientInstance()
        : InstanceID(0)
        , Quantity(1)
        , PlacementPosition(FVector::ZeroVector)
        , PlacementRotation(FRotator::ZeroRotator)
        , PlatingPosition(FVector::ZeroVector)
        , PlatingRotation(FRotator::ZeroRotator)
        , PlatingScale(FVector::OneVector)
    {}

    // Unique identifier for this instance (never changes)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    int32 InstanceID;

    // The quantity of this ingredient
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    int32 Quantity;

    // The ingredient data with preparations already applied
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    FPUIngredientBase IngredientData;

    // Ingredient tag for easy template creation (redundant with IngredientData.IngredientTag but convenient)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient", meta = (Categories = "Ingredient"))
    FGameplayTag IngredientTag;

    // Preparations for easy template creation (redundant with IngredientData.ActivePreparations but convenient)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient", meta = (Categories = "Preparation"))
    FGameplayTagContainer Preparations;

    /** When set on a gather/rail slot template, pantry only shows ingredients matching these types (OR). Empty = any ingredient. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient", meta = (Categories = "Ingredient.Type"))
    FGameplayTagContainer RequiredIngredientTypes;

    // Optional: Placement data for this instance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    FVector PlacementPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient")
    FRotator PlacementRotation;

    // Plating data for this instance (3D positioning on dish)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Plating")
    FVector PlatingPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Plating")
    FRotator PlatingRotation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Plating")
    FVector PlatingScale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Plating")
    bool bIsPlated = false;

    // Time and Temperature values (0.0 to 1.0)
    // Time: 0.0 = None, 0.33 = Low, 0.66 = Mid, 1.0 = Long
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Cooking", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float TimeValue = 0.0f;

    // Temperature: 0.0 = Raw, 0.33 = Low, 0.66 = Med, 1.0 = Hot
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient|Cooking", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float TemperatureValue = 0.0f;
};

/** One entry per mesh/liquid on plate - supports multiple meshes per InstanceID (e.g. Quantity > 1). */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUPlatingEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InstanceID = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Position = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Scale = FVector::OneVector;
    /** If true, this entry is a liquid (Niagara fill) rather than a mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsLiquid = false;
    /** Per-piece world transforms for chopped/minced ingredients. When non-empty, each procedural mesh piece is placed at these transforms. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FTransform> ChoppedPieceTransforms;
};

USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUDishBase : public FTableRowBase
{
    GENERATED_BODY()

public:
    FPUDishBase();

    bool HasCustomizationPipeline() const { return CustomizationStages.Num() > 0; }

    // Basic Identification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic", meta = (Categories = "Dish"))
    FGameplayTag DishTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic")
    FName DishName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic")
    FText DisplayName;

    /** Recipe description shown in the journal (e.g. narrative or cooking notes) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Basic")
    FText Description;

    // Visual Representation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Visual")
    TSoftObjectPtr<UTexture2D> PreviewTexture;

    /** Texture shown in the journal recipe book (e.g. dish illustration on the right page) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Visual")
    TSoftObjectPtr<UTexture2D> JournalTexture;

    /** 3D mesh for the dish container during plating (bowl, plate, etc.) - swapped onto the cooking station */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Visual")
    TSoftObjectPtr<UStaticMesh> DishMesh;

    // Data Tables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Data")
    TSoftObjectPtr<UDataTable> IngredientDataTable;

    // Array of ingredient instances in the dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Ingredients")
    TArray<FIngredientInstance> IngredientInstances;

    /** One entry per mesh on plate - supports multiple meshes per InstanceID. Captured from SpawnedIngredientMeshes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Plating")
    TArray<FPUPlatingEntry> PlatingEntries;

    /** World position of dish surface center when plating was captured. Used as origin for ingredient placement when copying to preview. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Plating")
    FVector PlatingDishCenter = FVector::ZeroVector;

    /**
     * Optional ordered customization pipeline. When non-empty, prefer this for stage order and IDs (`Stage.*` tags via each row's StageId)
     * over chaining separate `PUDishCustomizationWidget` Blueprint classes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Customization Pipeline")
    TArray<FPUDishCustomizationStageDescriptor> CustomizationStages;

    // Tags associated with this dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Tags", meta = (Categories = "Dish"))
    FGameplayTagContainer DishTags;

    // Custom name for the dish
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish|Naming")
    FText CustomName;

    // Get the total value for a specific flavor aspect across all ingredients
    float GetTotalFlavorAspect(const FName& AspectName) const;

    // Get the total value for a specific texture aspect across all ingredients
    float GetTotalTextureAspect(const FName& AspectName) const;

    // Check if the dish has a specific ingredient
    bool HasIngredient(const FGameplayTag& IngredientTag) const;

    // Get the current display name of the dish
    FText GetCurrentDisplayName() const;

    // Helper function to get an ingredient from the data table
    bool GetIngredient(const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient) const;

    // Helper function to get ingredient data for a specific instance
    bool GetIngredientForInstance(int32 InstanceIndex, FPUIngredientBase& OutIngredient) const;

    // Helper function to get all ingredients in the dish
    TArray<FPUIngredientBase> GetAllIngredients() const;

    // Helper function to get all ingredient instances (including IDs)
    TArray<FIngredientInstance> GetAllIngredientInstances() const;

    // Helper function to get the total quantity of all ingredients
    int32 GetTotalIngredientQuantity() const;

    // Helper function to get ingredient data for a specific instance ID
    bool GetIngredientForInstanceID(int32 InstanceID, FPUIngredientBase& OutIngredient) const;

    // Helper function to get ingredient instance by ID
    bool GetIngredientInstanceByID(int32 InstanceID, FIngredientInstance& OutInstance) const;

    // Helper function to find instance index by ID
    int32 FindInstanceIndexByID(int32 InstanceID) const;

    // Helper functions for easy access to common properties
    FGameplayTag GetIngredientTag(int32 InstanceID) const;
    FGameplayTagContainer GetPreparations(int32 InstanceID) const;
    int32 GetQuantity(int32 InstanceID) const;

    // Helper function to generate a new unique instance ID
    int32 GenerateNewInstanceID() const;

    // Plating-related functions (internal use only)
    bool HasPlatingData() const;
    void SetIngredientPlating(int32 InstanceID, const FVector& Position, const FRotator& Rotation, const FVector& Scale);
    void ClearIngredientPlating(int32 InstanceID);
    bool GetIngredientPlating(int32 InstanceID, FVector& OutPosition, FRotator& OutRotation, FVector& OutScale) const;

private:
    // Static counter for generating unique instance IDs
    static std::atomic<int32> GlobalInstanceCounter;

public:
    // Generate a globally unique instance ID
    static int32 GenerateUniqueInstanceID();
};

// Planning stage data - ingredients selected for cooking without quantities
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUPlanningData
{
    GENERATED_BODY()

    // Selected ingredients for this dish (without quantities)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planning Data")
    TArray<FPUIngredientBase> SelectedIngredients;

    // Target dish being planned
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planning Data")
    FPUDishBase TargetDish;

    // Planning stage completed flag
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planning Data")
    bool bPlanningCompleted = false;

    FPUPlanningData()
    {
        bPlanningCompleted = false;
    }
}; 