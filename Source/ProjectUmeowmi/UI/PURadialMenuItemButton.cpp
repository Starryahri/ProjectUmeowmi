#include "PURadialMenuItemButton.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

UPURadialMenuItemButton::UPURadialMenuItemButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ItemIndex(-1)
    , ItemButton(nullptr)
{
}

void UPURadialMenuItemButton::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind to the button's OnClicked event if button exists
    if (ItemButton)
    {
        ItemButton->OnClicked.AddUniqueDynamic(this, &UPURadialMenuItemButton::HandleButtonClicked);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPURadialMenuItemButton::NativeConstruct - ItemButton not found! Make sure to bind it in Blueprint."));
    }
}

void UPURadialMenuItemButton::SetMenuItemData(const FRadialMenuItem& MenuItem, int32 InItemIndex)
{
    MenuItemData = MenuItem;
    ItemIndex = InItemIndex;

    // Set button enabled state if button exists
    if (ItemButton)
    {
        ItemButton->SetIsEnabled(MenuItem.bIsEnabled);
        // Fire click on mouse down to avoid needing a second click
        ItemButton->SetClickMethod(EButtonClickMethod::MouseDown);
        ItemButton->SetTouchMethod(EButtonTouchMethod::PreciseTap);
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenuItemButton::SetMenuItemData - Set data for item %d: %s (Enabled: %s)"),
        ItemIndex, *MenuItem.Label.ToString(), MenuItem.bIsEnabled ? TEXT("YES") : TEXT("NO"));

    // Call Blueprint event
    OnMenuItemDataSet(MenuItemData, ItemIndex);
}

void UPURadialMenuItemButton::HandleButtonClicked()
{
    if (ItemIndex >= 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPURadialMenuItemButton::HandleButtonClicked - Button clicked for item %d"), ItemIndex);
        
        // Broadcast the event with the index
        OnItemClicked.Broadcast(ItemIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPURadialMenuItemButton::HandleButtonClicked - Button clicked but ItemIndex is invalid: %d"), ItemIndex);
    }
}

