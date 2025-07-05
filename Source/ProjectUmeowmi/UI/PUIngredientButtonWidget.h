#pragma once

#include "CoreMinimal.h"
#include "ProjectUmeowmi/UI/PUCommonUserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientButtonWidget.generated.h"

class UTextBlock;
class UButton;
class UImage;
class UPUDishCustomizationWidget;
class UPUIngredientQuantityControl;

/**
 * Widget for displaying and handling individual ingredient instances
 * Each button represents one FIngredientInstance with its own quantity and preparations
 */
UCLASS()
class PROJECTUMEOWMI_API UPUIngredientButtonWidget : public UPUCommonUserWidget
{
    GENERATED_BODY()

public:
    /** The ingredient instance this button represents */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ingredient")
    FIngredientInstance IngredientInstance;

    /** Widget to display the ingredient name */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UTextBlock* IngredientNameText;

    /** Widget to display the ingredient quantity */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UTextBlock* QuantityText;

    /** Widget to display preparation indicators */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UTextBlock* PreparationText;

    /** Button to handle ingredient selection */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UButton* IngredientButton;

    /** Optional ingredient preview image */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UImage* IngredientImage;

    /** Parent dish customization widget */
    UPROPERTY()
    UPUDishCustomizationWidget* ParentDishWidget;

    /** Currently spawned quantity control widget */
    UPROPERTY()
    UPUIngredientQuantityControl* SpawnedQuantityWidget;

    /** Called when the widget is constructed */
    virtual void NativeConstruct() override;

    /** Called when the widget is destroyed */
    virtual void NativeDestruct() override;

    /** Initialize the button with ingredient instance data */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    void InitializeWithInstance(const FIngredientInstance& Instance);

    /** Update the button display with new instance data */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    void UpdateDisplay(const FIngredientInstance& Instance);

    /** Set the parent dish customization widget */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    void SetParentDishWidget(UPUDishCustomizationWidget* DishWidget);

    /** Handle button click to spawn quantity control widget */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    void OnIngredientButtonClicked();

    /** Close the spawned quantity control widget */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    void CloseQuantityWidget();

    /** Check if quantity control widget is currently open */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    bool IsQuantityWidgetOpen() const;

    /** Get the current ingredient instance data */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    const FIngredientInstance& GetIngredientInstance() const { return IngredientInstance; }

    /** Get the instance ID of this ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    int32 GetInstanceID() const { return IngredientInstance.InstanceID; }

protected:
    /** Handle button click event */
    UFUNCTION()
    void OnButtonClicked();

    /** Update the display text based on current instance data */
    void UpdateDisplayText();

    /** Update the preparation display text */
    void UpdatePreparationText();

    /** Update the ingredient image if available */
    void UpdateIngredientImage();

    /** Spawn the quantity control widget */
    void SpawnQuantityControlWidget();

    /** Destroy the current quantity control widget */
    void DestroyQuantityControlWidget();
}; 