#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "../DishCustomization/PUDishBase.h"
#include "GameplayTagContainer.h"
#include "PUIngredientButton.generated.h"

class UButton;
class UTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIngredientButtonClicked, const FPUIngredientBase&, IngredientData);

/**
 * @deprecated This class is deprecated. Use UPUIngredientSlot instead.
 * All functionality has been migrated to PUIngredientSlot.
 * This class will be removed in a future version.
 */
UCLASS(BlueprintType, Blueprintable, meta = (Deprecated))
class PROJECTUMEOWMI_API UPUIngredientButton : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUIngredientButton(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Set the ingredient data for this button
    // @deprecated Use UPUIngredientSlot::SetIngredientInstance instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void SetIngredientData(const FPUIngredientBase& InIngredientData);

    // Get the current ingredient data
    // @deprecated Use UPUIngredientSlot::GetIngredientInstance instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    const FPUIngredientBase& GetIngredientData() const { return IngredientData; }

    // Get UI components (Blueprint accessible)
    // @deprecated Use UPUIngredientSlot instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Components", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    UButton* GetIngredientButton() const { return IngredientButton; }

    // @deprecated Use UPUIngredientSlot instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Components", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    UTextBlock* GetIngredientNameText() const { return IngredientNameText; }

    // @deprecated Use UPUIngredientSlot::GetIngredientIcon instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Components", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    UImage* GetIngredientIcon() const { return IngredientIcon; }

    // Event when button is clicked
    // @deprecated Use UPUIngredientSlot events instead
    UPROPERTY(BlueprintAssignable, Category = "Ingredient Button|Events", meta = (DeprecatedProperty, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    FOnIngredientButtonClicked OnIngredientButtonClicked;

    // Events when button is hovered/unhovered
    // @deprecated Use UPUIngredientSlot events instead
    UPROPERTY(BlueprintAssignable, Category = "Ingredient Button|Events", meta = (DeprecatedProperty, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    FOnIngredientButtonClicked OnIngredientButtonHovered;

    // Native drag events (only enabled in cooking stage)
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // Enable/disable drag functionality
    // @deprecated Use UPUIngredientSlot::SetDragEnabled instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Drag", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void SetDragEnabled(bool bEnabled);

    // Check if drag is enabled
    // @deprecated Use UPUIngredientSlot::IsDragEnabled instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Drag", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    bool IsDragEnabled() const { return bDragEnabled; }


    // Generate a unique instance ID for this ingredient
    // @deprecated Use UPUIngredientSlot::GenerateUniqueInstanceID instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Drag", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    int32 GenerateUniqueInstanceID() const;

    // Plating-specific functions
    // @deprecated Use UPUIngredientSlot::SetIngredientInstance instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void SetIngredientInstance(const FIngredientInstance& InInstance);

    // @deprecated Use UPUIngredientSlot::GetIngredientInstance instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    const FIngredientInstance& GetIngredientInstance() const { return IngredientInstance; }

    // @deprecated Use UPUIngredientSlot::DecreaseQuantity instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void DecreaseQuantity();

    // @deprecated Use UPUIngredientSlot::ResetQuantity instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void ResetQuantity();

    // @deprecated Use UPUIngredientSlot::CanDrag instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    bool CanDrag() const { return RemainingQuantity > 0; }

    // @deprecated Use UPUIngredientSlot::GetRemainingQuantity instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    int32 GetRemainingQuantity() const { return RemainingQuantity; }

    // @deprecated Use UPUIngredientSlot::GetMaxQuantity instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    int32 GetMaxQuantity() const { return MaxQuantity; }

    // @deprecated Use UPUIngredientSlot events instead
    UPROPERTY(BlueprintAssignable, Category = "Ingredient Button|Events", meta = (DeprecatedProperty, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    FOnIngredientButtonClicked OnIngredientButtonUnhovered;

    // Plating helper functions
    // @deprecated Use UPUIngredientSlot::UpdatePlatingDisplay instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void UpdatePlatingDisplay();

    // Spawn ingredient at position (moved from plating widget)
    // @deprecated Use UPUIngredientSlot::SpawnIngredientAtPosition instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void SpawnIngredientAtPosition(const FVector2D& ScreenPosition);

    // Create drag drop operation (moved from plating widget)
    // @deprecated Use UPUIngredientSlot::CreateIngredientDragDropOperation instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Plating", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    class UPUIngredientDragDropOperation* CreateIngredientDragDropOperation() const;

    // Text visibility control for different stages
    // @deprecated Use UPUIngredientSlot::SetTextVisibility instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Display", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void SetTextVisibility(bool bShowQuantity, bool bShowDescription);

    // @deprecated Use UPUIngredientSlot::HideAllText instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Display", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void HideAllText();

    // @deprecated Use UPUIngredientSlot::ShowAllText instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Display", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void ShowAllText();

    // Debug method to check text component status
    // @deprecated Use UPUIngredientSlot::LogTextComponentStatus instead
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Display", meta = (DeprecatedFunction, DeprecationMessage = "PUIngredientButton is deprecated. Use PUIngredientSlot instead."))
    void LogTextComponentStatus();

protected:
    // Current ingredient data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Data")
    FPUIngredientBase IngredientData;

    // UI Components (will be bound in Blueprint)
    UPROPERTY(meta = (BindWidget))
    UButton* IngredientButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* IngredientNameText;

    UPROPERTY(meta = (BindWidget))
    UImage* IngredientIcon;

    // Plating-specific UI components
    UPROPERTY(meta = (BindWidget))
    UTextBlock* QuantityText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* PreparationText;

    // Whether drag functionality is enabled
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Button|Drag")
    bool bDragEnabled = false;

    // Plating-specific properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plating")
    FIngredientInstance IngredientInstance;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plating")
    int32 RemainingQuantity = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plating")
    int32 MaxQuantity = 0;

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button")
    void OnIngredientDataSet(const FPUIngredientBase& InIngredientData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button")
    void OnButtonClicked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button")
    void OnButtonHovered();

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button")
    void OnButtonUnhovered();

    // Plating-specific Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button|Plating")
    void OnIngredientInstanceSet(const FIngredientInstance& InInstance);

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button|Plating")
    void OnQuantityChanged(int32 NewQuantity);

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button|Plating")
    void OnPreparationStateChanged();

private:
    // Button event handlers
    UFUNCTION()
    void OnIngredientButtonClickedInternal();

    UFUNCTION()
    void OnIngredientButtonHoveredInternal();

    UFUNCTION()
    void OnIngredientButtonUnhoveredInternal();

    // Plating helper functions
    void UpdateQuantityDisplay();
    void UpdatePreparationDisplay();
    FString GetPreparationDisplayText() const;
    FString GetPreparationIconText() const;
}; 