#include "PUOrderBase.h"
#include "PUDishBlueprintLibrary.h"
#include "Engine/Engine.h"

// Success bands per grade: [low, high] ratio (e.g. 0.80 = 80% of target)
namespace
{
    constexpr float PerfectBandLow = 0.80f;
    constexpr float PerfectBandHigh = 1.20f;
    constexpr float GreatBandLow = 0.70f;
    constexpr float GreatBandHigh = 1.30f;
    constexpr float OkayBandLow = 0.60f;
    constexpr float OkayBandHigh = 1.40f;
    // Outside 60%-140% = Needs Improvement

    // Numeric grades for averaging: Perfect=4, Great=3, Okay=2, NeedsImprovement=1
    constexpr float GradePerfect = 4.0f;
    constexpr float GradeGreat = 3.0f;
    constexpr float GradeOkay = 2.0f;
    constexpr float GradeNeedsImprovement = 1.0f;

    // Satisfaction score mapping for storage (all > 0 so IsCompleted works)
    constexpr float ScorePerfect = 1.0f;
    constexpr float ScoreGreat = 0.75f;
    constexpr float ScoreOkay = 0.5f;
    constexpr float ScoreNeedsImprovement = 0.25f;

    float GetAspectGrade(float Ratio)
    {
        if (Ratio >= PerfectBandLow && Ratio <= PerfectBandHigh) return GradePerfect;
        if (Ratio >= GreatBandLow && Ratio <= GreatBandHigh) return GradeGreat;
        if (Ratio >= OkayBandLow && Ratio <= OkayBandHigh) return GradeOkay;
        return GradeNeedsImprovement;
    }
}

// Debug: order detail dumps (LogOrderDetails, validation/completion helpers). Off = no spam.
namespace
{
    constexpr bool bPU_LogOrderDishDebug = true;
}

FName FOrderAspectRequirement::GetAspectName() const
{
    // Always derive from enums - they are the source of truth. AspectName is only for Blueprint Break compatibility
    // and can be wrong when empty FName serializes as "None" or when loading old data.
    if (AspectType == EOrderAspectType::Flavor)
    {
        return PUAspectHelpers::FlavorAspectToName(FlavorAspect);
    }
    return PUAspectHelpers::TextureAspectToName(TextureAspect);
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
        const FName AspectName = Req.GetAspectName();
        float CurrentValue = (Req.AspectType == EOrderAspectType::Flavor)
            ? Dish.GetTotalFlavorAspect(AspectName)
            : Dish.GetTotalTextureAspect(AspectName);
        if (CurrentValue < Req.TargetValue)
        {
            return false;
        }
    }
    return true;
}

float FPUOrderBase::GetSatisfactionScore(const FPUDishBase& Dish) const
{
    // 1. Base ingredient gate: missing any base ingredient = Needs Improvement
    if (BaseDish.IngredientInstances.Num() > 0)
    {
        if (UPUDishBlueprintLibrary::IsDishSuspicious(Dish, BaseDish))
        {
            return ScoreNeedsImprovement;
        }
    }

    // 2. No aspect requirements = default to Okay
    if (TargetAspects.Num() == 0)
    {
        return ScoreOkay;
    }

    // 3. Per-aspect grade from success bands
    float GradeSum = 0.0f;
    bool bAnyNeedsImprovement = false;
    for (const FOrderAspectRequirement& Req : TargetAspects)
    {
        const FName AspectName = Req.GetAspectName();
        float CurrentValue = (Req.AspectType == EOrderAspectType::Flavor)
            ? Dish.GetTotalFlavorAspect(AspectName)
            : Dish.GetTotalTextureAspect(AspectName);
        float TargetValue = FMath::Max(0.01f, Req.TargetValue);
        float Ratio = CurrentValue / TargetValue;

        float Grade = GetAspectGrade(Ratio);
        GradeSum += Grade;
        if (Grade <= GradeNeedsImprovement)
        {
            bAnyNeedsImprovement = true;
        }
    }

    // 4. Average of grades
    float AvgGrade = GradeSum / static_cast<float>(TargetAspects.Num());

    // 5. Floor rule: any Needs Improvement caps overall at Okay
    float FinalGrade = bAnyNeedsImprovement ? FMath::Min(AvgGrade, GradeOkay) : AvgGrade;

    // 6. Map grade to satisfaction score (0.25-1.0)
    if (FinalGrade >= 3.5f) return ScorePerfect;
    if (FinalGrade >= 2.5f) return ScoreGreat;
    if (FinalGrade >= 1.5f) return ScoreOkay;
    return ScoreNeedsImprovement;
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
        UE_LOG(LogTemp, Display, TEXT("  Target %s %s: %.2f"), Req.AspectType == EOrderAspectType::Flavor ? TEXT("Flavor") : TEXT("Texture"), *Req.GetAspectName().ToString(), Req.TargetValue);
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
        const FName AspectName = Req.GetAspectName();
        float Val = (Req.AspectType == EOrderAspectType::Flavor)
            ? CompletedDish.GetTotalFlavorAspect(AspectName)
            : CompletedDish.GetTotalTextureAspect(AspectName);
        //UE_LOG(LogTemp,Display, TEXT("Final %s %s: %.2f"), Req.AspectType == EOrderAspectType::Flavor ? TEXT("Flavor") : TEXT("Texture"), *Req.AspectName.ToString(), Val);
    }
    
    //UE_LOG(LogTemp,Display, TEXT("==============================="));
} 