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

/** Single aspect requirement: e.g. Salt >= 5, Crispy >= 3. */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FOrderAspectRequirement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Aspect")
    FName AspectName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Aspect", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float MinValue = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Aspect")
    EOrderAspectType AspectType = EOrderAspectType::Flavor;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Basic")
    FText OrderDescription;

    // Order Requirements
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Requirements")
    int32 MinIngredientCount = 3;

    /** One or more aspect requirements (flavor and/or texture). All must be met. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Requirements")
    TArray<FOrderAspectRequirement> TargetAspects;

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

    void LogCompletionDetails() const;

    // Completion data access
    bool IsCompleted() const { return FinalSatisfactionScore > 0.0f; }
    
    const FPUDishBase& GetCompletedDish() const { return CompletedDish; }
    
    float GetFinalSatisfactionScore() const { return FinalSatisfactionScore; }
}; 