#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "PUCookingStageWidget.generated.h"

class UPUIngredientQuantityControl;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCookingCompleted, const FPUDishBase&, FinalDishData);

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUCookingStageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUCookingStageWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Initialize the cooking stage with dish data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void InitializeCookingStage(const FPUDishBase& DishData);

    // Get the current dish data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    const FPUDishBase& GetCurrentDishData() const { return CurrentDishData; }

    // Get the planning data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    const FPUPlanningData& GetPlanningData() const { return PlanningData; }

    // Finish cooking and transition to plating
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void FinishCookingAndStartPlating();

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnCookingStageInitialized(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnCookingStageCompleted(const FPUDishBase& FinalDishData);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Cooking Stage Widget|Events")
    FOnCookingCompleted OnCookingCompleted;

protected:
    // Current dish data (being built during cooking)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Data")
    FPUDishBase CurrentDishData;

    // Planning data from the planning stage
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planning Data")
    FPUPlanningData PlanningData;

    // Widget class references
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientQuantityControl> QuantityControlClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUPreparationCheckbox> PreparationCheckboxClass;

private:
    // Create quantity controls for selected ingredients
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void CreateQuantityControlsForSelectedIngredients();

    // Handle quantity control changes
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void OnQuantityControlChanged(const FIngredientInstance& IngredientInstance);

    // Handle quantity control removal
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void OnQuantityControlRemoved(int32 InstanceID, class UPUIngredientQuantityControl* QuantityControlWidget);
}; 