#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUDishBase.h"
#include "PUOrderBase.generated.h"

/** Whether an order aspect requirement is for flavor or texture. */
UENUM(BlueprintType)
enum class EOrderAspectType : uint8
{
    Flavor,
    Texture
};

// EPUFlavorAspect and EPUTextureAspect are defined in PUIngredientBase.h (included via PUDishBase)

/** Single aspect requirement: e.g. Salt >= 5, Crispy >= 3. Use dropdowns to select aspect - no typing. */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FOrderAspectRequirement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Aspect")
    EOrderAspectType AspectType = EOrderAspectType::Flavor;

    /** Flavor aspect to target (when AspectType is Flavor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Aspect", meta = (EditCondition = "AspectType == EOrderAspectType::Flavor", EditConditionHides))
    EPUFlavorAspect FlavorAspect = EPUFlavorAspect::Salt;

    /** Texture aspect to target (when AspectType is Texture). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Aspect", meta = (EditCondition = "AspectType == EOrderAspectType::Texture", EditConditionHides))
    EPUTextureAspect TextureAspect = EPUTextureAspect::Crispy;

    /** Target value for this aspect. Player's dish is scored by ratio: PlayerValue / TargetValue (80-120% = perfect). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Aspect", meta = (DisplayName = "Target Value"))
    float TargetValue = 5.0f;

    /** Aspect name as FName - kept in sync with FlavorAspect/TextureAspect. For Blueprint Break node compatibility. */
    UPROPERTY(BlueprintReadOnly, Category = "Order|Aspect", meta = (DisplayName = "Aspect Name"))
    FName AspectName;

    /** Returns the aspect name as FName for use with dish/ingredient APIs. */
    FName GetAspectName() const;
};

USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUOrderBase : public FTableRowBase
{
    GENERATED_BODY()

public:
    FPUOrderBase();

    // Basic Identification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Basic")
    FName OrderID;

    /** Participant name of the dish giver who gave this order. Used to scope HasActiveOrder/OrderCompleted dialogue conditions per participant. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Basic")
    FName OrderGiverParticipantName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Basic")
    FText OrderDescription;

    // Order Requirements
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Requirements")
    int32 MinIngredientCount = 3;

    /** One or more aspect requirements (flavor and/or texture). All must be met. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Requirements")
    TArray<FOrderAspectRequirement> TargetAspects;

    /** Hints the player has discovered (e.g. from dialogue). Subset of TargetAspects. Use for partial radar chart display. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Requirements")
    TArray<FOrderAspectRequirement> DiscoveredHints;

    // Dialogue Integration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Dialogue")
    FText OrderDialogueText;

    // Base dish for this order
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Dish")
    FPUDishBase BaseDish;

    // Order completion data
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Completion")
    FPUDishBase CompletedDish; // The actual dish the player created

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Completion")
    float FinalSatisfactionScore = 0.0f;

    // Validation methods
    bool ValidateDish(const FPUDishBase& Dish) const;

    float GetSatisfactionScore(const FPUDishBase& Dish) const;

    // Debug methods
    void LogOrderDetails() const;

    void LogValidationResults(const FPUDishBase& Dish) const;

    /** Returns display text for the order including the dish giver (e.g. "Order from Yeoh: Make me something..."). Use in UI. */
    FText GetOrderDisplayText() const;

    void LogCompletionDetails() const;

    // Completion data access
    bool IsCompleted() const { return FinalSatisfactionScore > 0.0f; }
    
    const FPUDishBase& GetCompletedDish() const { return CompletedDish; }
    
    float GetFinalSatisfactionScore() const { return FinalSatisfactionScore; }
}; 