#pragma once

#include "CoreMinimal.h"
#include "TalkingObject.h"
#include "../DishCustomization/PUOrderComponent.h"
#include "PUDishGiver.generated.h"

/**
 * Specialized talking object for characters who give dish requests
 */
UCLASS()
class PROJECTUMEOWMI_API APUDishGiver : public ATalkingObject
{
    GENERATED_BODY()

public:
    APUDishGiver();

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Order-related dialogue methods
    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    void GenerateOrderForDialogue();

    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    FText GetOrderDialogueText() const;

    // Dialogue-controlled order generation
    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    void GenerateAndGiveOrderToPlayer();

    // Override dialogue participant methods to include order data
    virtual bool CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const override;
    virtual FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const override;
    virtual bool GetBoolValue_Implementation(FName ValueName) const override;

    // Available dialogue conditions for order system:
    // - "HasActiveOrder": Returns true if player has an active order
    // - "OrderCompleted": Returns true if player has a completed order
    // - "NoActiveOrder": Returns true if player has no active order

    // Order access
    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    const FPUOrderBase& GetCurrentOrder() const;

    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    bool HasActiveOrder() const;

    // Order validation
    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    bool ValidateDish(const FPUDishBase& Dish) const;

    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    float GetSatisfactionScore(const FPUDishBase& Dish) const;

    // Order completion feedback
    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    void HandleOrderCompletion(AProjectUmeowmiCharacter* PlayerCharacter);

    // Clear completed order from player (called from dialogue)
    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    void ClearCompletedOrderFromPlayer();

    // Delayed clearing mechanism to prevent race conditions
    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    void MarkOrderForClearing();

    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    void ExecuteDelayedOrderClearing();

    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    FText GetOrderCompletionFeedback(AProjectUmeowmiCharacter* PlayerCharacter) const;

    // Order clearing (for dialogue control)
    // Helper function to set dialogue variables from order
    void SetDialogueVariablesFromOrder(const FPUOrderBase& Order);

    // Helper function to analyze completed dish and set dialogue variables
    void AnalyzeCompletedDish(const FPUOrderBase& CompletedOrder);

protected:
    // Order component - only dish givers have this
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Giver|Components")
    UPUOrderComponent* OrderComponent;

    // Weak reference to player character to prevent dangling references
    UPROPERTY()
    TWeakObjectPtr<AProjectUmeowmiCharacter> CachedPlayerCharacter;

    // Test boolean for dialogue conditions
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Test")
    bool bTestCondition = true;

    // Dialogue-accessible order variables (basic types only)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Orders")
    bool bHasOrderReady = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Orders")
    FText OrderDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Orders")
    int32 MinIngredientCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Orders")
    FText TargetFlavorProperty; // As text, not FName

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Orders")
    float MinFlavorValue = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Orders")
    FText OrderDialogueText;

    // Dialogue-accessible completion variables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    bool bHasCompletedDish = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    float CompletedDishSatisfaction = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    int32 CompletedDishIngredientCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    FText CompletedDishFlavorValue; // Final flavor value as text

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    FText CompletedDishTargetFlavor; // Target flavor property name

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    FText CompletedDishMinFlavorValue; // Minimum required as text

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    FText MostUsedIngredient; // Name of most used ingredient

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    int32 PreparationCount = 0; // How many ingredients were prepared

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    FText QualityLevel; // "Perfect", "Great", "Good", "Okay"

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    FText FeedbackText; // Pre-generated feedback based on analysis

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Dialogue|Completion")
    FText SatisfactionFeedbackText; // Satisfaction-based feedback text

    // Override interaction to generate order
    virtual void StartInteraction() override;

private:
    // Delayed clearing state
    bool bOrderMarkedForClearing = false;
    FTimerHandle DelayedClearingTimerHandle;
}; 