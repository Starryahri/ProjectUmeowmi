#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PURadialMenu.generated.h"

class UImage;
class UTextBlock;
class UButton;
class UPanelWidget;
class UCanvasPanel;
class UCanvasPanelSlot;

/**
 * Data structure for a radial menu item
 */
USTRUCT(BlueprintType)
struct FRadialMenuItem
{
    GENERATED_BODY()

    FRadialMenuItem()
        : Icon(nullptr)
        , bIsEnabled(true)
        , bIsApplied(false)
    {}

    // Display name for the menu item
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu Item")
    FText Label;

    // Icon texture for the menu item
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu Item")
    UTexture2D* Icon;

    // Action tag (e.g., "Prep.Char" for preparations or "Action.Remove" for actions)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu Item", meta = (Categories = "Prep,Action"))
    FGameplayTag ActionTag;

    // Whether this item is enabled/selectable
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu Item")
    bool bIsEnabled;

    // Whether this preparation is already applied (for prep menu items)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu Item")
    bool bIsApplied;

    // Tooltip/description text
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu Item")
    FText Tooltip;
};

/**
 * Event delegate for when a menu item is selected
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRadialMenuItemSelected, const FRadialMenuItem&, SelectedItem);

/**
 * Event delegate for when the menu is closed
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRadialMenuClosed);

/**
 * Radial menu widget for displaying circular menu options
 * Used for ingredient slot preparation and action menus
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPURadialMenu : public UUserWidget
{
    GENERATED_BODY()

public:
    UPURadialMenu(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    // Set the menu items to display
    UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    void SetMenuItems(const TArray<FRadialMenuItem>& InMenuItems);

    // Get the current menu items
    UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    TArray<FRadialMenuItem> GetMenuItems() const { return MenuItems; }

    // Show the menu at a specific screen position
    UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    void ShowMenuAtPosition(const FVector2D& ScreenPosition);

    // Hide the menu
    UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    void HideMenu();

    // Check if menu is visible
    UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    bool IsMenuVisible() const { return bIsVisible; }

    // Set the menu center position (for positioning items in a circle)
    UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    void SetMenuCenterPosition(const FVector2D& CenterPosition);

    // Get the menu center position
    UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    FVector2D GetMenuCenterPosition() const { return MenuCenterPosition; }

    // Set the radius for menu items (distance from center)
    UFUNCTION(BlueprintCallable, Category = "Radial Menu|Layout")
    void SetItemRadius(float InRadius);

    // Get the item radius
    UFUNCTION(BlueprintCallable, Category = "Radial Menu|Layout")
    float GetItemRadius() const { return ItemRadius; }

    // Set the center position for menu items (relative to container, in widget space)
    // If not set, will use container center automatically
    UFUNCTION(BlueprintCallable, Category = "Radial Menu|Layout")
    void SetLayoutCenterPosition(const FVector2D& InCenterPosition);

    // Get the layout center position
    UFUNCTION(BlueprintCallable, Category = "Radial Menu|Layout")
    FVector2D GetLayoutCenterPosition() const { return LayoutCenterPosition; }

    // Clear custom center position (use container center instead)
    UFUNCTION(BlueprintCallable, Category = "Radial Menu|Layout")
    void UseAutoCenter() { bUseCustomCenter = false; }

    // Get menu item buttons (for Blueprint to bind click events)
    UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    TArray<class UPURadialMenuItemButton*> GetMenuItemButtons() const { return MenuItemButtons; }

    // Handle menu item selection by index (can be called from Blueprint button clicks)
    UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    void SelectMenuItemByIndex(int32 ItemIndex);

    // Preview/arrange buttons in radial layout (for Blueprint designer preview)
    // Arranges all child widgets in MenuItemsContainer into a circle
    // Useful for testing layouts in Blueprint designer - call this in PreConstruct
    UFUNCTION(BlueprintCallable, Category = "Radial Menu|Preview")
    void PreviewRadialLayout();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Radial Menu|Events")
    FOnRadialMenuItemSelected OnMenuItemSelected;

    UPROPERTY(BlueprintAssignable, Category = "Radial Menu|Events")
    FOnRadialMenuClosed OnMenuClosed;

    // Preparation data table reference (can be set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu|Data")
    UDataTable* PreparationDataTable;

    // Set the preparation data table
    UFUNCTION(BlueprintCallable, Category = "Radial Menu|Data")
    void SetPreparationDataTable(UDataTable* InDataTable);

    // Get the preparation data table
    UFUNCTION(BlueprintCallable, Category = "Radial Menu|Data")
    UDataTable* GetPreparationDataTable() const { return PreparationDataTable; }

protected:
    // Current menu items
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radial Menu")
    TArray<FRadialMenuItem> MenuItems;

    // Menu visibility flag
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radial Menu")
    bool bIsVisible = false;

    // Center position of the menu (for circular layout)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radial Menu")
    FVector2D MenuCenterPosition;

    // Radius for positioning menu items in a circle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu|Layout", meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float ItemRadius = 100.0f;

    // Custom center position for menu items (if bUseCustomCenter is true)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu|Layout")
    FVector2D LayoutCenterPosition = FVector2D(200.0f, 200.0f);

    // Whether to use custom center position or auto-calculate from container
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu|Layout")
    bool bUseCustomCenter = false;

    // Container widget for menu items (should be a Canvas Panel in Blueprint)
    UPROPERTY(meta = (BindWidget))
    UCanvasPanel* MenuItemsContainer;
    
    // Visual indicator (arrow/line) that points to selected menu item (optional, can be set in Blueprint)
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* SelectionIndicator;
    
    // Note: Direction line is now drawn using Slate's NativePaint (no Image widget needed)
    
    // Debug visualization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu|Debug")
    bool bShowDebugRegions = false; // Set to true to show debug visualization (regions and angle labels)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu|Debug")
    bool bShowDebugText = false; // Set to true to show debug text (stick values, angle, selected button)

    // Button widget class to use for menu items (can be set in Blueprint)
    // Defaults to UPURadialMenuItemButton, but can be overridden with a custom Blueprint class
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Menu|Layout")
    TSubclassOf<class UPURadialMenuItemButton> MenuItemButtonClass;

    // Size of each menu item button
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu|Layout")
    FVector2D MenuItemSize = FVector2D(64.0f, 64.0f);

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Radial Menu")
    void OnMenuItemsSet(const TArray<FRadialMenuItem>& InMenuItems);

    UFUNCTION(BlueprintImplementableEvent, Category = "Radial Menu")
    void OnMenuShown();

    UFUNCTION(BlueprintImplementableEvent, Category = "Radial Menu")
    void OnMenuHidden();

private:
    // Helper function to update menu layout
    void UpdateMenuLayout();

    // Clear all menu item buttons
    void ClearMenuItems();

    // Create and position a menu item button
    class UPURadialMenuItemButton* CreateMenuItemButton(const FRadialMenuItem& MenuItem, float Angle, int32 Index);

    // Note: Button click handling is done via SelectMenuItemByIndex which Blueprint calls

    // Array to store created button widgets
    UPROPERTY()
    TArray<class UPURadialMenuItemButton*> MenuItemButtons;
    
    // Currently selected menu item index (for controller navigation)
    int32 SelectedMenuItemIndex = 0;
    
    // Navigate to next/previous menu item
    void NavigateToMenuItem(int32 NewIndex);
    
    // Set focus to currently selected menu item
    void SetFocusToSelectedMenuItem();
    
    // Joystick-based selection state
    float JoystickDeadzone = 0.3f; // Deadzone to prevent drift
    float JoystickSelectionCooldown = 0.0f; // Cooldown to prevent rapid switching
    const float JoystickSelectionRepeatDelay = 0.1f; // Delay between selection changes
    
    // Select menu item based on joystick direction (angle in degrees)
    void SelectMenuItemByAngle(float AngleDegrees);
    
    // Get the angle for a menu item index (based on its position in the circle)
    float GetMenuItemAngle(int32 ItemIndex) const;
    
    // Update the visual indicator to point to the selected menu item
    void UpdateSelectionIndicator();
    
    // Update the direction line to point from center to stick/mouse direction
    void UpdateDirectionLine(float AngleDegrees, float InputMagnitude);
    
    // Custom paint to draw the direction line using Slate (like radar graphs)
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
                             const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
                             int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
    // Current direction line state (for NativePaint)
    mutable float CurrentDirectionAngle = 0.0f;
    mutable float CurrentDirectionLength = 0.0f;
    mutable bool bShouldDrawDirectionLine = false;
    mutable float CurrentStickX = 0.0f;
    mutable float CurrentStickY = 0.0f;

};

