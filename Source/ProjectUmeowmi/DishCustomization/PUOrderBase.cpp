#include "PUOrderBase.h"
#include "Engine/Engine.h"

// Debug output toggles (kept in code, but disabled by default to avoid log spam).
namespace
{
    constexpr bool bPU_LogOrderDishDebug = false;
}

FPUOrderBase::FPUOrderBase()
    : OrderID(NAME_None)
    , OrderDescription(FText::GetEmpty())
    , MinIngredientCount(3)
    , TargetFlavorProperty(FName(TEXT("Salt")))
    , MinFlavorValue(5.0f)
    , OrderDialogueText(FText::GetEmpty())
{
}

bool FPUOrderBase::ValidateDish(const FPUDishBase& Dish) const
{
    //UE_LOG(LogTemp,Log, TEXT("FPUOrderBase::ValidateDish - Starting validation for order: %s"), *OrderID.ToString());
    
    // Check ingredient count - sum up all quantities, not just unique types
    int32 CurrentIngredientCount = Dish.GetTotalIngredientQuantity();
    bool bIngredientCountValid = CurrentIngredientCount >= MinIngredientCount;
    
    //UE_LOG(LogTemp,Log, TEXT("FPUOrderBase::ValidateDish - Ingredient count: %d/%d (Required: %d) - Valid: %s"), 
    //    CurrentIngredientCount, MinIngredientCount, MinIngredientCount, bIngredientCountValid ? TEXT("YES") : TEXT("NO"));
    
    // Check flavor requirement
    float CurrentFlavorValue = Dish.GetTotalFlavorAspect(TargetFlavorProperty);
    bool bFlavorValid = CurrentFlavorValue >= MinFlavorValue;
    
    //UE_LOG(LogTemp,Log, TEXT("FPUOrderBase::ValidateDish - Flavor %s: %.2f/%.2f (Required: %.2f) - Valid: %s"), 
    //    *TargetFlavorProperty.ToString(), CurrentFlavorValue, MinFlavorValue, MinFlavorValue, bFlavorValid ? TEXT("YES") : TEXT("NO"));
    
    bool bOverallValid = bIngredientCountValid && bFlavorValid;
    
    //UE_LOG(LogTemp,Log, TEXT("FPUOrderBase::ValidateDish - Overall validation result: %s"), bOverallValid ? TEXT("PASS") : TEXT("FAIL"));
    
    return bOverallValid;
}

float FPUOrderBase::GetSatisfactionScore(const FPUDishBase& Dish) const
{
    //UE_LOG(LogTemp,Log, TEXT("FPUOrderBase::GetSatisfactionScore - Calculating satisfaction for order: %s"), *OrderID.ToString());
    
    float Score = 0.0f;
    
    // Ingredient count satisfaction (50% of score) - sum up all quantities
    int32 CurrentIngredientCount = Dish.GetTotalIngredientQuantity();
    float IngredientScore = FMath::Clamp(static_cast<float>(CurrentIngredientCount) / static_cast<float>(MinIngredientCount), 0.0f, 1.0f);
    Score += IngredientScore * 0.5f;
    
    //UE_LOG(LogTemp,Log, TEXT("FPUOrderBase::GetSatisfactionScore - Ingredient score: %.2f (Count: %d/%d)"), 
    //    IngredientScore, CurrentIngredientCount, MinIngredientCount);
    
    // Flavor satisfaction (50% of score)
    float CurrentFlavor = Dish.GetTotalFlavorAspect(TargetFlavorProperty);
    float FlavorScore = FMath::Clamp(CurrentFlavor / MinFlavorValue, 0.0f, 1.0f);
    Score += FlavorScore * 0.5f;
    
    //UE_LOG(LogTemp,Log, TEXT("FPUOrderBase::GetSatisfactionScore - Flavor score: %.2f (Value: %.2f/%.2f)"), 
    //    FlavorScore, CurrentFlavor, MinFlavorValue);
    
    //UE_LOG(LogTemp,Log, TEXT("FPUOrderBase::GetSatisfactionScore - Final satisfaction score: %.2f"), Score);
    
    return Score;
}

void FPUOrderBase::LogOrderDetails() const
{
    if (!bPU_LogOrderDishDebug)
    {
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("=== ORDER DETAILS ==="));
    //UE_LOG(LogTemp,Display, TEXT("Order ID: %s"), *OrderID.ToString());
    //UE_LOG(LogTemp,Display, TEXT("Description: %s"), *OrderDescription.ToString());
    //UE_LOG(LogTemp,Display, TEXT("Min Ingredients: %d"), MinIngredientCount);
    //UE_LOG(LogTemp,Display, TEXT("Target Flavor: %s"), *TargetFlavorProperty.ToString());
    //UE_LOG(LogTemp,Display, TEXT("Min Flavor Value: %.2f"), MinFlavorValue);
    //UE_LOG(LogTemp,Display, TEXT("Dialogue Text: %s"), *OrderDialogueText.ToString());
    //UE_LOG(LogTemp,Display, TEXT("==================="));
}

void FPUOrderBase::LogValidationResults(const FPUDishBase& Dish) const
{
    if (!bPU_LogOrderDishDebug)
    {
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("=== VALIDATION RESULTS ==="));
    
    // Log order details
    LogOrderDetails();
    
    // Log dish details - calculate total ingredient count
    int32 TotalIngredientCount = Dish.GetTotalIngredientQuantity();
    //UE_LOG(LogTemp,Display, TEXT("Dish Ingredients: %d (Total Quantity: %d)"), Dish.IngredientInstances.Num(), TotalIngredientCount);
    //UE_LOG(LogTemp,Display, TEXT("Dish Flavor %s: %.2f"), *TargetFlavorProperty.ToString(), Dish.GetTotalFlavorAspect(TargetFlavorProperty));
    
    // Log validation results
    bool bValid = ValidateDish(Dish);
    float Satisfaction = GetSatisfactionScore(Dish);
    
    //UE_LOG(LogTemp,Display, TEXT("Validation Result: %s"), bValid ? TEXT("PASS") : TEXT("FAIL"));
    //UE_LOG(LogTemp,Display, TEXT("Satisfaction Score: %.2f"), Satisfaction);
    //UE_LOG(LogTemp,Display, TEXT("========================="));
}

void FPUOrderBase::LogCompletionDetails() const
{
    if (!bPU_LogOrderDishDebug)
    {
        return;
    }

    if (!IsCompleted())
    {
        //UE_LOG(LogTemp,Display, TEXT("=== ORDER NOT COMPLETED ==="));
        return;
    }
    
    //UE_LOG(LogTemp,Display, TEXT("=== ORDER COMPLETION DETAILS ==="));
    //UE_LOG(LogTemp,Display, TEXT("Order ID: %s"), *OrderID.ToString());
    //UE_LOG(LogTemp,Display, TEXT("Final Satisfaction Score: %.2f"), FinalSatisfactionScore);
    
    // Log completed dish details
    //UE_LOG(LogTemp,Display, TEXT("Completed Dish:"));
    //UE_LOG(LogTemp,Display, TEXT("  - Total Ingredients: %d"), CompletedDish.IngredientInstances.Num());
    //UE_LOG(LogTemp,Display, TEXT("  - Total Quantity: %d"), CompletedDish.GetTotalIngredientQuantity());
    
    // Log each ingredient in the completed dish
    for (int32 i = 0; i < CompletedDish.IngredientInstances.Num(); i++)
    {
        const FIngredientInstance& Instance = CompletedDish.IngredientInstances[i];
        //UE_LOG(LogTemp,Display, TEXT("  - Ingredient %d: %s (Qty: %d)"), 
        //    i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity);
        
        // Log preparations if any
        if (Instance.IngredientData.ActivePreparations.Num() > 0)
        {
            TArray<FGameplayTag> PreparationTags;
            Instance.IngredientData.ActivePreparations.GetGameplayTagArray(PreparationTags);
            FString PrepString = TEXT("    Preparations: ");
            for (const FGameplayTag& PrepTag : PreparationTags)
            {
                PrepString += PrepTag.ToString() + TEXT(", ");
            }
            //UE_LOG(LogTemp,Display, TEXT("    %s"), *PrepString);
        }
    }
    
    // Log final flavor values
    if (!TargetFlavorProperty.IsNone())
    {
        float FinalFlavorValue = CompletedDish.GetTotalFlavorAspect(TargetFlavorProperty);
        //UE_LOG(LogTemp,Display, TEXT("Final %s Value: %.2f"), *TargetFlavorProperty.ToString(), FinalFlavorValue);
    }
    
    //UE_LOG(LogTemp,Display, TEXT("==============================="));
} 