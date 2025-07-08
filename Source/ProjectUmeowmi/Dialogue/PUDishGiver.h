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

    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    FText GetOrderCompletionFeedback(AProjectUmeowmiCharacter* PlayerCharacter) const;

    // Order clearing (for dialogue control)
    UFUNCTION(BlueprintCallable, Category = "Dish Giver|Orders")
    void ClearCompletedOrderFromPlayer();

protected:
    // Order component - only dish givers have this
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Giver|Components")
    UPUOrderComponent* OrderComponent;

    // Test boolean for dialogue conditions
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Giver|Test")
    bool bTestCondition = true;

    // Override interaction to generate order
    virtual void StartInteraction() override;
}; 