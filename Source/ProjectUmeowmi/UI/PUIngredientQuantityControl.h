#pragma once

#include "CoreMinimal.h"
#include "ProjectUmeowmi/UI/PUCommonUserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientQuantityControl.generated.h"

class UTextBlock;
class UButton;
class UImage;
class UCheckBox;
class UVerticalBox;
class UHorizontalBox;
class UPUIngredientButtonWidget;
class UPUDishCustomizationWidget;

/**
 * Widget for controlling quantity and preparations of a single ingredient instance
 * This widget is spawned when an ingredient button is clicked
 */
UCLASS()
class PROJECTUMEOWMI_API UPUIngredientQuantityControl : public UPUCommonUserWidget
{
    GENERATED_BODY()

public:
    /** The ingredient instance this control represents */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ingredient")
    FIngredientInstance IngredientInstance;

    /** Widget to display the ingredient name */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UTextBlock* IngredientNameText;

    /** Widget to display the current quantity */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UTextBlock* QuantityDisplayText;

    /** Button to decrease quantity */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UButton* DecreaseQuantityButton;

    /** Button to increase quantity */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UButton* IncreaseQuantityButton;

    /** Container for preparation checkboxes */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UVerticalBox* PreparationContainer;

    /** Button to close this control widget */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UButton* CloseButton;

    /** Optional ingredient preview image */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    UImage* IngredientImage;

    /** Parent ingredient button widget */
    UPROPERTY()
    UPUIngredientButtonWidget* ParentButtonWidget;

    /** Parent dish customization widget */
    UPROPERTY()
    UPUDishCustomizationWidget* ParentDishWidget;

    /** Called when the widget is constructed */
    virtual void NativeConstruct() override;

    /** Called when the widget is destroyed */
    virtual void NativeDestruct() override;

    /** Initialize the control with ingredient instance data */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Quantity Control")
    void InitializeWithInstance(const FIngredientInstance& Instance);

    /** Update the control display with new instance data */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Quantity Control")
    void UpdateDisplay(const FIngredientInstance& Instance);

    /** Set the parent button widget */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Quantity Control")
    void SetParentButtonWidget(UPUIngredientButtonWidget* ButtonWidget);

    /** Set the parent dish customization widget */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Quantity Control")
    void SetParentDishWidget(UPUDishCustomizationWidget* DishWidget);

    /** Get the current ingredient instance data */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Quantity Control")
    const FIngredientInstance& GetIngredientInstance() const { return IngredientInstance; }

    /** Get the instance ID of this ingredient */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Quantity Control")
    int32 GetInstanceID() const { return IngredientInstance.InstanceID; }

    /** Get the current quantity */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Quantity Control")
    int32 GetCurrentQuantity() const { return IngredientInstance.Quantity; }

    /** Get the current preparations */
    UFUNCTION(BlueprintCallable, Category = "Ingredient Quantity Control")
    FGameplayTagContainer GetCurrentPreparations() const;

protected:
    /** Handle decrease quantity button click */
    UFUNCTION()
    void OnDecreaseQuantityClicked();

    /** Handle increase quantity button click */
    UFUNCTION()
    void OnIncreaseQuantityClicked();

    /** Handle close button click */
    UFUNCTION()
    void OnCloseButtonClicked();

    /** Handle preparation checkbox state changed */
    UFUNCTION()
    void OnPreparationCheckboxChanged(bool bIsChecked, FGameplayTag PreparationTag);

    /** Update the display text based on current instance data */
    void UpdateDisplayText();

    /** Update the preparation checkboxes */
    void UpdatePreparationCheckboxes();

    /** Update the ingredient image if available */
    void UpdateIngredientImage();

    /** Update quantity buttons enabled state based on min/max limits */
    void UpdateQuantityButtonStates();

    /** Apply changes to the dish data */
    void ApplyChangesToDishData();

    /** Create preparation checkbox widgets */
    void CreatePreparationCheckboxes();

    /** Get available preparations for this ingredient */
    TArray<FGameplayTag> GetAvailablePreparations() const;

    /** Check if a preparation is currently applied */
    bool IsPreparationApplied(const FGameplayTag& PreparationTag) const;

    /** Add a preparation to the ingredient */
    void AddPreparation(const FGameplayTag& PreparationTag);

    /** Remove a preparation from the ingredient */
    void RemovePreparation(const FGameplayTag& PreparationTag);

private:
    /** Map of preparation checkboxes by tag */
    UPROPERTY()
    TMap<FGameplayTag, UCheckBox*> PreparationCheckboxes;

    /** Temporary data for tracking changes before applying */
    UPROPERTY()
    int32 PendingQuantity;

    UPROPERTY()
    FGameplayTagContainer PendingPreparations;
}; 