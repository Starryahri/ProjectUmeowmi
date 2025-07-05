#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUDishCustomizationWidget.generated.h"

class UPUDishCustomizationComponent;
class UPUIngredientButtonWidget;
class UPUIngredientQuantityControl;
class UUniformGridPanel;
class UScrollBox;

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

    // Ingredient button management
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void CreateIngredientButtons();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void UpdateIngredientButtons();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void ClearIngredientButtons();

    // Quantity control management
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void AddQuantityControlToScrollBox(UPUIngredientQuantityControl* QuantityControl);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void RemoveQuantityControlFromScrollBox(UPUIngredientQuantityControl* QuantityControl);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void ClearQuantityControlScrollBox();

    // Get widget class references
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    TSubclassOf<UPUIngredientButtonWidget> GetIngredientButtonWidgetClass() const { return IngredientButtonWidgetClass; }

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    TSubclassOf<UPUIngredientQuantityControl> GetQuantityControlWidgetClass() const { return QuantityControlWidgetClass; }

protected:
    // Current dish data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Data")
    FPUDishBase CurrentDishData;

    // Reference to the customization component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPUDishCustomizationComponent* CustomizationComponent;

    // Widget class references
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Classes")
    TSubclassOf<UPUIngredientButtonWidget> IngredientButtonWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Classes")
    TSubclassOf<UPUIngredientQuantityControl> QuantityControlWidgetClass;

    // UI Components (BindWidget)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UUniformGridPanel* IngredientButtonGrid;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UScrollBox* QuantityControlScrollBox;

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnDishDataReceived(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnDishDataChanged(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnCustomizationModeEnded();

private:
    void SubscribeToEvents();
    void UnsubscribeFromEvents();

    // Ingredient button management helpers
    void CreateIngredientButtonForInstance(const FIngredientInstance& Instance);
    void UpdateIngredientButtonForInstance(const FIngredientInstance& Instance);
    void RemoveIngredientButtonForInstance(int32 InstanceID);

    // Quantity control management helpers
    UPUIngredientQuantityControl* CreateQuantityControlWidget(const FIngredientInstance& Instance);

    // Data management
    TArray<UPUIngredientButtonWidget*> IngredientButtons;
    TArray<UPUIngredientQuantityControl*> ActiveQuantityControls;
}; 