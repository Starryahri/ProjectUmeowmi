#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientQuantityControl.h"
#include "PUIngredientDragDropOperation.h"
#include "PUIngredientSlot.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UPUIngredientQuantityControl;
class UUserWidget; // For radial menu (stubbed)

// Location enum for ingredient slots
UENUM(BlueprintType)
enum class EPUIngredientSlotLocation : uint8
{
    Pantry,
    ActiveIngredientArea,
    Prep
};

// Events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIngredientDroppedOnSlot, class UPUIngredientSlot*, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmptySlotClicked, class UPUIngredientSlot*, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotIngredientChanged, const FIngredientInstance&, IngredientInstance);

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUIngredientSlot : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUIngredientSlot(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Set the ingredient instance for this slot (use ClearSlot() to empty)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot")
    void SetIngredientInstance(const FIngredientInstance& InIngredientInstance);

    // Clear the slot (make it empty)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot")
    void ClearSlot();

    // Get the current ingredient instance
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot")
    const FIngredientInstance& GetIngredientInstance() const { return IngredientInstance; }

    // Check if slot is empty
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot")
    bool IsEmpty() const { return !bHasIngredient; }

    // Set the location (Pantry or ActiveIngredientArea)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot")
    void SetLocation(EPUIngredientSlotLocation InLocation);

    // Get the location
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot")
    EPUIngredientSlotLocation GetLocation() const { return Location; }

    // Set selection state
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot")
    void SetSelected(bool bSelected);

    // Get selection state
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot")
    bool IsSelected() const { return bIsSelected; }

    // Set the preparation data table
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Data")
    void SetPreparationDataTable(UDataTable* InPreparationDataTable) { PreparationDataTable = InPreparationDataTable; }

    // Update all display elements
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Display")
    void UpdateDisplay();

    // Get UI components (Blueprint accessible)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Components")
    UImage* GetIngredientIcon() const { return IngredientIcon; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Components")
    UPUIngredientQuantityControl* GetQuantityControl() const { return QuantityControlWidget; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Components")
    UTextBlock* GetHoverText() const { return HoverText; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Components")
    UImage* GetPrepIcon1() const { return PrepIcon1; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Components")
    UImage* GetPrepIcon2() const { return PrepIcon2; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Components")
    UImage* GetSuspiciousIcon() const { return SuspiciousIcon; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Components")
    UPanelWidget* GetQuantityControlContainer() const { return QuantityControlContainer; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Components")
    UImage* GetPlateBackground() const { return PlateBackground; }

    // Radial menu functions (stubbed for now)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Radial Menu")
    void ShowRadialMenu(bool bIsPrepMenu);

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Radial Menu")
    void HideRadialMenu();

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Radial Menu")
    bool IsRadialMenuVisible() const { return bRadialMenuVisible; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Ingredient Slot|Events")
    FOnIngredientDroppedOnSlot OnIngredientDroppedOnSlot;

    UPROPERTY(BlueprintAssignable, Category = "Ingredient Slot|Events")
    FOnEmptySlotClicked OnEmptySlotClicked;

    UPROPERTY(BlueprintAssignable, Category = "Ingredient Slot|Events")
    FOnSlotIngredientChanged OnSlotIngredientChanged;

protected:
    // Current ingredient instance
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slot")
    FIngredientInstance IngredientInstance;

    // Flag to track if slot has an ingredient
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slot")
    bool bHasIngredient = false;

    // Flag to track if slot is selected
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slot")
    bool bIsSelected = false;

    // Location of this slot
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot")
    EPUIngredientSlotLocation Location = EPUIngredientSlotLocation::ActiveIngredientArea;

    // Optional: Reference to preparation data table for looking up preparation names
    // If set, will be used to get preparation display names (e.g., "Charred" from "Prep.Char")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Data")
    UDataTable* PreparationDataTable;

    // UI Components (will be bound in Blueprint)
    UPROPERTY(meta = (BindWidget))
    UImage* IngredientIcon;

    // Plate background image (opacity changes on hover)
    UPROPERTY(meta = (BindWidget))
    UImage* PlateBackground;

    // Container for quantity control widget
    UPROPERTY(meta = (BindWidget))
    class UPanelWidget* QuantityControlContainer;

    // Quantity control widget class
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ingredient Slot|Widget Classes")
    TSubclassOf<UPUIngredientQuantityControl> QuantityControlClass;

    // Created quantity control widget instance
    UPROPERTY()
    UPUIngredientQuantityControl* QuantityControlWidget = nullptr;

    // Flag to track if events are bound (prevent duplicate bindings)
    bool bQuantityControlEventsBound = false;

    // Prep icon images (max 2 visible, or suspicious icon for 3+)
    UPROPERTY(meta = (BindWidget))
    UImage* PrepIcon1;

    UPROPERTY(meta = (BindWidget))
    UImage* PrepIcon2;

    UPROPERTY(meta = (BindWidget))
    UImage* SuspiciousIcon;

    // Hover/focus text display
    UPROPERTY(meta = (BindWidget))
    UTextBlock* HoverText;

    // Radial menu widget class (stubbed)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ingredient Slot|Radial Menu")
    TSubclassOf<UUserWidget> RadialMenuWidgetClass;

    // Radial menu widget instance (stubbed)
    UPROPERTY()
    UUserWidget* RadialMenuWidget = nullptr;

    // Radial menu visibility flag
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slot|Radial Menu")
    bool bRadialMenuVisible = false;

    // Native drag and drop events
    virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    // Native mouse events for click handling
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // Native hover events
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    // Native focus events
    virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
    virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Slot")
    void OnIngredientInstanceSet(const FIngredientInstance& InIngredientInstance);

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Slot")
    void OnSlotEmptied();

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Slot")
    void OnLocationChanged(EPUIngredientSlotLocation NewLocation);

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Slot")
    void OnDragOverSlot();

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Slot")
    void OnDragLeaveSlot();

private:
    // Helper functions
    void UpdateIngredientIcon();
    void UpdatePrepIcons();
    void UpdateQuantityControl();
    void ClearDisplay();

    // Get the appropriate texture based on location
    UTexture2D* GetTextureForLocation() const;

    // Get display text for ingredient (with preparations)
    FText GetIngredientDisplayText() const;

    // Update hover text visibility
    void UpdateHoverTextVisibility(bool bShow);

    // Update plate background opacity based on selection state
    void UpdatePlateBackgroundOpacity();

    // Quantity control event handlers
    UFUNCTION()
    void OnQuantityControlChanged(const FIngredientInstance& InIngredientInstance);

    UFUNCTION()
    void OnQuantityControlRemoved(int32 InstanceID, UPUIngredientQuantityControl* InQuantityControlWidget);

};

