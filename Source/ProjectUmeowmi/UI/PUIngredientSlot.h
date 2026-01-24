#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientQuantityControl.h"
#include "PUIngredientDragDropOperation.h"
#include "PURadialMenu.h"
#include "PUIngredientSlot.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UPUIngredientQuantityControl;
class USlider;
class UMaterialInstanceDynamic;

// Location enum for ingredient slots
UENUM(BlueprintType)
enum class EPUIngredientSlotLocation : uint8
{
    Pantry,
    ActiveIngredientArea,
    Prep,
    Plating,
    Prepped
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

    // Get ingredient data from the parent dish customization widget
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Data")
    TArray<FPUIngredientBase> GetIngredientData() const;

    // GUID-based unique ID generation for ingredient instances
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Data")
    static int32 GenerateGUIDBasedInstanceID();

    // Generate a unique instance ID for this ingredient slot
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Data")
    int32 GenerateUniqueInstanceID() const;

    // Update all display elements
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Display")
    void UpdateDisplay();

    // Drag and drop functions
    // Enable/disable drag functionality
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Drag")
    void SetDragEnabled(bool bEnabled);

    // Check if drag is enabled
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Drag")
    bool IsDragEnabled() const { return bDragEnabled; }

    // Create drag drop operation
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Drag")
    class UPUIngredientDragDropOperation* CreateIngredientDragDropOperation() const;

    // Create a drag visual widget (for use in Blueprint OnDragDetected)
    // This creates a copy of the slot widget with the ingredient icon properly set
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Drag")
    class UPUIngredientSlot* CreateDragVisualWidget() const;

    // Plating-specific functions
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Plating")
    void DecreaseQuantity();

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Plating")
    void ResetQuantity();

    // Reset quantity from the dish data (for plating reset)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Plating")
    void ResetQuantityFromDishData();

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Plating")
    bool CanDrag() const { return RemainingQuantity > 0; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Plating")
    int32 GetRemainingQuantity() const { return RemainingQuantity; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Plating")
    int32 GetMaxQuantity() const { return MaxQuantity; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Plating")
    void UpdatePlatingDisplay();

    // Spawn ingredient at position (moved from plating widget)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Plating")
    void SpawnIngredientAtPosition(const FVector2D& ScreenPosition);

    // Text visibility control for different stages
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Display")
    void SetTextVisibility(bool bShowQuantity, bool bShowDescription);

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Display")
    void HideAllText();

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Display")
    void ShowAllText();

    // Debug method to check text component status
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Display")
    void LogTextComponentStatus();


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
    void ShowRadialMenu(bool bIsPrepMenu, bool bIncludeActions = false);

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Radial Menu")
    void HideRadialMenu();

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Radial Menu")
    bool IsRadialMenuVisible() const { return bRadialMenuVisible; }

    // Set the container for the radial menu (can be called from Blueprint)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Radial Menu")
    void SetRadialMenuContainer(class UPanelWidget* InContainer);

    // Set the dish customization widget reference (called when slot is created)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot")
    void SetDishCustomizationWidget(class UPUDishCustomizationWidget* InDishWidget);

    // Time/Temperature slider functions
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Time/Temp")
    void SetTimeValue(float NewTimeValue);

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Time/Temp")
    void SetTemperatureValue(float NewTemperatureValue);

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Time/Temp")
    float GetTimeValue() const { return IngredientInstance.TimeValue; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Time/Temp")
    float GetTemperatureValue() const { return IngredientInstance.TemperatureValue; }

    // Update slider display and labels
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Time/Temp")
    void UpdateTimeTempSliders();

    // Show/hide sliders based on location and ingredient state
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Time/Temp")
    void UpdateSliderVisibility();

    // Enable/disable sliders
    UFUNCTION(BlueprintCallable, Category = "Ingredient Slot|Time/Temp")
    void SetSlidersEnabled(bool bEnabled);

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

    // Initial ingredient instance (can be set in Blueprint when creating the widget)
    // This will be applied in NativeConstruct if set
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Initialization", meta = (ExposeOnSpawn = "true"))
    FIngredientInstance InitialIngredientInstance;

    // Flag to track if slot has an ingredient
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slot")
    bool bHasIngredient = false;

    // Flag to track if slot is selected
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slot")
    bool bIsSelected = false;

    // Location of this slot
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot", meta = (ExposeOnSpawn = "true"))
    EPUIngredientSlotLocation Location = EPUIngredientSlotLocation::ActiveIngredientArea;

    // Optional: Reference to preparation data table for looking up preparation names
    // If set, will be used to get preparation display names (e.g., "Charred" from "Prep.Char")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Data")
    UDataTable* PreparationDataTable;

    // Prep bowl texture arrays (for Prepped location slots)
    // These arrays should be one-to-one (same length, matching pairs)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Prep Bowls", meta = (ToolTip = "Array of front prep bowl textures"))
    TArray<UTexture2D*> PrepBowlFrontTextures;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Prep Bowls", meta = (ToolTip = "Array of back prep bowl textures (should match length of PrepBowlFrontTextures)"))
    TArray<UTexture2D*> PrepBowlBackTextures;

    // Flag to use random prep bowl selection (any front with any back) instead of paired selection
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Prep Bowls", meta = (ToolTip = "If true, randomly selects any front with any back. If false, uses paired selection (same index)"))
    bool bUseRandomPrepBowls = false;

    // UI Components (will be bound in Blueprint)
    UPROPERTY(meta = (BindWidget))
    UImage* IngredientIcon;

    // Plate background image (always at 100% opacity, outline shown on hover)
    UPROPERTY(meta = (BindWidget))
    UImage* PlateBackground;

    // Prep bowl images (for Prepped location slots)
    UPROPERTY(meta = (BindWidget))
    UImage* PrepBowlFront;

    UPROPERTY(meta = (BindWidget))
    UImage* PrepBowlBack;

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

    // Prep icon images (1 prep icon visible for single prep, or suspicious icon for 2+)
    UPROPERTY(meta = (BindWidget))
    UImage* PrepIcon1;

    UPROPERTY(meta = (BindWidget))
    UImage* PrepIcon2;

    UPROPERTY(meta = (BindWidget))
    UImage* SuspiciousIcon;

    // Material instance for suspicious icon in prepped area
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Suspicious", meta = (ToolTip = "Material instance to use for suspicious icon in prepped area"))
    TSoftObjectPtr<UMaterialInterface> SuspiciousMaterialInstance;

    // Pixelate factor for suspicious material (exposed parameter)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Suspicious", meta = (ToolTip = "Pixelate factor to apply to suspicious material"))
    float SuspiciousPixelateFactor = 10.0f;

    // Dynamic material instance created from SuspiciousMaterialInstance
    UPROPERTY()
    UMaterialInstanceDynamic* SuspiciousDynamicMaterial = nullptr;

    // Material instance for preparation-tinted icon in prepped area
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Preparation", meta = (ToolTip = "Material instance to use for preparation-tinted icon in prepped area"))
    TSoftObjectPtr<UMaterialInterface> PreparationMaterialInstance;

    // Dynamic material instance created from PreparationMaterialInstance
    UPROPERTY()
    UMaterialInstanceDynamic* PreparationDynamicMaterial = nullptr;

    // Cached average color from ingredient texture
    UPROPERTY()
    FLinearColor CachedAverageColor = FLinearColor::White;

    // Color saturation multiplier for intensifying average colors (1.0 = no change, 1.5 = 50% more saturated, 2.0 = double saturation)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Preparation", meta = (ToolTip = "Multiplier to boost color saturation (1.0 = original, higher = more vibrant)", ClampMin = "1.0", ClampMax = "3.0"))
    float ColorSaturationMultiplier = 1.5f;

    // Hover/focus text display
    UPROPERTY(meta = (BindWidget))
    UTextBlock* HoverText;

    // Radial menu widget class
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ingredient Slot|Radial Menu")
    TSubclassOf<UPURadialMenu> RadialMenuWidgetClass;

    // Container for radial menu (should be an Overlay, Canvas Panel, or any Panel Widget)
    // If set, menu will be added to this container instead of viewport
    // Can be set via SetRadialMenuContainer() function from Blueprint
    UPROPERTY()
    class UPanelWidget* RadialMenuContainer;

    // Radial menu widget instance
    UPROPERTY()
    UPURadialMenu* RadialMenuWidget = nullptr;

    // Radial menu visibility flag
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slot|Radial Menu")
    bool bRadialMenuVisible = false;

    // Guard to avoid binding radial menu events multiple times
    bool bRadialMenuEventsBound = false;

    // Cached reference to the dish customization widget (set when slot is created)
    UPROPERTY()
    TWeakObjectPtr<class UPUDishCustomizationWidget> CachedDishWidget;

    // Whether drag functionality is enabled
    // Set to true by default for testing - can be disabled in Blueprint if needed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Drag")
    bool bDragEnabled = true;

    // Time/Temperature slider properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Time/Temp")
    bool bShowTimeTempSliders = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Slot|Time/Temp")
    bool bSlidersEnabled = true;

    // Plating-specific properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slot|Plating")
    int32 RemainingQuantity = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slot|Plating")
    int32 MaxQuantity = 0;

    // Plating-specific UI components
    UPROPERTY(meta = (BindWidget))
    UTextBlock* QuantityText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* PreparationText;

    // Time/Temperature slider components
    UPROPERTY(meta = (BindWidget))
    USlider* TimeSlider;

    UPROPERTY(meta = (BindWidget))
    USlider* TemperatureSlider;

    // Time/Temperature label text (optional - shows current state)
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TimeLabelText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TemperatureLabelText;

    // Native drag and drop events
    virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    // Native mouse events for click handling
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // Native drag detection
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

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

    // Plating-specific Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Slot|Plating")
    void OnQuantityChanged(int32 NewQuantity);

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Slot|Plating")
    void OnPreparationStateChanged();

    // Time/Temperature Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Slot|Time/Temp")
    void OnTimeTemperatureChanged(float TimeValue, float TemperatureValue);

private:
    // Helper functions
    void UpdateIngredientIcon();
    void UpdatePrepIcons();
    void UpdatePrepBowls();
    UTexture2D* GetPreparationTexture(const FGameplayTag& PreparationTag) const;
    UTexture2D* GetPreparationPrepTexture(const FGameplayTag& PreparationTag, UDataTable* PrepDataTable) const;
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

    // Plating helper functions
    void UpdateQuantityDisplay();
    void UpdatePreparationDisplay();
    FString GetPreparationDisplayText() const;
    FString GetPreparationIconText() const;

    // Time/Temperature helper functions
    void InitializeTimeTempSliders();
    void UpdateTimeLabelText();
    void UpdateTemperatureLabelText();
    bool ShouldShowSliders() const;
    
    // Recalculate aspects from base + time/temp + quantity
    void RecalculateAspectsFromBase();

    // Get average color from ingredient texture
    void GetAverageColorFromIngredientTexture();

    // Boost color saturation using HSV conversion
    FLinearColor BoostColorSaturation(const FLinearColor& Color, float SaturationMultiplier) const;

    // Quantity control event handlers
    UFUNCTION()
    void OnQuantityControlChanged(const FIngredientInstance& InIngredientInstance);

    UFUNCTION()
    void OnQuantityControlRemoved(int32 InstanceID, UPUIngredientQuantityControl* InQuantityControlWidget);

    // Time/Temperature slider event handlers
    UFUNCTION()
    void OnTimeSliderValueChanged(float NewValue);

    UFUNCTION()
    void OnTemperatureSliderValueChanged(float NewValue);

    // Radial menu event handlers
    UFUNCTION()
    void OnRadialMenuItemSelected(const FRadialMenuItem& SelectedItem);

    UFUNCTION()
    void OnRadialMenuClosed();

    // Helper function to get slot's screen position
    FVector2D GetSlotScreenPosition() const;

    // Helper functions for radial menu
    TArray<FRadialMenuItem> BuildPreparationMenuItems() const;
    TArray<FRadialMenuItem> BuildActionMenuItems() const;
    bool ApplyPreparationToIngredient(const FGameplayTag& PreparationTag);
    bool RemovePreparationFromIngredient(const FGameplayTag& PreparationTag);
    void ExecuteAction(const FGameplayTag& ActionTag);
    class UPUDishCustomizationComponent* GetDishCustomizationComponent() const;
    class UPUDishCustomizationWidget* GetDishCustomizationWidget() const;

};

