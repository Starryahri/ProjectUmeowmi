#include "PUIngredientSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "PUIngredientQuantityControl.h"
#include "PUIngredientDragDropOperation.h"
#include "Input/Events.h"
#include "../DishCustomization/PUPreparationBase.h"
#include "Engine/DataTable.h"

UPUIngredientSlot::UPUIngredientSlot(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Location(EPUIngredientSlotLocation::ActiveIngredientArea)
    , bHasIngredient(false)
    , QuantityControlWidget(nullptr)
    , RadialMenuWidget(nullptr)
    , bRadialMenuVisible(false)
{
}

void UPUIngredientSlot::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeConstruct - Slot widget constructed: %s"), *GetName());

    // Hide hover text by default
    if (HoverText)
    {
        HoverText->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Update display on construction
    UpdateDisplay();
}

void UPUIngredientSlot::NativeDestruct()
{
    // Clean up quantity control widget
    if (QuantityControlWidget)
    {
        if (QuantityControlContainer)
        {
            QuantityControlContainer->RemoveChild(QuantityControlWidget);
        }
        QuantityControlWidget = nullptr;
    }

    // Clean up radial menu widget (stubbed)
    if (RadialMenuWidget)
    {
        RadialMenuWidget = nullptr;
    }

    Super::NativeDestruct();
}

void UPUIngredientSlot::SetIngredientInstance(const FIngredientInstance& InIngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Setting ingredient instance (Slot: %s)"), *GetName());
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Input Instance ID: %d, Qty: %d, Preparations: %d"),
        InIngredientInstance.InstanceID, InIngredientInstance.Quantity, InIngredientInstance.Preparations.Num());

    // Store the instance data
    IngredientInstance = InIngredientInstance;
    bHasIngredient = true;

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Stored Ingredient: %s (ID: %d, Qty: %d, Preparations: %d)"),
        *IngredientInstance.IngredientData.DisplayName.ToString(),
        IngredientInstance.InstanceID,
        IngredientInstance.Quantity,
        IngredientInstance.Preparations.Num());

    // Update all display elements
    UpdateDisplay();

    // Call Blueprint event
    OnIngredientInstanceSet(IngredientInstance);

    // Broadcast change event
    OnSlotIngredientChanged.Broadcast(IngredientInstance);
}

void UPUIngredientSlot::ClearSlot()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ClearSlot - Clearing slot: %s"), *GetName());

    bHasIngredient = false;
    IngredientInstance = FIngredientInstance(); // Reset to default

    // Update all display elements
    UpdateDisplay();

    // Call Blueprint event
    OnSlotEmptied();
}

void UPUIngredientSlot::SetLocation(EPUIngredientSlotLocation InLocation)
{
    if (Location != InLocation)
    {
        Location = InLocation;
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetLocation - Location changed to: %d"), (int32)Location);

        // Update display when location changes (texture may change)
        UpdateDisplay();

        // Call Blueprint event
        OnLocationChanged(Location);
    }
}

void UPUIngredientSlot::UpdateDisplay()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateDisplay - Updating display (Slot: %s, Empty: %s)"),
        *GetName(), IsEmpty() ? TEXT("TRUE") : TEXT("FALSE"));

    if (IsEmpty())
    {
        ClearDisplay();
    }
    else
    {
        UpdateIngredientIcon();
        UpdatePrepIcons();
        UpdateQuantityControl();
    }
}

void UPUIngredientSlot::UpdateIngredientIcon()
{
    if (!IngredientIcon)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateIngredientIcon - IngredientIcon component not found"));
        return;
    }

    if (!bHasIngredient)
    {
        IngredientIcon->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    UTexture2D* TextureToUse = GetTextureForLocation();
    if (TextureToUse)
    {
        IngredientIcon->SetBrushFromTexture(TextureToUse);
        IngredientIcon->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateIngredientIcon - Set texture for location: %d"),
            (int32)Location);
    }
    else
    {
        IngredientIcon->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateIngredientIcon - No texture found for ingredient: %s"),
            *IngredientInstance.IngredientData.DisplayName.ToString());
    }
}

void UPUIngredientSlot::UpdatePrepIcons()
{
    if (!bHasIngredient)
    {
        // Hide all prep icons
        if (PrepIcon1) PrepIcon1->SetVisibility(ESlateVisibility::Collapsed);
        if (PrepIcon2) PrepIcon2->SetVisibility(ESlateVisibility::Collapsed);
        if (SuspiciousIcon) SuspiciousIcon->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    int32 PrepCount = IngredientInstance.Preparations.Num();

    if (PrepCount >= 3)
    {
        // Show suspicious icon, hide individual prep icons
        if (PrepIcon1) PrepIcon1->SetVisibility(ESlateVisibility::Collapsed);
        if (PrepIcon2) PrepIcon2->SetVisibility(ESlateVisibility::Collapsed);
        if (SuspiciousIcon)
        {
            SuspiciousIcon->SetVisibility(ESlateVisibility::Visible);
            // TODO: Set suspicious icon texture when available
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepIcons - Showing suspicious icon (3+ preps)"));
        }
    }
    else if (PrepCount > 0)
    {
        // Show up to 2 prep icons
        if (SuspiciousIcon) SuspiciousIcon->SetVisibility(ESlateVisibility::Collapsed);

        // Show first prep icon
        if (PrepIcon1)
        {
            PrepIcon1->SetVisibility(ESlateVisibility::Visible);
            // TODO: Set prep icon texture based on first preparation tag
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepIcons - Showing prep icon 1"));
        }

        // Show second prep icon if available
        if (PrepCount >= 2 && PrepIcon2)
        {
            PrepIcon2->SetVisibility(ESlateVisibility::Visible);
            // TODO: Set prep icon texture based on second preparation tag
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepIcons - Showing prep icon 2"));
        }
        else if (PrepIcon2)
        {
            PrepIcon2->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    else
    {
        // No preparations, hide all icons
        if (PrepIcon1) PrepIcon1->SetVisibility(ESlateVisibility::Collapsed);
        if (PrepIcon2) PrepIcon2->SetVisibility(ESlateVisibility::Collapsed);
        if (SuspiciousIcon) SuspiciousIcon->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UPUIngredientSlot::UpdateQuantityControl()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - START (HasIngredient: %s, Widget: %s, Class: %s, Container: %s)"),
        bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"),
        QuantityControlWidget ? TEXT("EXISTS") : TEXT("NULL"),
        QuantityControlClass ? TEXT("SET") : TEXT("NULL"),
        QuantityControlContainer ? TEXT("SET") : TEXT("NULL"));

    if (!bHasIngredient)
    {
        // Unbind events before removing
        if (QuantityControlWidget && bQuantityControlEventsBound)
        {
            QuantityControlWidget->OnQuantityControlChanged.RemoveDynamic(this, &UPUIngredientSlot::OnQuantityControlChanged);
            QuantityControlWidget->OnQuantityControlRemoved.RemoveDynamic(this, &UPUIngredientSlot::OnQuantityControlRemoved);
            bQuantityControlEventsBound = false;
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Events unbound from quantity control"));
        }

        // Remove quantity control if slot is empty
        if (QuantityControlWidget)
        {
            if (QuantityControlContainer)
            {
                QuantityControlContainer->RemoveChild(QuantityControlWidget);
            }
            QuantityControlWidget->RemoveFromParent();
            QuantityControlWidget = nullptr;
        }
        return;
    }

    if (!QuantityControlContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - QuantityControlContainer is not set! Cannot add quantity control."));
        return;
    }

    // First, check if there's already a quantity control widget in the container (placed in Blueprint)
    if (!QuantityControlWidget)
    {
        // Look for existing quantity control widget in the container
        for (int32 i = 0; i < QuantityControlContainer->GetChildrenCount(); i++)
        {
            if (UPUIngredientQuantityControl* ExistingWidget = Cast<UPUIngredientQuantityControl>(QuantityControlContainer->GetChildAt(i)))
            {
                QuantityControlWidget = ExistingWidget;
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Found existing quantity control widget in container"));
                break;
            }
        }
    }

    // If still no widget, try to create one (if class is set)
    if (!QuantityControlWidget && QuantityControlClass)
    {
        QuantityControlWidget = CreateWidget<UPUIngredientQuantityControl>(GetWorld(), QuantityControlClass);
        if (QuantityControlWidget)
        {
            QuantityControlContainer->AddChild(QuantityControlWidget);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Created quantity control widget dynamically"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - Failed to create quantity control widget"));
        }
    }
    else if (!QuantityControlWidget && !QuantityControlClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - No quantity control widget found and QuantityControlClass is not set!"));
        UE_LOG(LogTemp, Warning, TEXT("⚠️   Either place a WBP_IngredientQuantityControl widget in the QuantityControlContainer in Blueprint,"));
        UE_LOG(LogTemp, Warning, TEXT("⚠️   OR set the QuantityControlClass property in the slot widget's defaults."));
        return;
    }

    // Bind to quantity control events (only once!)
    if (QuantityControlWidget && !bQuantityControlEventsBound)
    {
        QuantityControlWidget->OnQuantityControlChanged.AddDynamic(this, &UPUIngredientSlot::OnQuantityControlChanged);
        QuantityControlWidget->OnQuantityControlRemoved.AddDynamic(this, &UPUIngredientSlot::OnQuantityControlRemoved);
        bQuantityControlEventsBound = true;
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Events bound to quantity control"));
    }

    // Update quantity control with current ingredient instance
    if (QuantityControlWidget)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Setting ingredient instance on quantity control"));
        UE_LOG(LogTemp, Display, TEXT("🎯   Slot Instance - ID: %d, Qty: %d, Ingredient: %s, Preparations: %d"),
            IngredientInstance.InstanceID,
            IngredientInstance.Quantity,
            *IngredientInstance.IngredientData.DisplayName.ToString(),
            IngredientInstance.Preparations.Num());
        
        // Verify the instance has a valid ID before setting
        if (IngredientInstance.InstanceID == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - WARNING: IngredientInstance has InstanceID = 0! This might be a problem."));
        }
        
        QuantityControlWidget->SetIngredientInstance(IngredientInstance);
        QuantityControlWidget->SetVisibility(ESlateVisibility::Visible);
        
        // Verify it was set correctly
        const FIngredientInstance& SetInstance = QuantityControlWidget->GetIngredientInstance();
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - After setting, Quantity Control has Instance ID: %d, Qty: %d"),
            SetInstance.InstanceID, SetInstance.Quantity);
    }
}

void UPUIngredientSlot::ClearDisplay()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ClearDisplay - Clearing display"));

    // Hide icon
    if (IngredientIcon)
    {
        IngredientIcon->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Hide prep icons
    if (PrepIcon1) PrepIcon1->SetVisibility(ESlateVisibility::Collapsed);
    if (PrepIcon2) PrepIcon2->SetVisibility(ESlateVisibility::Collapsed);
    if (SuspiciousIcon) SuspiciousIcon->SetVisibility(ESlateVisibility::Collapsed);

    // Hide hover text
    if (HoverText)
    {
        HoverText->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Remove quantity control
    if (QuantityControlWidget)
    {
        if (QuantityControlContainer)
        {
            QuantityControlContainer->RemoveChild(QuantityControlWidget);
        }
        QuantityControlWidget->RemoveFromParent();
        QuantityControlWidget = nullptr;
    }
}

UTexture2D* UPUIngredientSlot::GetTextureForLocation() const
{
    if (!bHasIngredient)
    {
        return nullptr;
    }

    // TODO: When PantryTexture field is added to FPUIngredientBase, use it here
    // For now, we'll use PreviewTexture for both locations as a placeholder
    if (Location == EPUIngredientSlotLocation::Pantry)
    {
        // TODO: Return IngredientInstance.IngredientData.PantryTexture when available
        // For now, fallback to PreviewTexture
        return IngredientInstance.IngredientData.PreviewTexture;
    }
    else // ActiveIngredientArea
    {
        return IngredientInstance.IngredientData.PreviewTexture;
    }
}

bool UPUIngredientSlot::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UPUIngredientDragDropOperation* IngredientDragOp = Cast<UPUIngredientDragDropOperation>(InOperation);
    if (IngredientDragOp)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDragOver - Drag over slot: %s"),
            *GetName());

        // Call Blueprint event for visual feedback
        OnDragOverSlot();

        return true;
    }

    return false;
}

bool UPUIngredientSlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UPUIngredientDragDropOperation* IngredientDragOp = Cast<UPUIngredientDragDropOperation>(InOperation);
    if (IngredientDragOp)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Drop on slot: %s (Ingredient: %s)"),
            *GetName(), *IngredientDragOp->IngredientData.DisplayName.ToString());

        // Create ingredient instance from drag operation
        FIngredientInstance NewInstance;
        NewInstance.InstanceID = IngredientDragOp->InstanceID;
        NewInstance.Quantity = IngredientDragOp->Quantity;
        NewInstance.IngredientData = IngredientDragOp->IngredientData;
        NewInstance.IngredientTag = IngredientDragOp->IngredientTag;
        // Sync Preparations with ActivePreparations from IngredientData
        NewInstance.Preparations = IngredientDragOp->IngredientData.ActivePreparations;

        // Set the ingredient instance
        SetIngredientInstance(NewInstance);

        // Broadcast drop event
        OnIngredientDroppedOnSlot.Broadcast(this);

        return true;
    }

    return false;
}

void UPUIngredientSlot::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UPUIngredientDragDropOperation* IngredientDragOp = Cast<UPUIngredientDragDropOperation>(InOperation);
    if (IngredientDragOp)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDragEnter - Drag entered slot: %s"),
            *GetName());
        OnDragOverSlot();
    }
}

void UPUIngredientSlot::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UPUIngredientDragDropOperation* IngredientDragOp = Cast<UPUIngredientDragDropOperation>(InOperation);
    if (IngredientDragOp)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDragLeave - Drag left slot: %s"),
            *GetName());
        OnDragLeaveSlot();
    }
}

FReply UPUIngredientSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Mouse button down on slot: %s"),
        *GetName());

    if (IsEmpty())
    {
        // Empty slot clicked - open pantry
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Empty slot clicked, opening pantry"));
        OnEmptySlotClicked.Broadcast(this);
        return FReply::Handled();
    }

    // Slot has ingredient - handle left/right click
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Left click - show prep radial menu
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Left click, showing prep radial menu"));
        ShowRadialMenu(true);
        return FReply::Handled();
    }
    else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        // Right click - show actions radial menu
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Right click, showing actions radial menu"));
        ShowRadialMenu(false);
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

void UPUIngredientSlot::ShowRadialMenu(bool bIsPrepMenu)
{
    if (IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - Cannot show menu on empty slot"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Showing radial menu (Prep: %s)"),
        bIsPrepMenu ? TEXT("TRUE") : TEXT("FALSE"));

    // TODO: Implement radial menu creation and display
    // For now, this is stubbed
    if (RadialMenuWidgetClass)
    {
        if (!RadialMenuWidget)
        {
            RadialMenuWidget = CreateWidget<UUserWidget>(GetWorld(), RadialMenuWidgetClass);
            if (RadialMenuWidget)
            {
                // Add to viewport or parent widget
                // TODO: Determine proper parent/positioning
                RadialMenuWidget->AddToViewport();
                bRadialMenuVisible = true;
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Radial menu created (STUBBED)"));
            }
        }
        else
        {
            RadialMenuWidget->SetVisibility(ESlateVisibility::Visible);
            bRadialMenuVisible = true;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - RadialMenuWidgetClass not set"));
    }
}

void UPUIngredientSlot::HideRadialMenu()
{
    if (RadialMenuWidget)
    {
        RadialMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
        bRadialMenuVisible = false;
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::HideRadialMenu - Radial menu hidden"));
    }
}

void UPUIngredientSlot::OnQuantityControlChanged(const FIngredientInstance& InIngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::OnQuantityControlChanged - Quantity control changed (ID: %d, Qty: %d)"),
        InIngredientInstance.InstanceID, InIngredientInstance.Quantity);

    // Update instance if IDs match
    if (bHasIngredient && IngredientInstance.InstanceID == InIngredientInstance.InstanceID)
    {
        IngredientInstance = InIngredientInstance;

        // DON'T call UpdateDisplay() here - it would call UpdateQuantityControl() again, causing recursion!
        // Only update the parts that need updating (icon, prep icons, etc.)
        // The quantity control already updated itself, so we don't need to update it again
        UpdateIngredientIcon();
        UpdatePrepIcons();

        // Broadcast change
        OnSlotIngredientChanged.Broadcast(IngredientInstance);
    }
}

void UPUIngredientSlot::OnQuantityControlRemoved(int32 InstanceID, UPUIngredientQuantityControl* InQuantityControlWidget)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::OnQuantityControlRemoved - Quantity control removed (ID: %d)"),
        InstanceID);

    // Clear the slot
    ClearSlot();
}

void UPUIngredientSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseEnter - Mouse entered slot: %s"),
        *GetName());

    // Show hover text if slot has ingredient
    if (bHasIngredient)
    {
        UpdateHoverTextVisibility(true);
    }
}

void UPUIngredientSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseLeave - Mouse left slot: %s"),
        *GetName());

    // Hide hover text
    UpdateHoverTextVisibility(false);
}

void UPUIngredientSlot::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
    Super::NativeOnAddedToFocusPath(InFocusEvent);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnAddedToFocusPath - Focus added to slot: %s"),
        *GetName());

    // Show hover text if slot has ingredient
    if (bHasIngredient)
    {
        UpdateHoverTextVisibility(true);
    }
}

void UPUIngredientSlot::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
    Super::NativeOnRemovedFromFocusPath(InFocusEvent);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnRemovedFromFocusPath - Focus removed from slot: %s"),
        *GetName());

    // Hide hover text
    UpdateHoverTextVisibility(false);
}

FText UPUIngredientSlot::GetIngredientDisplayText() const
{
    if (!bHasIngredient)
    {
        return FText::GetEmpty();
    }

    // Debug: Log what preparations we have
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetIngredientDisplayText - START"));
    UE_LOG(LogTemp, Display, TEXT("🎯   Ingredient: %s"), *IngredientInstance.IngredientData.DisplayName.ToString());
    UE_LOG(LogTemp, Display, TEXT("🎯   Preparations count: %d"), IngredientInstance.Preparations.Num());
    
    TArray<FGameplayTag> PrepTags;
    IngredientInstance.Preparations.GetGameplayTagArray(PrepTags);
    for (const FGameplayTag& PrepTag : PrepTags)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯   Preparation tag: %s"), *PrepTag.ToString());
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯   PreparationDataTable set: %s"), PreparationDataTable ? TEXT("YES") : TEXT("NO"));

    // Get preparation data table from slot property
    FPUIngredientBase IngredientDataCopy = IngredientInstance.IngredientData;
    
    // If we have a preparation data table set on the slot, use it
    if (PreparationDataTable)
    {
        // Set the preparation data table on the ingredient data copy
        IngredientDataCopy.PreparationDataTable = PreparationDataTable;
        UE_LOG(LogTemp, Display, TEXT("🎯   Set PreparationDataTable on ingredient data copy"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️   PreparationDataTable not set on slot!"));
    }

    // Sync Preparations with ActivePreparations before calling GetCurrentDisplayName()
    // GetCurrentDisplayName() uses ActivePreparations to look up preparations from the data table
    IngredientDataCopy.ActivePreparations = IngredientInstance.Preparations;
    
    UE_LOG(LogTemp, Display, TEXT("🎯   After sync, ActivePreparations count: %d"), IngredientDataCopy.ActivePreparations.Num());
    
    // Use the ingredient's GetCurrentDisplayName() which already handles preparations correctly
    // This method looks up preparations from the data table and uses NamePrefix properly
    // It formats as "PrepName IngredientName" for single prep, combines prefixes for multiple preps,
    // and uses "Dubious IngredientName" for 3+ preparations
    FText Result = IngredientDataCopy.GetCurrentDisplayName();
    
    UE_LOG(LogTemp, Display, TEXT("🎯   GetCurrentDisplayName() returned: %s"), *Result.ToString());
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetIngredientDisplayText - END"));
    
    return Result;
}

void UPUIngredientSlot::UpdateHoverTextVisibility(bool bShow)
{
    if (!HoverText)
    {
        return;
    }

    if (bShow && bHasIngredient)
    {
        // Update text content
        FText DisplayText = GetIngredientDisplayText();
        HoverText->SetText(DisplayText);
        HoverText->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateHoverTextVisibility - Showing hover text: %s"),
            *DisplayText.ToString());
    }
    else
    {
        HoverText->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateHoverTextVisibility - Hiding hover text"));
    }
}

