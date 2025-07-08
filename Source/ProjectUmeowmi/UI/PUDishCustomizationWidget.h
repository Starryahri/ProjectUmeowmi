#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientButton.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "PUDishCustomizationWidget.generated.h"

class UPUDishCustomizationComponent;

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUDishCustomizationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUDishCustomizationWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Event handlers for dish data
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void OnInitialDishDataReceived(const FPUDishBase& InitialDishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void OnDishDataUpdated(const FPUDishBase& UpdatedDishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void OnCustomizationEnded();

    // Set the customization component reference
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void SetCustomizationComponent(UPUDishCustomizationComponent* Component);

    // Get current dish data
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    const FPUDishBase& GetCurrentDishData() const { return CurrentDishData; }

    // Update dish data and sync back to component
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void UpdateDishData(const FPUDishBase& NewDishData);

    // Ingredient management functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void CreateIngredientButtons();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void OnIngredientButtonClicked(const FPUIngredientBase& IngredientData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void OnQuantityControlChanged(const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void OnQuantityControlRemoved(int32 InstanceID, class UPUIngredientQuantityControl* QuantityControlWidget);

protected:
    // Current dish data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Data")
    FPUDishBase CurrentDishData;

    // Reference to the customization component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPUDishCustomizationComponent* CustomizationComponent;

    // Widget class references
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientButton> IngredientButtonClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientQuantityControl> QuantityControlClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUPreparationCheckbox> PreparationCheckboxClass;

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnDishDataReceived(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnDishDataChanged(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnCustomizationModeEnded();

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnIngredientButtonCreated(class UPUIngredientButton* IngredientButton, const FPUIngredientBase& IngredientData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnQuantityControlCreated(class UPUIngredientQuantityControl* QuantityControl, const FIngredientInstance& IngredientInstance);

private:
    void SubscribeToEvents();
    void UnsubscribeFromEvents();

    // Helper functions
    void CreateIngredientInstance(const FPUIngredientBase& IngredientData);
    void UpdateIngredientInstance(const FIngredientInstance& IngredientInstance);
    void RemoveIngredientInstance(int32 InstanceID);
    void RefreshQuantityControls();
}; 