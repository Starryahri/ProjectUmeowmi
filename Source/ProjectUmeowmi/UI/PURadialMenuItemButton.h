#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PURadialMenu.h"
#include "PURadialMenuItemButton.generated.h"

class UButton;
class UImage;
class UTextBlock;

/**
 * Event delegate for when a radial menu item button is clicked
 * Includes the item index for easy identification
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRadialMenuItemButtonClicked, int32, ItemIndex);

/**
 * Custom button widget for radial menu items
 * Stores its own index and emits it when clicked
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPURadialMenuItemButton : public UUserWidget
{
    GENERATED_BODY()

public:
    UPURadialMenuItemButton(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Set the menu item data and index for this button
    UFUNCTION(BlueprintCallable, Category = "Radial Menu Item Button")
    void SetMenuItemData(const FRadialMenuItem& MenuItem, int32 InItemIndex);

    // Clear the menu item data (safe to call during GC)
    void ClearMenuItemData();

    // Get the item index
    UFUNCTION(BlueprintCallable, Category = "Radial Menu Item Button")
    int32 GetItemIndex() const { return ItemIndex; }

    // Get the menu item data
    UFUNCTION(BlueprintCallable, Category = "Radial Menu Item Button")
    const FRadialMenuItem& GetMenuItemData() const { return MenuItemData; }

    // Get the button component (for styling in Blueprint)
    UFUNCTION(BlueprintCallable, Category = "Radial Menu Item Button|Components")
    UButton* GetButton() const { return ItemButton; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Radial Menu Item Button|Events")
    FOnRadialMenuItemButtonClicked OnItemClicked;

protected:
    // Button component (should be bound in Blueprint)
    UPROPERTY(meta = (BindWidget))
    UButton* ItemButton;

protected:
    // Menu item data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radial Menu Item Button")
    FRadialMenuItem MenuItemData;

    // Index of this item in the menu
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radial Menu Item Button")
    int32 ItemIndex = -1;

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Radial Menu Item Button")
    void OnMenuItemDataSet(const FRadialMenuItem& InMenuItemData, int32 InItemIndex);

private:
    // Handle button click
    UFUNCTION()
    void HandleButtonClicked();
};

