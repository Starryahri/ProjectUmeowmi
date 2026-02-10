#include "PURadialMenu.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"
#include "Blueprint/WidgetTree.h"
#include "PURadialMenuItemButton.h"
#include "Input/Events.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Components/CanvasPanel.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/STextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Engine/Engine.h"

UPURadialMenu::UPURadialMenu(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bIsVisible(false)
    , PreparationDataTable(nullptr)
    , ItemRadius(100.0f)
{
}

void UPURadialMenu::NativeConstruct()
{
    Super::NativeConstruct();

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::NativeConstruct - Radial menu widget constructed: %s"), *GetName());

    // Hide menu by default
    SetVisibility(ESlateVisibility::Collapsed);
    bIsVisible = false;
}

void UPURadialMenu::NativeDestruct()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::NativeDestruct - Radial menu widget destroyed: %s"), *GetName());

    // Clear delegates first to break any circular references
    OnMenuItemSelected.Clear();
    OnMenuClosed.Clear();
    
    // Clear buttons first (which also clears their MenuItemData)
    // This must happen before we touch MenuItems to avoid GC issues
    ClearMenuItems();
    
    // Clear the MenuItems array immediately - don't iterate over it
    // The UPROPERTY on the array should handle GC, but we clear it to be safe
    // Iterating over it during GC can cause crashes, so just empty it
    MenuItems.Empty();

    Super::NativeDestruct();
}

void UPURadialMenu::SetMenuItems(const TArray<FRadialMenuItem>& InMenuItems)
{
    MenuItems = InMenuItems;

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::SetMenuItems - Setting %d menu items"), MenuItems.Num());

    // Clear existing menu items
    ClearMenuItems();

    // Update the menu layout (this will create and position buttons)
    UpdateMenuLayout();

    // Call Blueprint event
    OnMenuItemsSet(MenuItems);
}

void UPURadialMenu::ShowMenuAtPosition(const FVector2D& ScreenPosition)
{
    // Debug logs removed - use bShowDebugText if you need to see this

    // Reset selected index to first item when menu opens
    SelectedMenuItemIndex = 0;
    
    // Set the center position
    MenuCenterPosition = ScreenPosition;
    SetMenuCenterPosition(ScreenPosition);

    // Only add to viewport if not already in a parent widget
    if (!GetParent() && !IsInViewport())
    {
        AddToViewport(9999); // Use very high z-order to ensure it's on top
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Added menu to viewport with z-order 9999"));
    }
    else if (GetParent())
    {
        // Already in a container, just make sure it's visible
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Menu already in container: %s"), *GetParent()->GetName());
    }
    else if (IsInViewport())
    {
        // Already in viewport, remove and re-add to bring to front
        RemoveFromParent();
        AddToViewport(9999);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Removed and re-added menu to viewport with z-order 9999"));
    }

    // Get widget size to center it properly
    FVector2D WidgetSize = GetDesiredSize();
    if (WidgetSize.X == 0 || WidgetSize.Y == 0)
    {
        // If size is zero, use a default size (should be set in Blueprint)
        WidgetSize = FVector2D(400.0f, 400.0f);
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPURadialMenu::ShowMenuAtPosition - Widget size is zero, using default 400x400"));
    }

    // Set position in viewport (SetPositionInViewport uses top-left corner, so offset by half size to center)
    FVector2D TopLeftPosition = ScreenPosition - (WidgetSize * 0.5f);
    SetPositionInViewport(TopLeftPosition);

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Widget size: (%.2f, %.2f), Top-left position: (%.2f, %.2f)"), 
    //    WidgetSize.X, WidgetSize.Y, TopLeftPosition.X, TopLeftPosition.Y);

    // Show the menu
    SetVisibility(ESlateVisibility::Visible);
    bIsVisible = true;

    // Force layout so hit-testing is correct on the first click (avoid double-click issue)
    ForceLayoutPrepass();
    if (MenuItemsContainer)
    {
        MenuItemsContainer->ForceLayoutPrepass();
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::ShowMenuAtPosition - Menu visibility set to Visible, bIsVisible: %s"), 
    //    bIsVisible ? TEXT("TRUE") : TEXT("FALSE"));

    // Call Blueprint event
    OnMenuShown();
    
    // Reset selected index when menu opens
    SelectedMenuItemIndex = 0;
    
    // Update the visual indicator
    UpdateSelectionIndicator();
    
    // Set focus to the first menu item after a short delay
    if (UWorld* World = GetWorld())
    {
        FTimerHandle FocusTimerHandle;
        TWeakObjectPtr<UPURadialMenu> WeakThis = this;
        World->GetTimerManager().SetTimer(FocusTimerHandle, [WeakThis]()
        {
            if (UPURadialMenu* Menu = WeakThis.Get())
            {
                if (IsValid(Menu))
                {
                    Menu->SetFocusToSelectedMenuItem();
                    Menu->UpdateSelectionIndicator();
                }
            }
        }, 0.15f, false);
    }
}

void UPURadialMenu::HideMenu()
{
    if (!bIsVisible)
    {
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::HideMenu - Hiding menu"));

    // Hide the menu
    SetVisibility(ESlateVisibility::Collapsed);
    bIsVisible = false;
    
    // Hide the selection indicator
    if (SelectionIndicator && IsValid(SelectionIndicator))
    {
        SelectionIndicator->SetVisibility(ESlateVisibility::Collapsed);
    }
    
    // Hide the direction line
    bShouldDrawDirectionLine = false;
    CurrentDirectionLength = 0.0f;
    CurrentInputMagnitude = 0.0f;
    if (IsValid(this))
    {
        Invalidate(EInvalidateWidget::Paint);
    }

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
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPURadialMenu::UpdateMenuLayout - MenuItemsContainer not set! Cannot create menu items."));
        return;
    }

    // Calculate angle step between items (360 degrees / number of items)
    float AngleStep = 360.0f / ItemCount;

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::UpdateMenuLayout - Updating layout for %d items (angle step: %.2f degrees, radius: %.2f)"), 
    //    ItemCount, AngleStep, ItemRadius);

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
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPURadialMenu::UpdateMenuLayout - Container size is zero, using default 400x400"));
    }
    
    // Calculate center position
    FVector2D CenterPosition;
    if (bUseCustomCenter)
    {
        CenterPosition = LayoutCenterPosition;
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::UpdateMenuLayout - Using custom center: (%.2f, %.2f)"), 
        //    CenterPosition.X, CenterPosition.Y);
    }
    else
    {
        CenterPosition = ContainerSize * 0.5f;
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::UpdateMenuLayout - Container size: (%.2f, %.2f), Auto center: (%.2f, %.2f)"), 
        //    ContainerSize.X, ContainerSize.Y, CenterPosition.X, CenterPosition.Y);
    }

    // Debug log removed - use bShowDebugText if you need to see this
    
    // Create and position buttons for each menu item
    for (int32 i = 0; i < ItemCount; ++i)
    {
        // Calculate angle for this item (start at -90 degrees so first item is at top)
        // This gives us standard math angles: 0° = right, 90° = up, 180° = left, 270° = down
        // Normalize to 0-360 range to match GetMenuItemAngle()
        float AngleDegrees = (-90.0f + (AngleStep * i));
        if (AngleDegrees < 0.0f)
        {
            AngleDegrees += 360.0f;
        }
        float AngleRadians = FMath::DegreesToRadians(AngleDegrees);

        // Calculate position using trigonometry
        // In Unreal's UI: X increases right, Y increases down
        // To align buttons with stick input (which uses Atan2(-Y, X)), we need to flip the Y axis
        // Use +Sin instead of -Sin to flip 180° so buttons match stick direction
        float X = CenterPosition.X + (ItemRadius * FMath::Cos(AngleRadians));
        float Y = CenterPosition.Y + (ItemRadius * FMath::Sin(AngleRadians)); // Use +Sin to flip orientation

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

                // Debug log removed - use bShowDebugText if you need to see this
            }
        }
    }

    // Ensure layout is up to date so clicks register immediately
    ForceLayoutPrepass();
    if (MenuItemsContainer)
    {
        MenuItemsContainer->ForceLayoutPrepass();
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::UpdateMenuLayout - Created and positioned %d menu item buttons"), MenuItemButtons.Num());
}

void UPURadialMenu::ClearMenuItems()
{
    // Make a copy of the array to avoid iterating over it if GC happens during iteration
    TArray<UPURadialMenuItemButton*> ButtonsToClear = MenuItemButtons;
    
    // Unbind delegates and clear MenuItemData from all button widgets before removing them
    // This is critical to prevent GC from accessing invalid texture pointers
    for (UPURadialMenuItemButton* Button : ButtonsToClear)
    {
        if (IsValid(Button))
        {
            // Unbind the button's click event from this menu
            if (Button->OnItemClicked.IsBound())
            {
                Button->OnItemClicked.RemoveDynamic(this, &UPURadialMenu::SelectMenuItemByIndex);
            }
            
            // Explicitly clear the MenuItemData to null out texture pointers
            // Use ClearMenuItemData() instead of SetMenuItemData() to avoid triggering
            // Blueprint events or accessing other UObjects during GC
            Button->ClearMenuItemData();
        }
    }
    
    // Remove all button widgets from container
    for (UPURadialMenuItemButton* Button : ButtonsToClear)
    {
        if (IsValid(Button) && MenuItemsContainer)
        {
            MenuItemsContainer->RemoveChild(Button);
            Button->RemoveFromParent();
        }
    }

    MenuItemButtons.Empty();
    
    // NOTE: Do NOT clear MenuItems array here - it's needed for UpdateMenuLayout()
    // MenuItems array is only cleared in NativeDestruct() to prevent GC crashes
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::ClearMenuItems - Cleared all menu item buttons"));
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
        //UE_LOG(LogTemp,Error, TEXT("❌ UPURadialMenu::CreateMenuItemButton - Failed to create button widget"));
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

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::CreateMenuItemButton - Created button for item %d: %s (Enabled: %s)"), 
    //    Index, *MenuItem.Label.ToString(), MenuItem.bIsEnabled ? TEXT("YES") : TEXT("NO"));

    return ItemButton;
}


void UPURadialMenu::SelectMenuItemByIndex(int32 ItemIndex)
{
    if (!MenuItems.IsValidIndex(ItemIndex))
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPURadialMenu::SelectMenuItemByIndex - Invalid item index: %d (MenuItems has %d items)"), 
        //    ItemIndex, MenuItems.Num());
        return;
    }

    const FRadialMenuItem& SelectedItem = MenuItems[ItemIndex];
    
    if (!SelectedItem.bIsEnabled)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPURadialMenu::SelectMenuItemByIndex - Item %d is disabled"), ItemIndex);
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::SelectMenuItemByIndex - Item %d selected: %s (Tag: %s)"), 
    //    ItemIndex, *SelectedItem.Label.ToString(), *SelectedItem.ActionTag.ToString());

    // Log all menu items for debugging
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::SelectMenuItemByIndex - All menu items:"));
    for (int32 i = 0; i < MenuItems.Num(); ++i)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯   [%d] %s (Tag: %s, Enabled: %s)"), 
        //    i, *MenuItems[i].Label.ToString(), *MenuItems[i].ActionTag.ToString(), 
        //    MenuItems[i].bIsEnabled ? TEXT("YES") : TEXT("NO"));
    }

    // Broadcast the selection event
    OnMenuItemSelected.Broadcast(SelectedItem);
}

void UPURadialMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Only handle joystick input if menu is visible
    if (!bIsVisible || MenuItemButtons.Num() == 0)
    {
        JoystickSelectionCooldown = 0.0f;
        return;
    }
    
    // Debug logs removed - use bShowDebugText if you need to see this
    
    // Update cooldown
    if (JoystickSelectionCooldown > 0.0f)
    {
        JoystickSelectionCooldown -= InDeltaTime;
    }
    
    // Get player controller to read joystick input
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            // Get left stick input values
            float LeftStickX = 0.0f;
            float LeftStickY = 0.0f;
            
            // Get analog stick values
            if (PC->InputComponent)
            {
                LeftStickX = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
                LeftStickY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
                
                // Debug: Log raw input values (only when there's significant input to avoid spam)
                if (FMath::Abs(LeftStickX) > 0.1f || FMath::Abs(LeftStickY) > 0.1f)
                {
                    UE_LOG(LogTemp, Warning, TEXT("🎮 Raw Stick Input - X: %.2f, Y: %.2f"), LeftStickX, LeftStickY);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("🎮 UPURadialMenu::NativeTick - InputComponent is NULL!"));
            }
            
            // Apply deadzone
            FVector2D StickInput(LeftStickX, LeftStickY);
            float InputMagnitude = StickInput.Size();
            
            if (InputMagnitude < JoystickDeadzone)
            {
                // Input is below deadzone, hide the direction line
                bShouldDrawDirectionLine = false;
                CurrentDirectionLength = 0.0f;
                CurrentInputMagnitude = 0.0f;
                CurrentStickX = 0.0f;
                CurrentStickY = 0.0f;
                if (IsValid(this))
                {
                    Invalidate(EInvalidateWidget::Paint);
                }
                return;
            }
            
            // Normalize input
            StickInput.Normalize();
            
            // Calculate angle in degrees (0-360, where 0 is right, 90 is up, 180 is left, 270 is down)
            // In Unreal's input: stick up = negative Y, stick right = positive X
            // We want standard math angles: 0° = right, 90° = up, 180° = left, 270° = down
            // Atan2(-Y, X) converts from Unreal's input (up = -Y) to standard math (up = +Y)
            // This gives us: up stick → Atan2(1, 0) = 90°, right stick → Atan2(0, 1) = 0°
            float AngleRadians = FMath::Atan2(-StickInput.Y, StickInput.X);
            float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);
            
            // Convert to 0-360 range (atan2 returns -180 to 180)
            if (AngleDegrees < 0.0f)
            {
                AngleDegrees += 360.0f;
            }
            
            // Update the direction line to show where the stick is pointing
            // Use normalized magnitude (0.0 to 1.0) for line length
            float NormalizedMagnitude = FMath::Clamp((InputMagnitude - JoystickDeadzone) / (1.0f - JoystickDeadzone), 0.0f, 1.0f);
            
            // Store stick values for on-screen display
            CurrentStickX = LeftStickX;
            CurrentStickY = LeftStickY;
            
            // Debug log removed - use bShowDebugText if you need to see this
            
            UpdateDirectionLine(AngleDegrees, NormalizedMagnitude);
            
            // Only update selection if cooldown is expired
            if (JoystickSelectionCooldown <= 0.0f)
            {
                SelectMenuItemByAngle(AngleDegrees);
                JoystickSelectionCooldown = JoystickSelectionRepeatDelay;
            }
        }
        else
        {
            // No player controller, hide the direction line
            bShouldDrawDirectionLine = false;
            CurrentDirectionLength = 0.0f;
            CurrentInputMagnitude = 0.0f;
            if (IsValid(this))
            {
                Invalidate(EInvalidateWidget::Paint);
            }
        }
    }
}

float UPURadialMenu::GetMenuItemAngle(int32 ItemIndex) const
{
    // Cache array size to avoid accessing during GC
    const int32 NumButtons = MenuItemButtons.Num();
    if (NumButtons == 0)
    {
        return 0.0f;
    }
    
    // Calculate angle step between items (360 degrees / number of items)
    float AngleStep = 360.0f / NumButtons;
    
    // Calculate angle for this item (start at -90 degrees so first item is at top)
    // Then convert to 0-360 range
    float AngleDegrees = (-90.0f + (AngleStep * ItemIndex));
    if (AngleDegrees < 0.0f)
    {
        AngleDegrees += 360.0f;
    }
    
    // Debug log (only log once per item when menu is shown, use Warning level for visibility)
    static TSet<int32> LoggedItems;
    if (bIsVisible && !LoggedItems.Contains(ItemIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯 Menu Item %d - Angle: %.2f° (AngleStep: %.2f°, Total Items: %d)"), 
            ItemIndex, AngleDegrees, AngleStep, NumButtons);
        LoggedItems.Add(ItemIndex);
        
        // Clear logged items when menu closes (reset when all items are logged)
        if (LoggedItems.Num() >= NumButtons)
        {
            LoggedItems.Empty();
        }
    }
    
    return AngleDegrees;
}

void UPURadialMenu::SelectMenuItemByAngle(float AngleDegrees)
{
    // Cache array sizes to avoid accessing during potential GC
    const int32 NumButtons = MenuItemButtons.Num();
    const int32 NumMenuItems = MenuItems.Num();
    
    if (NumButtons == 0 || !MenuItemsContainer)
    {
        return;
    }
    
    // Convert stick angle to direction vector (standard math: 0° = right, 90° = up)
    float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
    FVector2D StickDirection(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians));
    
    // Get the center position of the menu (use the layout center)
    FVector2D CenterPos = bUseCustomCenter ? LayoutCenterPosition : MenuCenterPosition;
    
    // If we don't have a valid center, try to get it from the container
    if (CenterPos.IsNearlyZero())
    {
        FGeometry ContainerGeometry = MenuItemsContainer->GetCachedGeometry();
        FVector2D ContainerSize = ContainerGeometry.GetLocalSize();
        CenterPos = ContainerSize * 0.5f;
    }
    
    // Find the menu item closest to the stick direction
    int32 ClosestIndex = 0;
    float SmallestAngleDiff = 360.0f;
    
    for (int32 i = 0; i < NumButtons; ++i)
    {
        // Skip disabled items - use cached size check
        if (i >= 0 && i < NumMenuItems && !MenuItems[i].bIsEnabled)
        {
            continue;
        }
        
        // Use cached size check instead of array access
        if (i < 0 || i >= NumButtons || !IsValid(MenuItemButtons[i]))
        {
            continue;
        }
        
        // Get the actual button position from its slot
        FVector2D ButtonPos = CenterPos; // Default to center if we can't get position
        if (UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(MenuItemButtons[i]->Slot))
        {
            FVector2D SlotPos = ButtonSlot->GetPosition();
            FVector2D SlotSize = ButtonSlot->GetSize();
            // Button center position = slot position + half size
            ButtonPos = SlotPos + (SlotSize * 0.5f);
        }
        
        // Calculate direction from center to button
        FVector2D ButtonDirection = (ButtonPos - CenterPos);
        float ButtonDistance = ButtonDirection.Size();
        
        if (ButtonDistance > 0.01f) // Avoid division by zero
        {
            ButtonDirection.Normalize();
            
            // Calculate angle of button direction
            float ButtonAngleRadians = FMath::Atan2(ButtonDirection.Y, ButtonDirection.X);
            float ButtonAngleDegrees = FMath::RadiansToDegrees(ButtonAngleRadians);
            if (ButtonAngleDegrees < 0.0f)
            {
                ButtonAngleDegrees += 360.0f;
            }
            
            // Calculate the difference between joystick angle and button angle
            // Handle wrap-around (e.g., 350° and 10° are only 20° apart)
            float AngleDiff = FMath::Abs(AngleDegrees - ButtonAngleDegrees);
            if (AngleDiff > 180.0f)
            {
                AngleDiff = 360.0f - AngleDiff;
            }
            
            if (AngleDiff < SmallestAngleDiff)
            {
                SmallestAngleDiff = AngleDiff;
                ClosestIndex = i;
            }
        }
    }
    
    // Only update if the selection changed
    if (ClosestIndex != SelectedMenuItemIndex)
    {
        NavigateToMenuItem(ClosestIndex);
        UpdateSelectionIndicator();
    }
}

FReply UPURadialMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (!bIsVisible)
    {
        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
    }
    
    FKey Key = InKeyEvent.GetKey();
    
    UE_LOG(LogTemp, Log, TEXT("🎮 UPURadialMenu::NativeOnKeyDown - Key pressed: %s (Menu visible: %s, Items: %d, Selected: %d)"), 
        *Key.ToString(), bIsVisible ? TEXT("YES") : TEXT("NO"), MenuItemButtons.Num(), SelectedMenuItemIndex);
    
    // Handle B button (Gamepad Face Button Right) or Escape to close menu
    if (Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Escape)
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 UPURadialMenu::NativeOnKeyDown - Close button pressed, hiding menu"));
        HideMenu();
        return FReply::Handled();
    }
    
    // Handle A button (Gamepad Face Button Bottom) to select current item
    if (Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Enter || Key == EKeys::SpaceBar)
    {
        if (MenuItemButtons.IsValidIndex(SelectedMenuItemIndex))
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPURadialMenu::NativeOnKeyDown - Select button pressed, selecting item %d"), SelectedMenuItemIndex);
            SelectMenuItemByIndex(SelectedMenuItemIndex);
            return FReply::Handled();
        }
    }
    
    // Handle D-pad navigation (left stick is handled in NativeTick for directional selection)
    bool bNavigated = false;
    if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Right)
    {
        // Navigate clockwise (next item)
        int32 NewIndex = (SelectedMenuItemIndex + 1) % MenuItemButtons.Num();
        NavigateToMenuItem(NewIndex);
        bNavigated = true;
    }
    else if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Left)
    {
        // Navigate counter-clockwise (previous item)
        int32 NewIndex = (SelectedMenuItemIndex - 1 + MenuItemButtons.Num()) % MenuItemButtons.Num();
        NavigateToMenuItem(NewIndex);
        bNavigated = true;
    }
    
    if (bNavigated)
    {
        return FReply::Handled();
    }
    
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPURadialMenu::NavigateToMenuItem(int32 NewIndex)
{
    if (!MenuItemButtons.IsValidIndex(NewIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UPURadialMenu::NavigateToMenuItem - Invalid index: %d (Total items: %d)"), 
            NewIndex, MenuItemButtons.Num());
        return;
    }
    
    // Skip disabled items
    int32 StartIndex = NewIndex;
    int32 Attempts = 0;
    while (Attempts < MenuItemButtons.Num())
    {
        if (MenuItems.IsValidIndex(NewIndex) && MenuItems[NewIndex].bIsEnabled)
        {
            break;
        }
        NewIndex = (NewIndex + 1) % MenuItemButtons.Num();
        Attempts++;
        
        // If we've gone full circle and all items are disabled, just use the original index
        if (NewIndex == StartIndex)
        {
            break;
        }
    }
    
    SelectedMenuItemIndex = NewIndex;
    SetFocusToSelectedMenuItem();
    UpdateSelectionIndicator();
    
    UE_LOG(LogTemp, Log, TEXT("🎮 UPURadialMenu::NavigateToMenuItem - Navigated to item %d: %s"), 
        SelectedMenuItemIndex, 
        MenuItems.IsValidIndex(SelectedMenuItemIndex) ? *MenuItems[SelectedMenuItemIndex].Label.ToString() : TEXT("INVALID"));
}

void UPURadialMenu::SetFocusToSelectedMenuItem()
{
    if (!MenuItemButtons.IsValidIndex(SelectedMenuItemIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UPURadialMenu::SetFocusToSelectedMenuItem - Invalid selected index: %d"), SelectedMenuItemIndex);
        return;
    }
    
    UPURadialMenuItemButton* SelectedButton = MenuItemButtons[SelectedMenuItemIndex];
    if (!SelectedButton || !IsValid(SelectedButton))
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UPURadialMenu::SetFocusToSelectedMenuItem - Selected button is invalid"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("🎮 UPURadialMenu::SetFocusToSelectedMenuItem - Setting focus to button widget %d"), SelectedMenuItemIndex);
    
    // Set focus to the widget itself (not the internal button component)
    // The widget is now focusable (set in NativeConstruct)
    SelectedButton->SetKeyboardFocus();
    FSlateApplication::Get().SetUserFocus(0, SelectedButton->TakeWidget());
    
    // Retry if focus wasn't set
    if (!SelectedButton->HasKeyboardFocus())
    {
        if (UWorld* World = GetWorld())
        {
            FTimerHandle RetryTimerHandle;
            TWeakObjectPtr<UPURadialMenuItemButton> WeakButton = SelectedButton;
            World->GetTimerManager().SetTimer(RetryTimerHandle, [WeakButton]()
            {
                if (UPURadialMenuItemButton* Button = WeakButton.Get())
                {
                    if (IsValid(Button))
                    {
                        Button->SetKeyboardFocus();
                        UE_LOG(LogTemp, Log, TEXT("🎮 UPURadialMenu::SetFocusToSelectedMenuItem - Retry: Focus set (HasFocus: %s)"), 
                            Button->HasKeyboardFocus() ? TEXT("YES") : TEXT("NO"));
                    }
                }
            }, 0.1f, false);
        }
    }
}

void UPURadialMenu::UpdateSelectionIndicator()
{
    if (!SelectionIndicator || !IsValid(SelectionIndicator))
    {
        // Indicator not set in Blueprint, that's okay
        return;
    }
    
    if (!MenuItemButtons.IsValidIndex(SelectedMenuItemIndex))
    {
        // Invalid selection, hide indicator
        SelectionIndicator->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    
    // Show the indicator
    SelectionIndicator->SetVisibility(ESlateVisibility::Visible);
    
    // Get the angle for the selected menu item
    float ItemAngle = GetMenuItemAngle(SelectedMenuItemIndex);
    
    // Convert to radians for rotation
    float AngleRadians = FMath::DegreesToRadians(ItemAngle);
    
    // The indicator should point in the direction of the selected item
    // We'll rotate it to point in that direction
    // Note: Widget rotation in Unreal is clockwise, and 0° is right, so we need to adjust
    // Our angle: 0° is right, positive is counter-clockwise
    // Widget rotation: 0° is right, positive is clockwise
    // So we need to negate the angle
    float RotationDegrees = -ItemAngle;
    
    // Set the rotation using render transform (works with any container type)
    SelectionIndicator->SetRenderTransformAngle(RotationDegrees);
    
    // Try to position it at the center if it's in a Canvas Panel
    if (UCanvasPanelSlot* IndicatorSlot = Cast<UCanvasPanelSlot>(SelectionIndicator->Slot))
    {
        // Get the center position of the menu
        FVector2D CenterPosition = MenuCenterPosition;
        
        // If we have a container, use its center
        if (MenuItemsContainer && IsValid(MenuItemsContainer))
        {
            FVector2D ContainerSize = MenuItemsContainer->GetDesiredSize();
            if (ContainerSize.X > 0 && ContainerSize.Y > 0)
            {
                CenterPosition = ContainerSize * 0.5f;
            }
        }
        
        // Position it at the center
        IndicatorSlot->SetPosition(CenterPosition);
        IndicatorSlot->SetZOrder(100); // Make sure it's on top
        
        UE_LOG(LogTemp, Log, TEXT("🎮 UPURadialMenu::UpdateSelectionIndicator - Updated indicator in Canvas Panel to point at item %d (Angle: %.2f°, Rotation: %.2f°)"), 
            SelectedMenuItemIndex, ItemAngle, RotationDegrees);
    }
    else
    {
        // If not in a canvas panel, just rotate the widget itself
        // The position should be set in Blueprint to be at the center
        UE_LOG(LogTemp, Log, TEXT("🎮 UPURadialMenu::UpdateSelectionIndicator - Updated indicator rotation (not in canvas panel, Angle: %.2f°, Rotation: %.2f°)"), 
            ItemAngle, RotationDegrees);
    }
}

int32 UPURadialMenu::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                 const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                 int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    // Call parent paint first
    LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
    
    // Get the center of the widget
    FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;
    
    // Cache array sizes and indices to avoid accessing arrays during GC
    const int32 NumButtons = MenuItemButtons.Num();
    const int32 NumMenuItems = MenuItems.Num();
    const int32 CachedSelectedIndex = SelectedMenuItemIndex;
    
    // Draw debug visualization if enabled
    if (bShowDebugRegions && bIsVisible && NumButtons > 0)
    {
        // Draw lines from center to each button to show regions
        for (int32 i = 0; i < NumButtons; ++i)
        {
            float ItemAngle = GetMenuItemAngle(i);
            float AngleRadians = FMath::DegreesToRadians(ItemAngle);
            
            // Calculate direction (same as button positioning - use +Sin to match flipped buttons)
            FVector2D Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians));
            FVector2D EndPoint = Center + (Direction * ItemRadius);
            
            // Draw region line (lighter color)
            TArray<FVector2D> RegionLinePoints;
            RegionLinePoints.Add(Center);
            RegionLinePoints.Add(EndPoint);
            
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId + 1,
                AllottedGeometry.ToPaintGeometry(),
                RegionLinePoints,
                ESlateDrawEffect::None,
                FLinearColor(0.5f, 0.5f, 0.5f, 0.5f), // Gray, semi-transparent
                false,
                1.0f
            );
        }
        
        // Draw the selected region more prominently
        if (CachedSelectedIndex >= 0 && CachedSelectedIndex < NumButtons)
        {
            float SelectedAngle = GetMenuItemAngle(CachedSelectedIndex);
            float AngleRadians = FMath::DegreesToRadians(SelectedAngle);
            FVector2D Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians)); // Use +Sin to match flipped buttons
            FVector2D EndPoint = Center + (Direction * ItemRadius);
            
            TArray<FVector2D> SelectedLinePoints;
            SelectedLinePoints.Add(Center);
            SelectedLinePoints.Add(EndPoint);
            
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(),
                SelectedLinePoints,
                ESlateDrawEffect::None,
                FLinearColor::Yellow, // Yellow for selected
                false,
                2.0f
            );
        }
        
        // Draw angle labels for each button
        const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 12);
        const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        
        for (int32 i = 0; i < NumButtons; ++i)
        {
            // Use cached size check instead of accessing array
            if (i < 0 || i >= NumButtons)
            {
                continue;
            }
            
            float ItemAngle = GetMenuItemAngle(i);
            float AngleRadians = FMath::DegreesToRadians(ItemAngle);
            
            // Calculate button position (same as how buttons are positioned)
            // Use +Sin to match the flipped button positioning
            FVector2D Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians));
            FVector2D ButtonPosition = Center + (Direction * ItemRadius);
            
            // Format text showing button index and angle
            FString AngleText = FString::Printf(TEXT("[%d] %.0f°"), i, ItemAngle);
            
            // Measure text size
            FVector2D TextSize = FontMeasure->Measure(AngleText, FontInfo);
            
            // Position text slightly offset from button (toward center)
            FVector2D TextOffset = -Direction * 15.0f; // Offset 15px toward center
            FVector2D TextPosition = ButtonPosition + TextOffset - (TextSize * 0.5f);
            
            // Draw text background (small semi-transparent rectangle)
            FVector2D BackgroundSize = TextSize + FVector2D(4.0f, 4.0f);
            FVector2D BackgroundPosition = TextPosition - FVector2D(2.0f, 2.0f);
            
            FSlateDrawElement::MakeBox(
                OutDrawElements,
                LayerId + 3,
                AllottedGeometry.ToPaintGeometry(BackgroundSize, FSlateLayoutTransform(BackgroundPosition)),
                FCoreStyle::Get().GetBrush("WhiteBrush"),
                ESlateDrawEffect::None,
                FLinearColor(0.0f, 0.0f, 0.0f, 0.6f) // Black with 60% opacity
            );
            
            // Draw the text
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 4,
                AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(TextPosition)),
                AngleText,
                FontInfo,
                ESlateDrawEffect::None,
                i == CachedSelectedIndex ? FLinearColor::Yellow : FLinearColor::White // Yellow for selected, white for others
            );
        }
    }
    
    // Draw the direction line if needed (using Slate, like radar graphs)
    if (bShouldDrawDirectionLine && CurrentDirectionLength > 0.0f && bIsVisible)
    {
        // Calculate the end point of the line based on angle and length
        // Convert angle from degrees to radians
        // Note: 0° = right, positive = counter-clockwise (standard math convention)
        float AngleRadians = FMath::DegreesToRadians(CurrentDirectionAngle);
        
        // Calculate direction vector
        // The angle is in standard math coordinates (0° = right, 90° = up)
        // But Unreal's UI uses: X increases right, Y increases DOWN
        // Buttons are positioned with: Y = Center - (Radius * Sin(angle))
        // So if angle is 90° (up), Sin(90°) = 1, so Y = Center - Radius (moves up)
        // For the line to match, we need: when angle is 90°, line should point up
        // Direction = (Cos, -Sin) gives: (0, -1) which is up in UI (Y decreases)
        // But if controls are backwards, try: (Cos, Sin) without negation
        FVector2D Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians)); // Try without -Sin
        FVector2D EndPoint = Center + (Direction * CurrentDirectionLength);
        
        // Calculate color intensity based on input magnitude (0.3 to 1.0 range)
        // Map to 0.5 to 1.0 for alpha, and use brighter colors for stronger input
        float Intensity = FMath::Clamp(CurrentInputMagnitude, 0.3f, 1.0f);
        float NormalizedIntensity = (Intensity - 0.3f) / 0.7f; // Normalize to 0-1
        
        // Base color: use exposed property, but adjust brightness based on input intensity
        FLinearColor BaseColor = DirectionLineBaseColor;
        // Make color brighter with input intensity
        BaseColor.R = FMath::Clamp(BaseColor.R * (0.5f + NormalizedIntensity * 0.5f), 0.0f, 1.0f);
        BaseColor.G = FMath::Clamp(BaseColor.G * (0.5f + NormalizedIntensity * 0.5f), 0.0f, 1.0f);
        BaseColor.B = FMath::Clamp(BaseColor.B * (0.5f + NormalizedIntensity * 0.5f), 0.0f, 1.0f);
        BaseColor.A = 1.0f; // Full opacity for main line
        
        // Glow color (softer, more transparent)
        FLinearColor GlowColor = FLinearColor(
            BaseColor.R * 0.7f,
            BaseColor.G * 0.7f,
            BaseColor.B * 0.7f,
            DirectionLineGlowOpacity * NormalizedIntensity // Glow opacity based on intensity
        );
        
        // Draw glow/shadow effect (thicker, semi-transparent line behind)
        TArray<FVector2D> GlowLinePoints;
        GlowLinePoints.Add(Center);
        GlowLinePoints.Add(EndPoint);
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 4, // Draw behind main line
            AllottedGeometry.ToPaintGeometry(),
            GlowLinePoints,
            ESlateDrawEffect::None,
            GlowColor,
            true,  // bAntialias for smooth glow
            DirectionLineGlowThickness   // Use exposed property
        );
        
        // Draw gradient line using multiple segments for smooth fade
        // Create segments from center to end with decreasing opacity
        const int32 GradientSegments = DirectionLineGradientSegments;
        for (int32 i = 0; i < GradientSegments; ++i)
        {
            float SegmentStart = (float)i / GradientSegments;
            float SegmentEnd = (float)(i + 1) / GradientSegments;
            
            // Calculate opacity: full at center, fade based on DirectionLineEndFadeAmount
            float StartOpacity = 1.0f - (SegmentStart * DirectionLineEndFadeAmount);
            float EndOpacity = 1.0f - (SegmentEnd * DirectionLineEndFadeAmount);
            
            FVector2D SegmentStartPoint = Center + (Direction * CurrentDirectionLength * SegmentStart);
            FVector2D SegmentEndPoint = Center + (Direction * CurrentDirectionLength * SegmentEnd);
            
            TArray<FVector2D> SegmentPoints;
            SegmentPoints.Add(SegmentStartPoint);
            SegmentPoints.Add(SegmentEndPoint);
            
            FLinearColor SegmentColor = BaseColor;
            SegmentColor.A = StartOpacity * NormalizedIntensity; // Apply intensity to opacity
            
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId + 5, // Draw on top of glow
                AllottedGeometry.ToPaintGeometry(),
                SegmentPoints,
                ESlateDrawEffect::None,
                SegmentColor,
                true, // bAntialias for smooth lines
                DirectionLineThickness  // Use exposed property
            );
        }
        
        // Draw arrowhead at the end
        const float ArrowheadLength = DirectionLineArrowheadLength; // Use exposed property
        const float ArrowheadAngle = FMath::DegreesToRadians(DirectionLineArrowheadAngle); // Use exposed property
        
        // Calculate arrowhead points (two lines forming a V at the end)
        FVector2D ArrowBase = EndPoint - (Direction * ArrowheadLength * 0.3f); // Slightly back from tip
        
        // Perpendicular direction for arrowhead spread
        FVector2D Perpendicular(-Direction.Y, Direction.X);
        
        // Arrowhead tip points
        FVector2D ArrowLeft = EndPoint - (Direction * ArrowheadLength * FMath::Cos(ArrowheadAngle)) 
                              + (Perpendicular * ArrowheadLength * FMath::Sin(ArrowheadAngle));
        FVector2D ArrowRight = EndPoint - (Direction * ArrowheadLength * FMath::Cos(ArrowheadAngle))
                               - (Perpendicular * ArrowheadLength * FMath::Sin(ArrowheadAngle));
        
        // Draw arrowhead lines (bright, full opacity)
        TArray<FVector2D> ArrowLeftLine;
        ArrowLeftLine.Add(EndPoint);
        ArrowLeftLine.Add(ArrowLeft);
        
        TArray<FVector2D> ArrowRightLine;
        ArrowRightLine.Add(EndPoint);
        ArrowRightLine.Add(ArrowRight);
        
        FLinearColor ArrowColor = BaseColor;
        ArrowColor.A = 1.0f; // Full opacity for arrowhead
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 6, // Draw on top of main line
            AllottedGeometry.ToPaintGeometry(),
            ArrowLeftLine,
            ESlateDrawEffect::None,
            ArrowColor,
            true, // bAntialias
            DirectionLineArrowheadThickness   // Use exposed property
        );
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 6,
            AllottedGeometry.ToPaintGeometry(),
            ArrowRightLine,
            ESlateDrawEffect::None,
            ArrowColor,
            true, // bAntialias
            DirectionLineArrowheadThickness   // Use exposed property
        );
        
        // Draw a bright dot at the center for emphasis
        const float CenterDotRadius = DirectionLineCenterDotRadius; // Use exposed property
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 7, // Draw on top of everything
            AllottedGeometry.ToPaintGeometry(
                FVector2D(CenterDotRadius * 2.0f, CenterDotRadius * 2.0f),
                FSlateLayoutTransform(Center - FVector2D(CenterDotRadius, CenterDotRadius))
            ),
            FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            BaseColor // Use same color as line
        );
    }
    
    // Draw joystick angle and direction text on screen (only if debug is enabled)
    if (bShowDebugText && bIsVisible && bShouldDrawDirectionLine && CurrentDirectionLength > 0.0f)
    {
        // Get default font
        const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 16);
        const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        
        // Format the text with raw stick values, calculated direction, and selected button
        float DirectionX = FMath::Cos(FMath::DegreesToRadians(CurrentDirectionAngle));
        float DirectionY = FMath::Sin(FMath::DegreesToRadians(CurrentDirectionAngle));
        
        // Get selected button info - use cached values to avoid accessing during GC
        FString SelectedButtonInfo = TEXT("None");
        if (CachedSelectedIndex >= 0 && CachedSelectedIndex < NumButtons)
        {
            if (CachedSelectedIndex >= 0 && CachedSelectedIndex < NumMenuItems)
            {
                // Cache the label string to avoid accessing struct during GC
                FString CachedLabel = MenuItems[CachedSelectedIndex].Label.ToString();
                SelectedButtonInfo = FString::Printf(TEXT("Button [%d]: %s (%.0f°)"), 
                    CachedSelectedIndex,
                    *CachedLabel,
                    GetMenuItemAngle(CachedSelectedIndex));
            }
            else
            {
                SelectedButtonInfo = FString::Printf(TEXT("Button [%d] (%.0f°)"), 
                    CachedSelectedIndex,
                    GetMenuItemAngle(CachedSelectedIndex));
            }
        }
        
        FString DebugText = FString::Printf(TEXT("Stick Raw: X=%.2f, Y=%.2f\nStick Angle: %.1f°\nDirection: X=%.2f, Y=%.2f\nSelected: %s"), 
            CurrentStickX, CurrentStickY,
            CurrentDirectionAngle,
            DirectionX, DirectionY,
            *SelectedButtonInfo);
        
        // Calculate text position (top-left of widget, offset a bit)
        FVector2D TextPosition(20.0f, 20.0f);
        
        // Measure text size
        FVector2D TextSize = FontMeasure->Measure(DebugText, FontInfo);
        
        // Draw text background (semi-transparent black rectangle)
        FVector2D BackgroundPosition = TextPosition - FVector2D(5.0f, 5.0f);
        FVector2D BackgroundSize = FVector2D(TextSize.X + 10.0f, TextSize.Y + 10.0f);
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 6,
            AllottedGeometry.ToPaintGeometry(BackgroundSize, FSlateLayoutTransform(BackgroundPosition)),
            FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            FLinearColor(0.0f, 0.0f, 0.0f, 0.7f) // Black with 70% opacity
        );
        
        // Draw the text
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 7,
            AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(TextPosition)),
            DebugText,
            FontInfo,
            ESlateDrawEffect::None,
            FLinearColor::White
        );
    }
    
    return LayerId;
}

void UPURadialMenu::UpdateDirectionLine(float AngleDegrees, float InputMagnitude)
{
    // Calculate line length based on input magnitude
    float LineLength = ItemRadius * FMath::Clamp(InputMagnitude, 0.3f, 1.0f);
    
    // Update the mutable state for NativePaint
    CurrentDirectionAngle = AngleDegrees;
    CurrentDirectionLength = LineLength;
    CurrentInputMagnitude = InputMagnitude; // Store for color intensity
    bShouldDrawDirectionLine = true;
    
    // Invalidate the widget to trigger a repaint
    if (IsValid(this))
    {
        Invalidate(EInvalidateWidget::Paint);
    }
    
    // If no input, hide the line
    if (InputMagnitude <= 0.0f)
    {
        bShouldDrawDirectionLine = false;
        CurrentDirectionLength = 0.0f;
        CurrentInputMagnitude = 0.0f;
        if (IsValid(this))
        {
            Invalidate(EInvalidateWidget::Paint);
        }
    }
}

void UPURadialMenu::PreviewRadialLayout()
{
    if (!MenuItemsContainer)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPURadialMenu::PreviewRadialLayout - MenuItemsContainer not set"));
        return;
    }

    // Get all child widgets from the container
    int32 ChildCount = MenuItemsContainer->GetChildrenCount();
    if (ChildCount == 0)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::PreviewRadialLayout - No children to arrange"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::PreviewRadialLayout - Arranging %d children in radial layout"), ChildCount);

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
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPURadialMenu::PreviewRadialLayout - Container size is zero, using default 400x400. Make sure container has a size set in Blueprint!"));
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
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::PreviewRadialLayout - Container size: (%.2f, %.2f), Center: (%.2f, %.2f)"), 
    //    ContainerSize.X, ContainerSize.Y, CenterPosition.X, CenterPosition.Y);

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
        // This gives us standard math angles: 0° = right, 90° = up, 180° = left, 270° = down
        float AngleDegrees = (-90.0f + (AngleStep * i));
        float AngleRadians = FMath::DegreesToRadians(AngleDegrees);

        // Calculate position using trigonometry
        // In Unreal's UI: X increases right, Y increases down
        // To align buttons with stick input (which uses Atan2(-Y, X)), we need to flip the Y axis
        // Use +Sin instead of -Sin to flip 180° so buttons match stick direction
        float X = CenterPosition.X + (ItemRadius * FMath::Cos(AngleRadians));
        float Y = CenterPosition.Y + (ItemRadius * FMath::Sin(AngleRadians)); // Use +Sin to flip orientation

        // Position the widget
        if (UCanvasPanelSlot* WidgetSlot = Cast<UCanvasPanelSlot>(ChildWidget->Slot))
        {
            WidgetSlot->SetPosition(FVector2D(X - (MenuItemSize.X * 0.5f), Y - (MenuItemSize.Y * 0.5f)));
            WidgetSlot->SetSize(MenuItemSize);
            WidgetSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            WidgetSlot->SetAlignment(FVector2D(0.5f, 0.5f));

            //UE_LOG(LogTemp,Display, TEXT("🎯   Positioned child %d at angle %.2f° at position (%.2f, %.2f)"), 
            //    i, AngleDegrees, X, Y);
        }
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::PreviewRadialLayout - Arranged %d widgets in radial layout"), ChildCount);
}

void UPURadialMenu::SetPreparationDataTable(UDataTable* InDataTable)
{
    PreparationDataTable = InDataTable;
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPURadialMenu::SetPreparationDataTable - Preparation data table set: %s"), 
    //    InDataTable ? *InDataTable->GetName() : TEXT("NULL"));
}

