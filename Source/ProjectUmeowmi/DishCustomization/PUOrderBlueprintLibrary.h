#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PUOrderBase.h"
#include "PUDishBase.h"
#include "PUOrderBlueprintLibrary.generated.h"

/**
 * Blueprint Function Library for order-related operations
 */
UCLASS()
class PROJECTUMEOWMI_API UPUOrderBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Validate if a dish meets the order requirements */
    UFUNCTION(BlueprintCallable, Category = "Order|Validation")
    static bool ValidateDish(const FPUOrderBase& Order, const FPUDishBase& Dish);

    /** Calculate satisfaction score for a dish against an order */
    UFUNCTION(BlueprintCallable, Category = "Order|Validation")
    static float GetSatisfactionScore(const FPUOrderBase& Order, const FPUDishBase& Dish);

    /** Log order details for debugging */
    UFUNCTION(BlueprintCallable, Category = "Order|Debug")
    static void LogOrderDetails(const FPUOrderBase& Order);

    /** Log validation results for debugging */
    UFUNCTION(BlueprintCallable, Category = "Order|Debug")
    static void LogValidationResults(const FPUOrderBase& Order, const FPUDishBase& Dish);

    /** Create a simple order with specified requirements */
    UFUNCTION(BlueprintCallable, Category = "Order|Creation")
    static FPUOrderBase CreateSimpleOrder(FName OrderID, FText Description, int32 MinIngredients, FName TargetFlavor, float MinFlavorValue, FText DialogueText);


}; 