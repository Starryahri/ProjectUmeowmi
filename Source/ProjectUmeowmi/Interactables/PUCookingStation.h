#pragma once

#include "CoreMinimal.h"
#include "../Dialogue/TalkingObject.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "../DishCustomization/PUDishBase.h"
#include "../DishCustomization/PUOrderBase.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "PUCookingStation.generated.h"

UCLASS()
class PROJECTUMEOWMI_API APUCookingStation : public ATalkingObject
{
    GENERATED_BODY()

public:
    APUCookingStation();

    // ATalkingObject overrides
    virtual void StartInteraction() override;
    virtual void EndInteraction() override;

    // End only the dialogue/interaction state from ATalkingObject without
    // shutting down the dish customization flow. Used by the dialogue UI
    // when a conversation ends but the player should remain in customization.
    void EndDialogueOnly();

protected:
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;

    // Cooking Station specific components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* StationMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* InteractionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPUDishCustomizationComponent* DishCustomizationComponent;

    // Cooking Station specific properties
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText StationName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText StationDescription;

    // Behavior when the player has an active order
    // If true (default), interacting with the station while holding an order
    // will immediately start dish customization and bypass dialogue.
    // If false, interaction will go through the normal TalkingObject dialogue flow,
    // and dish customization must be started explicitly (e.g. via Blueprint or dialogue event).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Station|Orders")
    bool bStartCustomizationImmediatelyWhenHasOrder = true;

    // Data tables for dish customization (same as order component)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* DishDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* IngredientDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* PreparationDataTable;



    // Helper functions
    UFUNCTION()
    void OnCustomizationEnded();

    // Explicitly start dish customization using the player's current order.
    // Useful when you want dialogue (or Blueprint logic) to decide whether and when
    // to enter customization instead of always auto-starting.
    UFUNCTION(BlueprintCallable, Category = "Cooking Station|Orders")
    void StartCustomizationFromCurrentOrder();

    // Cooking Station specific dialogue methods
    UFUNCTION(BlueprintCallable, Category = "Cooking Station|Dialogue")
    void StartNoOrderDialogue();

    // Override dialogue participant methods for cooking station specific logic
    virtual bool CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const override;
    virtual bool OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName) override;

    // Order validation
    UFUNCTION(BlueprintCallable, Category = "Cooking Station|Orders")
    bool ValidateDishAgainstOrder(const FPUDishBase& Dish, const FPUOrderBase& Order, float& OutSatisfactionScore) const;

private:
    // Calculate satisfaction score for order completion
    float CalculateSatisfactionScore(const FPUDishBase& Dish, const FPUOrderBase& Order) const;
}; 