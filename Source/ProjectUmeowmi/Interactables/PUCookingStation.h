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

protected:
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

    // Helper functions
    UFUNCTION()
    void OnCustomizationEnded();

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