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

    // Override dialogue participant methods to include order data
    virtual bool CheckCondition(const UDlgContext* Context, FName ConditionName) const override;
    virtual FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const override;

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

protected:
    // Order component - only dish givers have this
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Giver|Components")
    UPUOrderComponent* OrderComponent;

    // Override interaction to generate order
    virtual void StartInteraction() override;
}; 