#include "PUOrderBase.h"
#include "Engine/Engine.h"

// Debug output toggles (kept in code, but disabled by default to avoid log spam).
namespace
{
    constexpr bool bPU_LogOrderDishDebug = true; // Set to true to see order generation in Output Log
}

FPUOrderBase::FPUOrderBase()
    : OrderID(NAME_None)
    , OrderDescription(FText::GetEmpty())
    , MinIngredientCount(3)
    , OrderDialogueText(FText::GetEmpty())
{
}

bool FPUOrderBase::ValidateDish(const FPUDishBase& Dish) const
{
    int32 CurrentIngredientCount = Dish.GetTotalIngredientQuantity();
    if (CurrentIngredientCount < MinIngredientCount)
    {
        return false;
    }

    for (const FOrderAspectRequirement& Req : TargetAspects)
    {
        float CurrentValue = (Req.AspectType == EOrderAspectType::Flavor)
            ? Dish.GetTotalFlavorAspect(Req.AspectName)
            : Dish.GetTotalTextureAspect(Req.AspectName);
        if (CurrentValue < Req.MinValue)
        {
            return false;
        }
    }
    return true;
}

float FPUOrderBase::GetSatisfactionScore(const FPUDishBase& Dish) const
{
    float IngredientScore = 0.5f;
    {
        int32 CurrentIngredientCount = Dish.GetTotalIngredientQuantity();
        IngredientScore = FMath::Clamp(static_cast<float>(CurrentIngredientCount) / static_cast<float>(FMath::Max(1, MinIngredientCount)), 0.0f, 1.0f) * 0.5f;
    }

    float AspectScore = 0.5f;
    if (TargetAspects.Num() > 0)
    {
        float Sum = 0.0f;
        for (const FOrderAspectRequirement& Req : TargetAspects)
        {
            float CurrentValue = (Req.AspectType == EOrderAspectType::Flavor)
                ? Dish.GetTotalFlavorAspect(Req.AspectName)
                : Dish.GetTotalTextureAspect(Req.AspectName);
            Sum += FMath::Clamp(CurrentValue / FMath::Max(0.01f, Req.MinValue), 0.0f, 1.0f);
        }
        AspectScore = (Sum / static_cast<float>(TargetAspects.Num())) * 0.5f;
    }

    return IngredientScore + AspectScore;
}

void FPUOrderBase::LogOrderDetails() const
{
    if (!bPU_LogOrderDishDebug)
    {
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("=== ORDER DETAILS ==="));
    UE_LOG(LogTemp, Display, TEXT("Order ID: %s"), *OrderID.ToString());
    UE_LOG(LogTemp, Display, TEXT("Dish Giver: %s"), OrderGiverParticipantName.IsNone() ? TEXT("(none)") : *OrderGiverParticipantName.ToString());
    UE_LOG(LogTemp, Display, TEXT("Description: %s"), *OrderDescription.ToString());
    UE_LOG(LogTemp, Display, TEXT("Min Ingredients: %d"), MinIngredientCount);
    UE_LOG(LogTemp, Display, TEXT("Base Dish: %s (Tag: %s)"), *BaseDish.DisplayName.ToString(), *BaseDish.DishTag.ToString());
    for (const FOrderAspectRequirement& Req : TargetAspects)
    {
        UE_LOG(LogTemp, Display, TEXT("  Target %s %s: min %.2f"), Req.AspectType == EOrderAspectType::Flavor ? TEXT("Flavor") : TEXT("Texture"), *Req.AspectName.ToString(), Req.MinValue);
    }
    UE_LOG(LogTemp, Display, TEXT("Dialogue Text: %s"), *OrderDialogueText.ToString());
    UE_LOG(LogTemp, Display, TEXT("==================="));
}

FText FPUOrderBase::GetOrderDisplayText() const
{
    if (OrderGiverParticipantName.IsNone())
    {
        return OrderDescription;
    }
    return FText::Format(
        FText::FromString(TEXT("Order from {0}: {1}")),
        FText::FromName(OrderGiverParticipantName),
        OrderDescription
    );
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
    //for (const FOrderAspectRequirement& Req : TargetAspects)
    //    UE_LOG(LogTemp,Display, TEXT("Dish %s %s: %.2f"), Req.AspectType == EOrderAspectType::Flavor ? TEXT("Flavor") : TEXT("Texture"), *Req.AspectName.ToString(),
    //        Req.AspectType == EOrderAspectType::Flavor ? Dish.GetTotalFlavorAspect(Req.AspectName) : Dish.GetTotalTextureAspect(Req.AspectName));
    
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
    
    // Log final aspect values
    for (const FOrderAspectRequirement& Req : TargetAspects)
    {
        float Val = (Req.AspectType == EOrderAspectType::Flavor)
            ? CompletedDish.GetTotalFlavorAspect(Req.AspectName)
            : CompletedDish.GetTotalTextureAspect(Req.AspectName);
        //UE_LOG(LogTemp,Display, TEXT("Final %s %s: %.2f"), Req.AspectType == EOrderAspectType::Flavor ? TEXT("Flavor") : TEXT("Texture"), *Req.AspectName.ToString(), Val);
    }
    
    //UE_LOG(LogTemp,Display, TEXT("==============================="));
} 