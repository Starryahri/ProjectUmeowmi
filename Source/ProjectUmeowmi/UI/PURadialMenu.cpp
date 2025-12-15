#include "PURadialMenu.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Blueprint/WidgetTree.h"
#include "PURadialMenuItemButton.h"

UPURadialMenu::UPURadialMenu(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bIsVisible(false)
    , ItemRadius(100.0f)
{
}

void UPURadialMenu::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::NativeConstruct - Radial menu widget constructed: %s"), *GetName());

    // Hide menu by default
    SetVisibility(ESlateVisibility::Collapsed);
    bIsVisible = false;
}

void UPURadialMenu::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::NativeDestruct - Radial menu widget destroyed: %s"), *GetName());

    Super::NativeDestruct();
}

void UPURadialMenu::SetMenuItems(const TArray<FRadialMenuItem>& InMenuItems)
{
    MenuItems = InMenuItems;

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::SetMenuItems - Setting %d menu items"), MenuItems.Num());

    // Clear existing menu items
    ClearMenuItems();

    // Update the menu layout (this will create and position buttons)
    UpdateMenuLayout();

    // Call Blueprint event
    OnMenuItemsSet(MenuItems);
}

void UPURadialMenu::ShowMenuAtPosition(const FVector2D& ScreenPosition)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Showing menu at position (%.2f, %.2f)"), 
        ScreenPosition.X, ScreenPosition.Y);

    // Set the center position
    MenuCenterPosition = ScreenPosition;
    SetMenuCenterPosition(ScreenPosition);

    // Only add to viewport if not already in a parent widget
    if (!GetParent() && !IsInViewport())
    {
        AddToViewport(9999); // Use very high z-order to ensure it's on top
        UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Added menu to viewport with z-order 9999"));
    }
    else if (GetParent())
    {
        // Already in a container, just make sure it's visible
        UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Menu already in container: %s"), *GetParent()->GetName());
    }
    else if (IsInViewport())
    {
        // Already in viewport, remove and re-add to bring to front
        RemoveFromParent();
        AddToViewport(9999);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Removed and re-added menu to viewport with z-order 9999"));
    }

    // Get widget size to center it properly
    FVector2D WidgetSize = GetDesiredSize();
    if (WidgetSize.X == 0 || WidgetSize.Y == 0)
    {
        // If size is zero, use a default size (should be set in Blueprint)
        WidgetSize = FVector2D(400.0f, 400.0f);
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPURadialMenu::ShowMenuAtPosition - Widget size is zero, using default 400x400"));
    }

    // Set position in viewport (SetPositionInViewport uses top-left corner, so offset by half size to center)
    FVector2D TopLeftPosition = ScreenPosition - (WidgetSize * 0.5f);
    SetPositionInViewport(TopLeftPosition);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Widget size: (%.2f, %.2f), Top-left position: (%.2f, %.2f)"), 
        WidgetSize.X, WidgetSize.Y, TopLeftPosition.X, TopLeftPosition.Y);

    // Show the menu
    SetVisibility(ESlateVisibility::Visible);
    bIsVisible = true;

    // Force layout so hit-testing is correct on the first click (avoid double-click issue)
    ForceLayoutPrepass();
    if (MenuItemsContainer)
    {
        MenuItemsContainer->ForceLayoutPrepass();
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Menu visibility set to Visible, bIsVisible: %s"), 
        bIsVisible ? TEXT("TRUE") : TEXT("FALSE"));

    // Call Blueprint event
    OnMenuShown();
}

void UPURadialMenu::HideMenu()
{
    if (!bIsVisible)
    {
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::HideMenu - Hiding menu"));

    // Hide the menu
    SetVisibility(ESlateVisibility::Collapsed);
    bIsVisible = false;

    // Note: Don't remove from parent/viewport here - keep it in the hierarchy
    // Just hide it so it can be shown again quickly

    // Call Blueprint event
    OnMenuHidden();

    // Broadcast close event
    OnMenuClosed.Broadcast();
}

void UPURadialMenu::SetMenuCenterPosition(const FVector2D& CenterPosition)
{
    MenuCenterPosition = CenterPosition;
    UpdateMenuLayout();
}

void UPURadialMenu::SetItemRadius(float InRadius)
{
    if (ItemRadius != InRadius)
    {
        ItemRadius = InRadius;
        UpdateMenuLayout(); // Update layout when radius changes
    }
}

void UPURadialMenu::SetLayoutCenterPosition(const FVector2D& InCenterPosition)
{
    LayoutCenterPosition = InCenterPosition;
    bUseCustomCenter = true;
    UpdateMenuLayout();
}

void UPURadialMenu::UpdateMenuLayout()
{
    int32 ItemCount = MenuItems.Num();
    if (ItemCount == 0)
    {
        return;
    }

    if (!MenuItemsContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPURadialMenu::UpdateMenuLayout - MenuItemsContainer not set! Cannot create menu items."));
        return;
    }

    // Calculate angle step between items (360 degrees / number of items)
    float AngleStep = 360.0f / ItemCount;

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::UpdateMenuLayout - Updating layout for %d items (angle step: %.2f degrees, radius: %.2f)"), 
        ItemCount, AngleStep, ItemRadius);

    // Get container size - try multiple methods to ensure we get a valid size
    FVector2D ContainerSize = FVector2D::ZeroVector;
    
    // Force geometry update first to ensure we have valid cached geometry
    MenuItemsContainer->InvalidateLayoutAndVolatility();
    
    // Try GetCachedGeometry first (most reliable at runtime)
    FGeometry ContainerGeometry = MenuItemsContainer->GetCachedGeometry();
    if (ContainerGeometry.GetLocalSize().X > 0 && ContainerGeometry.GetLocalSize().Y > 0)
    {
        ContainerSize = ContainerGeometry.GetLocalSize();
    }
    else if (ContainerGeometry.GetAbsoluteSize().X > 0 && ContainerGeometry.GetAbsoluteSize().Y > 0)
    {
        ContainerSize = ContainerGeometry.GetAbsoluteSize();
    }
    
    // If that's zero, try GetDesiredSize
    if (ContainerSize.X == 0 || ContainerSize.Y == 0)
    {
        ContainerSize = MenuItemsContainer->GetDesiredSize();
    }
    
    // If still zero, try getting from parent widget
    if (ContainerSize.X == 0 || ContainerSize.Y == 0)
    {
        if (UWidget* Parent = MenuItemsContainer->GetParent())
        {
            FGeometry ParentGeometry = Parent->GetCachedGeometry();
            if (ParentGeometry.GetLocalSize().X > 0 && ParentGeometry.GetLocalSize().Y > 0)
            {
                ContainerSize = ParentGeometry.GetLocalSize();
            }
            else
            {
                FVector2D ParentSize = Parent->GetDesiredSize();
                if (ParentSize.X > 0 && ParentSize.Y > 0)
                {
                    ContainerSize = ParentSize;
                }
            }
        }
    }
    
    // Last resort: use default size
    if (ContainerSize.X == 0 || ContainerSize.Y == 0)
    {
        ContainerSize = FVector2D(400.0f, 400.0f);
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPURadialMenu::UpdateMenuLayout - Container size is zero, using default 400x400"));
    }
    
    // Calculate center position
    FVector2D CenterPosition;
    if (bUseCustomCenter)
    {
        CenterPosition = LayoutCenterPosition;
        UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::UpdateMenuLayout - Using custom center: (%.2f, %.2f)"), 
            CenterPosition.X, CenterPosition.Y);
    }
    else
    {
        CenterPosition = ContainerSize * 0.5f;
        UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::UpdateMenuLayout - Container size: (%.2f, %.2f), Auto center: (%.2f, %.2f)"), 
            ContainerSize.X, ContainerSize.Y, CenterPosition.X, CenterPosition.Y);
    }

    // Create and position buttons for each menu item
    for (int32 i = 0; i < ItemCount; ++i)
    {
        // Calculate angle for this item (start at -90 degrees so first item is at top)
        float AngleDegrees = (-90.0f + (AngleStep * i));
        float AngleRadians = FMath::DegreesToRadians(AngleDegrees);

        // Calculate position using trigonometry
        float X = CenterPosition.X + (ItemRadius * FMath::Cos(AngleRadians));
        float Y = CenterPosition.Y + (ItemRadius * FMath::Sin(AngleRadians));

        // Create the button for this menu item
        UPURadialMenuItemButton* ItemButton = CreateMenuItemButton(MenuItems[i], AngleDegrees, i);
        if (ItemButton)
        {
            // Position the button
            if (UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(ItemButton->Slot))
            {
                ButtonSlot->SetPosition(FVector2D(X - (MenuItemSize.X * 0.5f), Y - (MenuItemSize.Y * 0.5f)));
                ButtonSlot->SetSize(MenuItemSize);
                ButtonSlot->SetAnchors(FAnchors(0.5f, 0.5f));
                ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));

                UE_LOG(LogTemp, Display, TEXT("🎯   Positioned item %d at angle %.2f° at position (%.2f, %.2f)"), 
                    i, AngleDegrees, X, Y);
            }
        }
    }

    // Ensure layout is up to date so clicks register immediately
    ForceLayoutPrepass();
    if (MenuItemsContainer)
    {
        MenuItemsContainer->ForceLayoutPrepass();
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::UpdateMenuLayout - Created and positioned %d menu item buttons"), MenuItemButtons.Num());
}

void UPURadialMenu::ClearMenuItems()
{
    // Remove all button widgets
    for (UPURadialMenuItemButton* Button : MenuItemButtons)
    {
        if (Button && MenuItemsContainer)
        {
            MenuItemsContainer->RemoveChild(Button);
            Button->RemoveFromParent();
        }
    }

    MenuItemButtons.Empty();
    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::ClearMenuItems - Cleared all menu item buttons"));
}

UPURadialMenuItemButton* UPURadialMenu::CreateMenuItemButton(const FRadialMenuItem& MenuItem, float Angle, int32 Index)
{
    if (!MenuItemsContainer)
    {
        return nullptr;
    }

    // Create button widget
    UPURadialMenuItemButton* ItemButton = nullptr;
    
    if (MenuItemButtonClass)
    {
        // Use WidgetTree to construct child widgets
        ItemButton = WidgetTree->ConstructWidget<UPURadialMenuItemButton>(MenuItemButtonClass);
    }
    else
    {
        // Create default radial menu item button if no class is specified
        ItemButton = WidgetTree->ConstructWidget<UPURadialMenuItemButton>(UPURadialMenuItemButton::StaticClass());
    }

    if (!ItemButton)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPURadialMenu::CreateMenuItemButton - Failed to create button widget"));
        return nullptr;
    }

    // Add to container
    MenuItemsContainer->AddChild(ItemButton);

    // Set menu item data (this will configure the button and store the index)
    ItemButton->SetMenuItemData(MenuItem, Index);
    
    // Bind to the button's OnItemClicked event
    ItemButton->OnItemClicked.AddDynamic(this, &UPURadialMenu::SelectMenuItemByIndex);
    
    // Store the button
    MenuItemButtons.Add(ItemButton);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::CreateMenuItemButton - Created button for item %d: %s (Enabled: %s)"), 
        Index, *MenuItem.Label.ToString(), MenuItem.bIsEnabled ? TEXT("YES") : TEXT("NO"));

    return ItemButton;
}


void UPURadialMenu::SelectMenuItemByIndex(int32 ItemIndex)
{
    if (!MenuItems.IsValidIndex(ItemIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPURadialMenu::SelectMenuItemByIndex - Invalid item index: %d"), ItemIndex);
        return;
    }

    const FRadialMenuItem& SelectedItem = MenuItems[ItemIndex];
    
    if (!SelectedItem.bIsEnabled)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPURadialMenu::SelectMenuItemByIndex - Item %d is disabled"), ItemIndex);
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::SelectMenuItemByIndex - Item %d selected: %s"), 
        ItemIndex, *SelectedItem.Label.ToString());

    // Broadcast the selection event
    OnMenuItemSelected.Broadcast(SelectedItem);
}

void UPURadialMenu::PreviewRadialLayout()
{
    if (!MenuItemsContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPURadialMenu::PreviewRadialLayout - MenuItemsContainer not set"));
        return;
    }

    // Get all child widgets from the container
    int32 ChildCount = MenuItemsContainer->GetChildrenCount();
    if (ChildCount == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::PreviewRadialLayout - No children to arrange"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::PreviewRadialLayout - Arranging %d children in radial layout"), ChildCount);

    // Force layout invalidation to ensure geometry is up to date
    MenuItemsContainer->InvalidateLayoutAndVolatility();

    // Get container size - use same improved logic as UpdateMenuLayout
    FVector2D ContainerSize = FVector2D::ZeroVector;
    
    // Try GetCachedGeometry first (most reliable at runtime and in designer)
    FGeometry ContainerGeometry = MenuItemsContainer->GetCachedGeometry();
    if (ContainerGeometry.GetLocalSize().X > 0 && ContainerGeometry.GetLocalSize().Y > 0)
    {
        ContainerSize = ContainerGeometry.GetLocalSize();
    }
    else if (ContainerGeometry.GetAbsoluteSize().X > 0 && ContainerGeometry.GetAbsoluteSize().Y > 0)
    {
        ContainerSize = ContainerGeometry.GetAbsoluteSize();
    }
    
    // If that's zero, try GetDesiredSize
    if (ContainerSize.X == 0 || ContainerSize.Y == 0)
    {
        ContainerSize = MenuItemsContainer->GetDesiredSize();
    }
    
    // If still zero, try getting from parent widget
    if (ContainerSize.X == 0 || ContainerSize.Y == 0)
    {
        if (UWidget* Parent = MenuItemsContainer->GetParent())
        {
            FGeometry ParentGeometry = Parent->GetCachedGeometry();
            if (ParentGeometry.GetLocalSize().X > 0 && ParentGeometry.GetLocalSize().Y > 0)
            {
                ContainerSize = ParentGeometry.GetLocalSize();
            }
            else
            {
                FVector2D ParentSize = Parent->GetDesiredSize();
                if (ParentSize.X > 0 && ParentSize.Y > 0)
                {
                    ContainerSize = ParentSize;
                }
            }
        }
    }
    
    // Last resort: use default size (but log a warning)
    if (ContainerSize.X == 0 || ContainerSize.Y == 0)
    {
        ContainerSize = FVector2D(400.0f, 400.0f);
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPURadialMenu::PreviewRadialLayout - Container size is zero, using default 400x400. Make sure container has a size set in Blueprint!"));
    }

    // Calculate center position (use same logic as UpdateMenuLayout)
    FVector2D CenterPosition;
    if (bUseCustomCenter)
    {
        CenterPosition = LayoutCenterPosition;
    }
    else
    {
        CenterPosition = ContainerSize * 0.5f;
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::PreviewRadialLayout - Container size: (%.2f, %.2f), Center: (%.2f, %.2f)"), 
        ContainerSize.X, ContainerSize.Y, CenterPosition.X, CenterPosition.Y);

    // Calculate angle step between items
    float AngleStep = 360.0f / ChildCount;

    // Position each child widget in a circle
    for (int32 i = 0; i < ChildCount; ++i)
    {
        UWidget* ChildWidget = MenuItemsContainer->GetChildAt(i);
        if (!ChildWidget)
        {
            continue;
        }

        // Calculate angle for this item (start at -90 degrees so first item is at top)
        float AngleDegrees = (-90.0f + (AngleStep * i));
        float AngleRadians = FMath::DegreesToRadians(AngleDegrees);

        // Calculate position using trigonometry
        float X = CenterPosition.X + (ItemRadius * FMath::Cos(AngleRadians));
        float Y = CenterPosition.Y + (ItemRadius * FMath::Sin(AngleRadians));

        // Position the widget
        if (UCanvasPanelSlot* WidgetSlot = Cast<UCanvasPanelSlot>(ChildWidget->Slot))
        {
            WidgetSlot->SetPosition(FVector2D(X - (MenuItemSize.X * 0.5f), Y - (MenuItemSize.Y * 0.5f)));
            WidgetSlot->SetSize(MenuItemSize);
            WidgetSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            WidgetSlot->SetAlignment(FVector2D(0.5f, 0.5f));

            UE_LOG(LogTemp, Display, TEXT("🎯   Positioned child %d at angle %.2f° at position (%.2f, %.2f)"), 
                i, AngleDegrees, X, Y);
        }
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenu::PreviewRadialLayout - Arranged %d widgets in radial layout"), ChildCount);
}

