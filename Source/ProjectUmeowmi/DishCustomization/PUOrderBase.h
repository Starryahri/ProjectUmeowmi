#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PUDishBase.h"
#include "PUOrderBase.generated.h"

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Requirements")
    FName TargetFlavorProperty = FName(TEXT("Saltiness"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Requirements")
    float MinFlavorValue = 5.0f;

    // Dialogue Integration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order|Dialogue")
    FText OrderDialogueText;

    // Validation methods
    bool ValidateDish(const FPUDishBase& Dish) const;

    float GetSatisfactionScore(const FPUDishBase& Dish) const;

    // Debug methods
    void LogOrderDetails() const;

    void LogValidationResults(const FPUDishBase& Dish) const;
}; 