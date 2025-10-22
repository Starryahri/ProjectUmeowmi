#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientButton.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "PUPlatingWidget.generated.h"

class UPUDishCustomizationComponent;

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUPlatingWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUPlatingWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Event handlers for dish data
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void OnInitialDishDataReceived(const FPUDishBase& InitialDishData);

    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void OnDishDataUpdated(const FPUDishBase& UpdatedDishData);

    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void OnCustomizationEnded();

    // Set the customization component reference (for event subscription only)
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void SetCustomizationComponent(UPUDishCustomizationComponent* Component);

    // Get the customization component reference (for accessing data)
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    UPUDishCustomizationComponent* GetCustomizationComponent() const { return CustomizationComponent; }

    // Get current dish data
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    const FPUDishBase& GetCurrentDishData() const { return CurrentDishData; }

    // End customization function for UI buttons
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void EndCustomizationFromUI();

    // Update dish data and sync back to component
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void UpdateDishData(const FPUDishBase& NewDishData);

    // Plating-specific functions
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void CreateIngredientButtons();

    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void OnIngredientButtonClicked(const FPUIngredientBase& IngredientData);

    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void SpawnIngredientAtPosition(const FGameplayTag& IngredientTag, const FVector2D& ScreenPosition);

    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    UPUIngredientDragDropOperation* CreateIngredientDragDropOperation(const FGameplayTag& IngredientTag, const FPUIngredientBase& IngredientData, int32 InstanceID, int32 Quantity);

    // Blueprint-friendly functions to get ingredient data
    // Get ingredient instances for a specific base ingredient
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    TArray<FIngredientInstance> GetIngredientInstancesForBase(const FGameplayTag& BaseIngredientTag) const;

    // Get all base ingredient tags
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    TArray<FGameplayTag> GetBaseIngredientTags() const;

    // Reset all ingredient quantities to original values
    UFUNCTION(BlueprintCallable, Category = "Plating Widget")
    void ResetAllIngredientQuantities();

protected:
    // Current dish data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Data")
    FPUDishBase CurrentDishData;

    // Widget class references
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientButton> IngredientButtonClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientDragDropOperation> IngredientDragDropOperationClass;

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Plating Widget")
    void OnDishDataReceived(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Plating Widget")
    void OnDishDataChanged(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Plating Widget")
    void OnCustomizationModeEnded();

    UFUNCTION(BlueprintImplementableEvent, Category = "Plating Widget")
    void OnIngredientButtonCreated(class UPUIngredientButton* IngredientButton, const FPUIngredientBase& IngredientData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Plating Widget")
    void OnIngredientSpawned(const FGameplayTag& IngredientTag, const FVector2D& ScreenPosition);

private:
    // Internal component reference for event subscription only
    UPROPERTY()
    UPUDishCustomizationComponent* CustomizationComponent;

    void SubscribeToEvents();
    void UnsubscribeFromEvents();

    // Helper functions
    void RefreshIngredientButtons();
}; 