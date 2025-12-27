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
#include "PUDishCustomizationWidget.h"
#include "PUCookingStageWidget.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SlateWrapperTypes.h"
#include "GameplayTagContainer.h"
#include "PURadialMenu.h"
#include "../DishCustomization/PUDishBlueprintLibrary.h"

UPUIngredientSlot::UPUIngredientSlot(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Location(EPUIngredientSlotLocation::ActiveIngredientArea)
    , bHasIngredient(false)
    , QuantityControlWidget(nullptr)
    , RadialMenuWidget(nullptr)
    , bRadialMenuVisible(false)
    , bDragEnabled(true)  // Enable drag by default for testing
{
}

void UPUIngredientSlot::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeConstruct - Slot widget constructed: %s"), *GetName());

    // Try to find and cache the dish widget if not already set
    if (!CachedDishWidget.IsValid())
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::NativeConstruct - Cached dish widget not set, attempting to find it"));
        UPUDishCustomizationWidget* FoundWidget = GetDishCustomizationWidget();
        if (FoundWidget)
        {
            UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::NativeConstruct - Found and cached dish widget: %s"), *FoundWidget->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::NativeConstruct - Could not find dish widget during construction"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::NativeConstruct - Cached dish widget already set: %s"), 
            *CachedDishWidget.Get()->GetName());
    }

    // Hide hover text by default
    if (HoverText)
    {
        HoverText->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Initialize plate background (always 100% opacity, outline hidden by default)
    UpdatePlateBackgroundOpacity();

    // Hide QuantityText in prep and cooking stages (shown in plating)
    if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea)
    {
        if (QuantityText)
        {
            QuantityText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    // Hide PreparationText in prep, cooking, and plating stages
    if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Plating)
    {
        if (PreparationText)
        {
            PreparationText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // If InitialIngredientInstance is set (from Blueprint widget creation), apply it
    // SetIngredientInstance will call UpdateDisplay() internally
    if (InitialIngredientInstance.IngredientData.IngredientTag.IsValid())
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeConstruct - Initial ingredient instance found, applying: %s"), 
            *InitialIngredientInstance.IngredientData.DisplayName.ToString());
        SetIngredientInstance(InitialIngredientInstance);
    }
    else
    {
        // Update display on construction (only if no initial instance was set)
    UpdateDisplay();
    }
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
    // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Setting ingredient instance (Slot: %s)"), *GetName());
    // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Input Instance ID: %d, Qty: %d, Preparations: %d"),
    //     InIngredientInstance.InstanceID, InIngredientInstance.Quantity, InIngredientInstance.Preparations.Num());

    // Store the instance data
    IngredientInstance = InIngredientInstance;
    
    // IMPORTANT: Sync ActivePreparations FROM IngredientData TO Preparations
    // If the ingredient data table row has ActivePreparations set (like Prep.Char in bbqduck),
    // copy them to the instance's Preparations field so they're displayed
    if (IngredientInstance.Preparations.Num() == 0 && IngredientInstance.IngredientData.ActivePreparations.Num() > 0)
    {
        IngredientInstance.Preparations = IngredientInstance.IngredientData.ActivePreparations;
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Synced %d ActivePreparations from IngredientData to Preparations"), 
            IngredientInstance.IngredientData.ActivePreparations.Num());
    }
    
    // Also sync the other way for GetCurrentDisplayName() to work
    // This ensures GetCurrentDisplayName() can look up preparations from the data table
    IngredientInstance.IngredientData.ActivePreparations = IngredientInstance.Preparations;
    // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Final: Preparations=%d, ActivePreparations=%d"), 
    //     IngredientInstance.Preparations.Num(), IngredientInstance.IngredientData.ActivePreparations.Num());
    
    // Set plating-specific properties
    MaxQuantity = InIngredientInstance.Quantity;
    RemainingQuantity = MaxQuantity;
    
    // If quantity is 0, treat as empty (but still store ingredient data for pantry texture display)
    // Also check if InstanceID is 0 - if so, this is likely a pantry slot placeholder
    if (InIngredientInstance.Quantity <= 0 || InIngredientInstance.InstanceID == 0)
    {
        bHasIngredient = false;
        // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Quantity is 0 or InstanceID is 0, treating as empty (but storing ingredient data for display)"));
    }
    else
    {
        bHasIngredient = true;
    }

    // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Stored Ingredient: %s (ID: %d, Qty: %d, Preparations: %d, HasIngredient: %s)"),
    //     *IngredientInstance.IngredientData.DisplayName.ToString(),
    //     IngredientInstance.InstanceID,
    //     IngredientInstance.Quantity,
    //     IngredientInstance.Preparations.Num(),
    //     bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"));

    // Update all display elements
    UpdateDisplay();

    // Call Blueprint event (only if we actually have an ingredient)
    if (bHasIngredient)
    {
        OnIngredientInstanceSet(IngredientInstance);
    }

    // Broadcast change event (only if we actually have an ingredient)
    if (bHasIngredient)
    {
        OnSlotIngredientChanged.Broadcast(IngredientInstance);
    }
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
        // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetLocation - Location changed to: %d"), (int32)Location);

        // Immediately hide QuantityText if in prep or cooking stage (shown in plating)
    // Hide QuantityText in prep and cooking stages (shown in plating)
    if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea)
        {
            if (QuantityText)
            {
                QuantityText->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
        if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Plating)
        {
            if (PreparationText)
            {
                PreparationText->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        // Update display when location changes (texture may change)
        UpdateDisplay();

        // Call Blueprint event
        OnLocationChanged(Location);
    }
}

void UPUIngredientSlot::UpdateDisplay()
{
    // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateDisplay - Updating display (Slot: %s, Empty: %s, Location: %d)"),
    //     *GetName(), IsEmpty() ? TEXT("TRUE") : TEXT("FALSE"), (int32)Location);

    // For pantry and prep slots, we want to show the texture even if "empty" (quantity 0)
    // For other locations, clear display if empty
    bool bShouldClear = IsEmpty();
    if (Location == EPUIngredientSlotLocation::Pantry || Location == EPUIngredientSlotLocation::Prep)
    {
        // Pantry/Prep slots: only clear if we don't have ingredient data at all
        bShouldClear = !IngredientInstance.IngredientData.IngredientTag.IsValid();
    }

    if (bShouldClear)
    {
        ClearDisplay();
    }
    else
    {
        UpdateIngredientIcon();
        UpdatePrepIcons();
        UpdateQuantityControl();
        
        // Update quantity and preparation text displays (they will hide themselves if in prep/cooking stage)
        UpdateQuantityDisplay();
        UpdatePreparationDisplay();
        
        // FORCE quantity control to be visible in cooking stage after UpdateDisplay
        if (Location == EPUIngredientSlotLocation::ActiveIngredientArea)
        {
            if (QuantityControlWidget)
            {
                QuantityControlWidget->SetVisibility(ESlateVisibility::Visible);
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateDisplay - FORCED QuantityControlWidget to Visible in cooking stage"));
            }
            if (QuantityControlContainer)
            {
                QuantityControlContainer->SetVisibility(ESlateVisibility::Visible);
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateDisplay - FORCED QuantityControlContainer to Visible in cooking stage"));
            }
        }
    }
}

void UPUIngredientSlot::UpdateIngredientIcon()
{
    if (!IngredientIcon)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateIngredientIcon - IngredientIcon component not found"));
        return;
    }

    // For pantry and prep slots, show texture even if empty (to display ingredient texture)
    // For active ingredient area slots, only show if we have an ingredient
    bool bShouldShowTexture = false;
    if (Location == EPUIngredientSlotLocation::Pantry || Location == EPUIngredientSlotLocation::Prep)
    {
        // Pantry/Prep slots: show texture if we have ingredient data (even if quantity is 0)
        bShouldShowTexture = IngredientInstance.IngredientData.IngredientTag.IsValid();
    }
    else // ActiveIngredientArea
    {
        // Active ingredient area: only show if we have an actual ingredient
        bShouldShowTexture = bHasIngredient;
    }

    if (!bShouldShowTexture)
    {
        IngredientIcon->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    UTexture2D* TextureToUse = GetTextureForLocation();
    if (TextureToUse)
    {
        IngredientIcon->SetBrushFromTexture(TextureToUse);
        IngredientIcon->SetVisibility(ESlateVisibility::Visible);
        
        // Grey out the icon if quantity is 0, but NOT for Pantry or Prep locations
        if (IngredientInstance.Quantity <= 0 && Location != EPUIngredientSlotLocation::Pantry && Location != EPUIngredientSlotLocation::Prep)
        {
            // Grey color: 0.5, 0.5, 0.5, 1.0
            IngredientIcon->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateIngredientIcon - Set texture and GREYED OUT (quantity is 0) for location: %d (Texture: %s)"),
                (int32)Location, *TextureToUse->GetName());
        }
        else
        {
            // Normal white color: 1.0, 1.0, 1.0, 1.0
            IngredientIcon->SetColorAndOpacity(FLinearColor::White);
        // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateIngredientIcon - Set texture for location: %d (Texture: %s)"),
        //     (int32)Location, *TextureToUse->GetName());
        }
    }
    else
    {
        IngredientIcon->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateIngredientIcon - No texture found for ingredient: %s (Location: %d)"),
            *IngredientInstance.IngredientData.DisplayName.ToString(), (int32)Location);
    }
}

UTexture2D* UPUIngredientSlot::GetPreparationTexture(const FGameplayTag& PreparationTag) const
{
    if (!PreparationDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::GetPreparationTexture - PreparationDataTable not set"));
        return nullptr;
    }

    // Get the preparation name from the tag (everything after the last period) and convert to lowercase
    FString PrepFullTag = PreparationTag.ToString();
    int32 PrepLastPeriodIndex;
    if (PrepFullTag.FindLastChar('.', PrepLastPeriodIndex))
    {
        FString PrepName = PrepFullTag.RightChop(PrepLastPeriodIndex + 1).ToLower();
        FName PrepRowName = FName(*PrepName);
        
        if (FPUPreparationBase* Preparation = PreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetPreparationTexture")))
        {
            if (Preparation->PreviewTexture)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetPreparationTexture - Found texture for preparation: %s"), *PrepName);
                return Preparation->PreviewTexture;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::GetPreparationTexture - Preparation %s has no PreviewTexture"), *PrepName);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::GetPreparationTexture - Preparation not found in data table: %s"), *PrepName);
        }
    }
    
    return nullptr;
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

    // Get preparation tags
    TArray<FGameplayTag> PrepTags;
    IngredientInstance.Preparations.GetGameplayTagArray(PrepTags);
    int32 PrepCount = PrepTags.Num();

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

        // Show first prep icon with texture
        if (PrepIcon1)
        {
            PrepIcon1->SetVisibility(ESlateVisibility::Visible);
            
            // Get texture for first preparation
            if (PrepTags.Num() > 0)
            {
                UTexture2D* PrepTexture = GetPreparationTexture(PrepTags[0]);
                if (PrepTexture)
                {
                    PrepIcon1->SetBrushFromTexture(PrepTexture);
                    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepIcons - Set prep icon 1 texture: %s"), *PrepTexture->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdatePrepIcons - No texture found for first preparation: %s"), *PrepTags[0].ToString());
                }
            }
        }

        // Show second prep icon if available
        if (PrepCount >= 2 && PrepIcon2)
        {
            PrepIcon2->SetVisibility(ESlateVisibility::Visible);
            
            // Get texture for second preparation
            if (PrepTags.Num() > 1)
            {
                UTexture2D* PrepTexture = GetPreparationTexture(PrepTags[1]);
                if (PrepTexture)
                {
                    PrepIcon2->SetBrushFromTexture(PrepTexture);
                    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepIcons - Set prep icon 2 texture: %s"), *PrepTexture->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdatePrepIcons - No texture found for second preparation: %s"), *PrepTags[1].ToString());
                }
            }
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
    // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - START (HasIngredient: %s, Widget: %s, Class: %s, Container: %s)"),
    //     bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"),
    //     QuantityControlWidget ? TEXT("EXISTS") : TEXT("NULL"),
    //     QuantityControlClass ? TEXT("SET") : TEXT("NULL"),
    //     QuantityControlContainer ? TEXT("SET") : TEXT("NULL"));

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
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - QuantityControlWidget EXISTS, Location: %d (ActiveIngredientArea=%d)"),
            (int32)Location, (int32)EPUIngredientSlotLocation::ActiveIngredientArea);
        
        // Hide quantity control in plating, prep, and pantry stages
        // Show it in cooking stage (ActiveIngredientArea)
        if (Location == EPUIngredientSlotLocation::Plating || 
            Location == EPUIngredientSlotLocation::Prep || 
            Location == EPUIngredientSlotLocation::Pantry)
        {
            QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
            if (QuantityControlContainer)
            {
                QuantityControlContainer->SetVisibility(ESlateVisibility::Collapsed);
            }
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Hiding quantity control and container (Location: %d)"), (int32)Location);
            return;
        }
        
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - SHOWING quantity control (Location: %d = ActiveIngredientArea)"), (int32)Location);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Setting ingredient instance on quantity control"));
        UE_LOG(LogTemp, Display, TEXT("🎯   Slot Instance - ID: %d, Qty: %d, Ingredient: %s, Preparations: %d, HasIngredient: %s"),
            IngredientInstance.InstanceID,
            IngredientInstance.Quantity,
            *IngredientInstance.IngredientData.DisplayName.ToString(),
            IngredientInstance.Preparations.Num(),
            bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"));
        
        // Verify the instance has a valid ID before setting
        if (IngredientInstance.InstanceID == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - WARNING: IngredientInstance has InstanceID = 0! This might be a problem."));
        }
        
        QuantityControlWidget->SetIngredientInstance(IngredientInstance);
        QuantityControlWidget->SetVisibility(ESlateVisibility::Visible);
        
        // Ensure the container is also visible
        if (QuantityControlContainer)
        {
            QuantityControlContainer->SetVisibility(ESlateVisibility::Visible);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Set QuantityControlContainer to Visible"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - QuantityControlContainer is NULL! Container cannot be shown."));
        }
        
        // Verify it was set correctly
        const FIngredientInstance& SetInstance = QuantityControlWidget->GetIngredientInstance();
        ESlateVisibility CurrentVisibility = QuantityControlWidget->GetVisibility();
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - After setting, Quantity Control has Instance ID: %d, Qty: %d, Visibility: %d"),
            SetInstance.InstanceID, SetInstance.Quantity, (int32)CurrentVisibility);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - QuantityControlWidget is NULL! Cannot show quantity control."));
        UE_LOG(LogTemp, Warning, TEXT("⚠️   QuantityControlContainer: %s, QuantityControlClass: %s"),
            QuantityControlContainer ? TEXT("EXISTS") : TEXT("NULL"),
            QuantityControlClass ? TEXT("SET") : TEXT("NULL"));
    }
}

void UPUIngredientSlot::ClearDisplay()
{
    // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ClearDisplay - Clearing display"));

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

    // DON'T remove quantity control in cooking stage (ActiveIngredientArea) - just hide it
    // Only remove it completely in other stages
    if (Location == EPUIngredientSlotLocation::ActiveIngredientArea)
    {
        // In cooking stage, just hide it, don't remove it
        if (QuantityControlWidget)
        {
            QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ClearDisplay - Hiding quantity control in cooking stage (not removing)"));
        }
        if (QuantityControlContainer)
        {
            QuantityControlContainer->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ClearDisplay - Hiding quantity control container in cooking stage (not removing)"));
        }
    }
    else
    {
        // Remove quantity control - first check if we have a reference, then search container
        if (QuantityControlWidget)
        {
            if (QuantityControlContainer)
            {
                QuantityControlContainer->RemoveChild(QuantityControlWidget);
            }
            QuantityControlWidget->RemoveFromParent();
            QuantityControlWidget = nullptr;
        }
        else if (QuantityControlContainer)
        {
            // Search for any quantity control widget in the container (in case it was placed in Blueprint)
            for (int32 i = QuantityControlContainer->GetChildrenCount() - 1; i >= 0; i--)
            {
                if (UPUIngredientQuantityControl* FoundWidget = Cast<UPUIngredientQuantityControl>(QuantityControlContainer->GetChildAt(i)))
                {
                    QuantityControlContainer->RemoveChild(FoundWidget);
                    FoundWidget->RemoveFromParent();
                    // UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ClearDisplay - Removed quantity control widget found in container"));
                    break;
                }
            }
        }
    }
}

UTexture2D* UPUIngredientSlot::GetTextureForLocation() const
{
    // For pantry and prep slots, show texture even if bHasIngredient is false (for display purposes)
    // For other locations, require bHasIngredient to be true
    if (Location != EPUIngredientSlotLocation::Pantry && Location != EPUIngredientSlotLocation::Prep && !bHasIngredient)
    {
        return nullptr;
    }

    // Check if we have valid ingredient data
    if (!IngredientInstance.IngredientData.IngredientTag.IsValid())
    {
        return nullptr;
    }

    // Use PantryTexture for pantry slots, with PreviewTexture as fallback
    if (Location == EPUIngredientSlotLocation::Pantry)
    {
        // Use PantryTexture if available, otherwise fallback to PreviewTexture
        UTexture2D* Texture = IngredientInstance.IngredientData.PantryTexture;
        if (!Texture)
        {
            Texture = IngredientInstance.IngredientData.PreviewTexture;
            if (Texture)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetTextureForLocation - Pantry slot using PreviewTexture as fallback for ingredient: %s"),
                    *IngredientInstance.IngredientData.DisplayName.ToString());
            }
        }
        if (!Texture)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::GetTextureForLocation - Pantry slot has no PantryTexture or PreviewTexture for ingredient: %s"),
                *IngredientInstance.IngredientData.DisplayName.ToString());
        }
        return Texture;
    }
    else if (Location == EPUIngredientSlotLocation::Prep)
    {
        // Prep slots use PreviewTexture (or could use a specific prep texture in the future)
        UTexture2D* Texture = IngredientInstance.IngredientData.PreviewTexture;
        if (!Texture)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::GetTextureForLocation - Prep slot has no PreviewTexture for ingredient: %s"),
                *IngredientInstance.IngredientData.DisplayName.ToString());
        }
        return Texture;
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
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDragOver - Drag over slot: %s (Location: %d, Empty: %s)"),
            *GetName(), (int32)Location, IsEmpty() ? TEXT("TRUE") : TEXT("FALSE"));

        // In cooking stage, allow drag over whether slot is empty or not (for swapping)
        if (Location == EPUIngredientSlotLocation::ActiveIngredientArea)
        {
            // Call Blueprint event for visual feedback
            OnDragOverSlot();
            return true;
        }

        // For other stages, allow drag over as before
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
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Drop on slot: %s (Ingredient: %s, Location: %d, Empty: %s)"),
            *GetName(), *IngredientDragOp->IngredientInstance.IngredientData.DisplayName.ToString(), (int32)Location, IsEmpty() ? TEXT("TRUE") : TEXT("FALSE"));

        // In cooking stage, handle both empty slots (move) and occupied slots (swap)
        if (Location == EPUIngredientSlotLocation::ActiveIngredientArea)
        {
            // SAFETY CHECK: If InstanceID is 0, this is from pantry - generate new GUID and set quantity to 1
            if (IngredientDragOp->IngredientInstance.InstanceID == 0)
            {
                UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::NativeOnDrop - Detected pantry drag (ID: 0), generating new GUID and setting quantity to 1"));
                IngredientDragOp->IngredientInstance.InstanceID = GenerateGUIDBasedInstanceID();
                IngredientDragOp->IngredientInstance.Quantity = 1;
                // Ensure IngredientTag is set (should be from IngredientData, but set it explicitly for consistency)
                if (!IngredientDragOp->IngredientInstance.IngredientTag.IsValid() && IngredientDragOp->IngredientInstance.IngredientData.IngredientTag.IsValid())
                {
                    IngredientDragOp->IngredientInstance.IngredientTag = IngredientDragOp->IngredientInstance.IngredientData.IngredientTag;
                }
                UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::NativeOnDrop - Generated new InstanceID: %d, Quantity: 1, Tag: %s"), 
                    IngredientDragOp->IngredientInstance.InstanceID, *IngredientDragOp->IngredientInstance.IngredientTag.ToString());
            }
            
            // Store the target slot's ingredient instance if it exists (for swapping)
            FIngredientInstance TargetSlotIngredient = IngredientInstance;
            bool bTargetSlotHasIngredient = !IsEmpty();
            
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Cooking stage drop (Target empty: %s)"), 
                bTargetSlotHasIngredient ? TEXT("FALSE") : TEXT("TRUE"));
            
            if (bTargetSlotHasIngredient)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯   Target slot has ingredient - ID: %d, Qty: %d, Ingredient: %s"),
                    TargetSlotIngredient.InstanceID,
                    TargetSlotIngredient.Quantity,
                    *TargetSlotIngredient.IngredientData.DisplayName.ToString());
            }
            
            UE_LOG(LogTemp, Display, TEXT("🎯   Drag Operation IngredientInstance - ID: %d, Qty: %d, Ingredient: %s, Preparations: %d"),
                IngredientDragOp->IngredientInstance.InstanceID,
                IngredientDragOp->IngredientInstance.Quantity,
                *IngredientDragOp->IngredientInstance.IngredientData.DisplayName.ToString(),
                IngredientDragOp->IngredientInstance.Preparations.Num());

            // Set the target slot with the dragged ingredient
            SetIngredientInstance(IngredientDragOp->IngredientInstance);
            
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - After SetIngredientInstance, slot has - ID: %d, Qty: %d, HasIngredient: %s, Location: %d"),
                IngredientInstance.InstanceID,
                IngredientInstance.Quantity,
                bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"),
                (int32)Location);
            
            // FORCE update and show quantity control after drop
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - FORCING quantity control to be visible after drop"));
            UpdateQuantityControl();
            
            // Double-check visibility is set correctly
            if (QuantityControlWidget)
            {
                QuantityControlWidget->SetVisibility(ESlateVisibility::Visible);
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Explicitly set QuantityControlWidget to Visible"));
            }
            if (QuantityControlContainer)
            {
                QuantityControlContainer->SetVisibility(ESlateVisibility::Visible);
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Explicitly set QuantityControlContainer to Visible"));
            }

            // Find the source slot (the slot we dragged from) in the cooking stage
            // If target slot was empty: clear the source slot (move)
            // If target slot had an ingredient: swap by setting source slot with target's ingredient (swap)
            // Try to find the parent widget (could be PUDishCustomizationWidget or PUCookingStageWidget)
            UUserWidget* ParentWidget = GetTypedOuter<UUserWidget>();
            TArray<UPUIngredientSlot*> AllSlots;
            
            // Try PUDishCustomizationWidget first
            if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(ParentWidget))
            {
                AllSlots = DishWidget->GetCreatedIngredientSlots();
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Found PUDishCustomizationWidget, got %d slots"), AllSlots.Num());
            }
            // Try PUCookingStageWidget
            else if (UPUCookingStageWidget* CookingWidget = Cast<UPUCookingStageWidget>(ParentWidget))
            {
                AllSlots = CookingWidget->GetCreatedIngredientSlots();
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Found UPUCookingStageWidget, got %d slots"), AllSlots.Num());
            }
            else
            {
                // Try to find it by traversing up the widget hierarchy
                UWidget* Parent = GetParent();
                while (Parent)
                {
                    if (UPUDishCustomizationWidget* FoundDishWidget = Cast<UPUDishCustomizationWidget>(Parent))
                    {
                        AllSlots = FoundDishWidget->GetCreatedIngredientSlots();
                        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Found PUDishCustomizationWidget via hierarchy, got %d slots"), AllSlots.Num());
                        break;
                    }
                    if (UPUCookingStageWidget* FoundCookingWidget = Cast<UPUCookingStageWidget>(Parent))
                    {
                        AllSlots = FoundCookingWidget->GetCreatedIngredientSlots();
                        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Found UPUCookingStageWidget via hierarchy, got %d slots"), AllSlots.Num());
                        break;
                    }
                    if (Parent->GetParent())
                    {
                        Parent = Parent->GetParent();
                    }
                    else
                    {
                        break;
                    }
                }
            }

            // Find the source slot with the same InstanceID in the cooking stage
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Searching %d slots for source slot with InstanceID: %d"), 
                AllSlots.Num(), IngredientDragOp->IngredientInstance.InstanceID);
            
            bool bFoundSourceSlot = false;
            for (UPUIngredientSlot* SourceSlot : AllSlots)
            {
                if (SourceSlot && SourceSlot != this)
                {
                    const FIngredientInstance& SourceInstance = SourceSlot->GetIngredientInstance();
                    UE_LOG(LogTemp, Display, TEXT("🎯   Checking slot: %s (Location: %d, InstanceID: %d, HasIngredient: %s)"),
                        *SourceSlot->GetName(),
                        (int32)SourceSlot->GetLocation(),
                        SourceInstance.InstanceID,
                        SourceSlot->IsEmpty() ? TEXT("FALSE") : TEXT("TRUE"));
                    
                    if (SourceSlot->GetLocation() == EPUIngredientSlotLocation::ActiveIngredientArea &&
                        SourceInstance.InstanceID == IngredientDragOp->IngredientInstance.InstanceID &&
                        !SourceSlot->IsEmpty())
                    {
                        if (bTargetSlotHasIngredient)
                        {
                            // SWAP: Set source slot with target slot's ingredient
                            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - SWAPPING: Setting source slot %s with target slot's ingredient (ID: %d)"), 
                                *SourceSlot->GetName(), TargetSlotIngredient.InstanceID);
                            SourceSlot->SetIngredientInstance(TargetSlotIngredient);
                            
                            // Update quantity control on source slot after swap
                            SourceSlot->UpdateQuantityControl();
                            if (UPUIngredientQuantityControl* SourceQuantityControl = SourceSlot->GetQuantityControl())
                            {
                                SourceQuantityControl->SetVisibility(ESlateVisibility::Visible);
                            }
                            if (UPanelWidget* SourceQuantityContainer = SourceSlot->GetQuantityControlContainer())
                            {
                                SourceQuantityContainer->SetVisibility(ESlateVisibility::Visible);
                            }
                        }
                        else
                        {
                            // MOVE: Clear the source slot
                            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - MOVING: Clearing source slot: %s"), *SourceSlot->GetName());
                            SourceSlot->ClearSlot();
                        }
                        bFoundSourceSlot = true;
                        break;
                    }
                }
            }
            
            if (!bFoundSourceSlot)
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::NativeOnDrop - Could not find source slot with InstanceID: %d!"), 
                    IngredientDragOp->IngredientInstance.InstanceID);
            }

            // Broadcast drop event
            OnIngredientDroppedOnSlot.Broadcast(this);

            return true;
        }

        // For other stages (plating, etc.), use the ingredient instance directly from the drag operation
        SetIngredientInstance(IngredientDragOp->IngredientInstance);

        // Ensure quantity control stays hidden in plating stage after drop
        if (Location == EPUIngredientSlotLocation::Plating && QuantityControlWidget)
        {
            QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Ensuring quantity control stays hidden in plating stage after drop"));
        }

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
    
    // Ensure quantity control stays hidden in plating stage after drag ends
    if (Location == EPUIngredientSlotLocation::Plating && QuantityControlWidget)
    {
        QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnDragLeave - Ensuring quantity control stays hidden in plating stage"));
    }
}

FReply UPUIngredientSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Mouse button down on slot: %s (Button: %s, DragEnabled: %s, Location: %d)"),
        *GetName(), *InMouseEvent.GetEffectingButton().ToString(), bDragEnabled ? TEXT("TRUE") : TEXT("FALSE"), (int32)Location);

    // Special handling for pantry slots - clicking should select the ingredient (not drag)
    if (Location == EPUIngredientSlotLocation::Pantry && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Pantry slots: if we have valid ingredient data, treat click as selection (not drag)
        if (IngredientInstance.IngredientData.IngredientTag.IsValid())
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Pantry slot clicked, triggering OnEmptySlotClicked for selection"));
            // Use OnEmptySlotClicked event for pantry slot selection (bound to OnPantrySlotClicked in cooking stage)
            OnEmptySlotClicked.Broadcast(this);
            return FReply::Handled();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::NativeOnMouseButtonDown - Pantry slot clicked but no valid ingredient tag"));
        }
    }

    // If drag is enabled and we have an ingredient, don't handle left clicks here
    // Let the drag system handle it (NativeOnPreviewMouseButtonDown will handle drag)
    // BUT: Don't do this for pantry slots - we want clicks to work for selection
    if (Location != EPUIngredientSlotLocation::Pantry && bDragEnabled && bHasIngredient && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Drag enabled, letting drag system handle left click"));
        return FReply::Unhandled(); // Let drag system handle it
    }

    if (IsEmpty())
    {
        // Empty slot clicked - open pantry (only for non-pantry locations)
        if (Location != EPUIngredientSlotLocation::Pantry)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Empty slot clicked, opening pantry"));
            OnEmptySlotClicked.Broadcast(this);
            return FReply::Handled();
        }
    }

    // Slot has ingredient - handle left/right click (only if drag is disabled for left click)
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !bDragEnabled)
    {
        // Left click - show prep radial menu (only when drag is disabled)
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Left click (drag disabled), showing prep radial menu"));
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

    if (!RadialMenuWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - RadialMenuWidgetClass not set"));
        return;
    }

    // Create the menu widget if it doesn't exist
    if (!RadialMenuWidget)
    {
        RadialMenuWidget = CreateWidget<UPURadialMenu>(GetWorld(), RadialMenuWidgetClass);
        if (!RadialMenuWidget)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ UPUIngredientSlot::ShowRadialMenu - Failed to create radial menu widget"));
            return;
        }

        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Radial menu widget created"));
    }

    // Bind to menu events only once to avoid duplicate callbacks
    if (!bRadialMenuEventsBound && RadialMenuWidget)
    {
        RadialMenuWidget->OnMenuItemSelected.AddUniqueDynamic(this, &UPUIngredientSlot::OnRadialMenuItemSelected);
        RadialMenuWidget->OnMenuClosed.AddUniqueDynamic(this, &UPUIngredientSlot::OnRadialMenuClosed);
        bRadialMenuEventsBound = true;
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Bound radial menu events"));
    }

    // Get the slot's screen position
    FVector2D SlotScreenPosition = GetSlotScreenPosition();
    
    // Build menu items based on bIsPrepMenu
    TArray<FRadialMenuItem> MenuItems;
    
    if (bIsPrepMenu)
    {
        MenuItems = BuildPreparationMenuItems();
    }
    else
    {
        MenuItems = BuildActionMenuItems();
    }
    
    if (MenuItems.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - No menu items to display"));
        return;
    }

    // Add to container if available, otherwise add to viewport (do this BEFORE setting menu items)
    if (RadialMenuContainer)
    {
        // Add to container first (if not already added)
        if (!RadialMenuWidget->GetParent())
        {
            RadialMenuContainer->AddChild(RadialMenuWidget);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Added menu to container: %s"), *RadialMenuContainer->GetName());
        }
        
        // Get container size - use multiple methods to ensure we get valid size
        FVector2D ContainerSize = RadialMenuContainer->GetDesiredSize();
        if (ContainerSize.X == 0 || ContainerSize.Y == 0)
        {
            FGeometry ContainerGeometry = RadialMenuContainer->GetCachedGeometry();
            if (ContainerGeometry.GetLocalSize().X > 0 && ContainerGeometry.GetLocalSize().Y > 0)
            {
                ContainerSize = ContainerGeometry.GetLocalSize();
            }
            else if (ContainerGeometry.GetAbsoluteSize().X > 0 && ContainerGeometry.GetAbsoluteSize().Y > 0)
            {
                ContainerSize = ContainerGeometry.GetAbsoluteSize();
            }
        }
        
        // If we have container size, use container center directly (more reliable than converting screen position)
        if (ContainerSize.X > 0 && ContainerSize.Y > 0)
        {
            FVector2D ContainerCenter = ContainerSize * 0.5f;
            
            // Set menu items and show at container center
            RadialMenuWidget->SetMenuItems(MenuItems);
            RadialMenuWidget->ShowMenuAtPosition(ContainerCenter);
            
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Using container center: (%.2f, %.2f)"), 
                ContainerCenter.X, ContainerCenter.Y);
        }
        else
        {
            // Fallback: try to convert screen position to container-relative
            FGeometry ContainerGeometry = RadialMenuContainer->GetCachedGeometry();
            FVector2D ContainerScreenPosition = ContainerGeometry.GetAbsolutePosition();
            FVector2D ContainerRelativePosition = SlotScreenPosition - ContainerScreenPosition;
            
            // Set menu items and show
            RadialMenuWidget->SetMenuItems(MenuItems);
            RadialMenuWidget->ShowMenuAtPosition(ContainerRelativePosition);
            
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - Container size not available, using converted position"));
        }
    }
    else
    {
        // Fallback to viewport if no container
        // Set menu items and show
        RadialMenuWidget->SetMenuItems(MenuItems);
        RadialMenuWidget->ShowMenuAtPosition(SlotScreenPosition);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - No container set, added to viewport"));
    }
    
    bRadialMenuVisible = true;
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Radial menu shown with %d items"), MenuItems.Num());
}

void UPUIngredientSlot::HideRadialMenu()
{
    if (RadialMenuWidget)
    {
        RadialMenuWidget->HideMenu();
        bRadialMenuVisible = false;
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::HideRadialMenu - Radial menu hidden"));
    }
}

void UPUIngredientSlot::SetRadialMenuContainer(UPanelWidget* InContainer)
{
    RadialMenuContainer = InContainer;
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetRadialMenuContainer - Set radial menu container: %s"), 
        InContainer ? *InContainer->GetName() : TEXT("NULL"));
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

    // Show hover text (works for prep/pantry slots even when "empty")
    UpdateHoverTextVisibility(true);

    // Show outline on hover by setting outline alpha to visible
    if (PlateBackground)
    {
        FSlateBrush Brush = PlateBackground->GetBrush();
        if (Brush.OutlineSettings.Width > 0.0f)
        {
            // Enable outline by making it visible
            FLinearColor OutlineColor = Brush.OutlineSettings.Color.GetSpecifiedColor();
            OutlineColor.A = 1.0f;
            Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
            PlateBackground->SetBrush(Brush);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseEnter - Showing outline on plate background"));
        }
        else
        {
            // If no outline settings, set outline width and color
            Brush.OutlineSettings.Width = 2.0f;
            FLinearColor OutlineColor = FLinearColor::White;
            OutlineColor.A = 1.0f;
            Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
            PlateBackground->SetBrush(Brush);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseEnter - Enabled outline on plate background"));
        }
    }
}

void UPUIngredientSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseLeave - Mouse left slot: %s"),
        *GetName());

    // Hide hover text
    UpdateHoverTextVisibility(false);

    // Hide outline on hover leave, but keep it visible if selected
    if (PlateBackground)
    {
        FSlateBrush Brush = PlateBackground->GetBrush();
        FLinearColor OutlineColor = Brush.OutlineSettings.Color.GetSpecifiedColor();
        // Keep outline visible if slot is selected, otherwise hide it
        OutlineColor.A = bIsSelected ? 1.0f : 0.0f;
        Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
        PlateBackground->SetBrush(Brush);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseLeave - Outline alpha set to: %.2f (Selected: %s)"), 
            OutlineColor.A, bIsSelected ? TEXT("TRUE") : TEXT("FALSE"));
    }
}

void UPUIngredientSlot::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
    Super::NativeOnAddedToFocusPath(InFocusEvent);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnAddedToFocusPath - Focus added to slot: %s"),
        *GetName());

    // Show hover text (works for prep/pantry slots even when "empty")
    UpdateHoverTextVisibility(true);

    // Show outline on focus
    if (PlateBackground)
    {
        FSlateBrush Brush = PlateBackground->GetBrush();
        if (Brush.OutlineSettings.Width > 0.0f)
        {
            FLinearColor OutlineColor = Brush.OutlineSettings.Color.GetSpecifiedColor();
            OutlineColor.A = 1.0f;
            Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
            PlateBackground->SetBrush(Brush);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnAddedToFocusPath - Showing outline on plate background"));
        }
        else
        {
            Brush.OutlineSettings.Width = 2.0f;
            FLinearColor OutlineColor = FLinearColor::White;
            OutlineColor.A = 1.0f;
            Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
            PlateBackground->SetBrush(Brush);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnAddedToFocusPath - Enabled outline on plate background"));
        }
    }
}

void UPUIngredientSlot::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
    Super::NativeOnRemovedFromFocusPath(InFocusEvent);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnRemovedFromFocusPath - Focus removed from slot: %s"),
        *GetName());

    // Hide hover text
    UpdateHoverTextVisibility(false);

    // Hide outline on focus removal, but keep it visible if selected
    if (PlateBackground)
    {
        FSlateBrush Brush = PlateBackground->GetBrush();
        FLinearColor OutlineColor = Brush.OutlineSettings.Color.GetSpecifiedColor();
        // Keep outline visible if slot is selected, otherwise hide it
        OutlineColor.A = bIsSelected ? 1.0f : 0.0f;
        Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
        PlateBackground->SetBrush(Brush);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnRemovedFromFocusPath - Outline alpha set to: %.2f (Selected: %s)"), 
            OutlineColor.A, bIsSelected ? TEXT("TRUE") : TEXT("FALSE"));
    }
}

FText UPUIngredientSlot::GetIngredientDisplayText() const
{
    // For prep/pantry slots, show text even if "empty" (quantity 0) as long as we have ingredient data
    // For other locations, require bHasIngredient to be true
    bool bShouldShowText = false;
    if (Location == EPUIngredientSlotLocation::Pantry || Location == EPUIngredientSlotLocation::Prep)
    {
        // Prep/Pantry slots: show text if we have ingredient data (even if quantity is 0)
        bShouldShowText = IngredientInstance.IngredientData.IngredientTag.IsValid();
    }
    else
    {
        // Active ingredient area: only show if we have an actual ingredient
        bShouldShowText = bHasIngredient;
    }

    if (!bShouldShowText)
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

    // For prep/pantry slots, show text if we have ingredient data (even if "empty")
    // For other locations, require bHasIngredient
    bool bCanShowText = false;
    if (Location == EPUIngredientSlotLocation::Pantry || Location == EPUIngredientSlotLocation::Prep)
    {
        bCanShowText = IngredientInstance.IngredientData.IngredientTag.IsValid();
    }
    else
    {
        bCanShowText = bHasIngredient;
    }

    if (bShow && bCanShowText)
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

void UPUIngredientSlot::SetSelected(bool bSelected)
{
    if (bIsSelected != bSelected)
    {
        bIsSelected = bSelected;
        UpdatePlateBackgroundOpacity();
        
        // Update outline visibility based on selection state
        if (PlateBackground)
        {
            FSlateBrush Brush = PlateBackground->GetBrush();
            FLinearColor OutlineColor = Brush.OutlineSettings.Color.GetSpecifiedColor();
            // Show outline if selected, hide if not selected
            OutlineColor.A = bSelected ? 1.0f : 0.0f;
            Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
            PlateBackground->SetBrush(Brush);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetSelected - Outline alpha set to: %.2f (Selected: %s)"), 
                OutlineColor.A, bSelected ? TEXT("TRUE") : TEXT("FALSE"));
        }
        
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetSelected - Selection changed to: %s"), bSelected ? TEXT("TRUE") : TEXT("FALSE"));
    }
}

void UPUIngredientSlot::UpdatePlateBackgroundOpacity()
{
    if (!PlateBackground)
    {
        return;
    }

    // Always keep plate background at 100% opacity
    // Selection and hover states are now handled by outline instead
    FLinearColor CurrentColor = PlateBackground->GetColorAndOpacity();
    CurrentColor.A = 1.0f;
    PlateBackground->SetColorAndOpacity(CurrentColor);
    
    // Set outline visibility based on selection state (shown if selected, hidden if not)
    FSlateBrush Brush = PlateBackground->GetBrush();
    FLinearColor OutlineColor = Brush.OutlineSettings.Color.GetSpecifiedColor();
    OutlineColor.A = bIsSelected ? 1.0f : 0.0f;
    Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
    PlateBackground->SetBrush(Brush);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::UpdatePlateBackgroundOpacity - Set opacity to: 1.0 (always 100%%), outline alpha: %.2f (Selected: %s)"), 
        OutlineColor.A, bIsSelected ? TEXT("TRUE") : TEXT("FALSE"));
}

TArray<FPUIngredientBase> UPUIngredientSlot::GetIngredientData() const
{
    // Try to find the parent dish customization widget to get the component
    UWidget* ParentWidget = GetParent();
    while (ParentWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(ParentWidget))
        {
            if (UPUDishCustomizationComponent* Component = DishWidget->GetCustomizationComponent())
            {
                return Component->GetIngredientData();
            }
        }
        ParentWidget = ParentWidget->GetParent();
    }
    
    // If we can't find the parent widget, try to get it from the widget hierarchy
    UUserWidget* RootWidget = GetTypedOuter<UUserWidget>();
    while (RootWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(RootWidget))
        {
            if (UPUDishCustomizationComponent* Component = DishWidget->GetCustomizationComponent())
            {
                return Component->GetIngredientData();
            }
        }
        RootWidget = RootWidget->GetTypedOuter<UUserWidget>();
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::GetIngredientData - Could not find customization component"));
    return TArray<FPUIngredientBase>();
}

int32 UPUIngredientSlot::GenerateGUIDBasedInstanceID()
{
    // Generate a GUID and convert it to a unique integer
    FGuid NewGUID = FGuid::NewGuid();
    
    // Convert GUID to a unique integer using hash
    int32 UniqueID = GetTypeHash(NewGUID);
    
    // Ensure it's positive (hash can be negative)
    UniqueID = FMath::Abs(UniqueID);
    
    UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::GenerateGUIDBasedInstanceID - Generated GUID-based InstanceID: %d from GUID: %s"), 
        UniqueID, *NewGUID.ToString());
    
    return UniqueID;
}

int32 UPUIngredientSlot::GenerateUniqueInstanceID() const
{
    // Call the static GUID-based function
    int32 UniqueID = GenerateGUIDBasedInstanceID();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GenerateUniqueInstanceID - Generated unique ID %d for ingredient slot"), UniqueID);
    
    return UniqueID;
}

void UPUIngredientSlot::SetDragEnabled(bool bEnabled)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetDragEnabled - Setting drag enabled to %s"), 
        bEnabled ? TEXT("TRUE") : TEXT("FALSE"));
    
    bDragEnabled = bEnabled;
    
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetDragEnabled - Drag enabled set to: %s"), bEnabled ? TEXT("TRUE") : TEXT("FALSE"));
}

FReply UPUIngredientSlot::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Preview mouse button down (Drag enabled: %s, HasIngredient: %s, Button: %s)"), 
        bDragEnabled ? TEXT("TRUE") : TEXT("FALSE"), 
        bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"),
        *InMouseEvent.GetEffectingButton().ToString());

    // If the radial menu is currently visible, don't start drag detection from this slot.
    // Let the radial menu / its buttons handle the click so a single click selects an item.
    if (bRadialMenuVisible)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Radial menu visible, skipping drag detection"));
        return FReply::Unhandled();
    }
    
    // Check if the click is on the quantity control widget - if so, let it handle the click
    // This prevents drag detection from interfering with button clicks
    if (QuantityControlWidget && QuantityControlWidget->GetVisibility() == ESlateVisibility::Visible && QuantityControlContainer)
    {
        // Get the quantity control container's geometry
        FGeometry ContainerGeometry = QuantityControlContainer->GetCachedGeometry();
        
        // Check if the click position is within the container's bounds
        FVector2D ClickPosition = InMouseEvent.GetScreenSpacePosition();
        FVector2D ContainerAbsolutePosition = ContainerGeometry.GetAbsolutePosition();
        FVector2D ContainerSize = ContainerGeometry.GetAbsoluteSize();
        
        // Check if click is within container bounds
        if (ClickPosition.X >= ContainerAbsolutePosition.X && 
            ClickPosition.X <= ContainerAbsolutePosition.X + ContainerSize.X &&
            ClickPosition.Y >= ContainerAbsolutePosition.Y && 
            ClickPosition.Y <= ContainerAbsolutePosition.Y + ContainerSize.Y)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Click is on quantity control (pos: %.2f,%.2f, container: %.2f,%.2f size: %.2f,%.2f), letting it handle the event"), 
                ClickPosition.X, ClickPosition.Y, ContainerAbsolutePosition.X, ContainerAbsolutePosition.Y, ContainerSize.X, ContainerSize.Y);
            return FReply::Unhandled(); // Let the quantity control buttons handle the click
        }
    }
    
    // Only handle left mouse button and only if drag is enabled
    // For pantry slots, we want clicks to work for selection, but still allow dragging if mouse moves
    bool bCanDrag = false;
    bool bIsPantrySlot = (Location == EPUIngredientSlotLocation::Pantry);
    
    if (bIsPantrySlot)
    {
        // For pantry slots, we want clicks to work for selection
        // Don't set up drag detection here - let the click handler process it
        // If user wants to drag, they can do it by actually dragging (we'll handle that separately if needed)
        if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Pantry slot clicked, allowing click handler to process (no drag detection)"));
            // Return Unhandled so NativeOnMouseButtonDown can process the click for selection
            return FReply::Unhandled();
        }
    }
    else
    {
        // Other slots require bHasIngredient to be true for dragging
        bCanDrag = bDragEnabled && bHasIngredient;
    }
    
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bCanDrag && !bIsPantrySlot)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Starting drag detection for ingredient: %s (Location: %d)"), 
            *IngredientInstance.IngredientData.DisplayName.ToString(), (int32)Location);
        
        // Hide quantity control widget when dragging in plating stage
        if (Location == EPUIngredientSlotLocation::Plating && QuantityControlWidget)
        {
            QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Hiding quantity control during drag in plating stage"));
        }
        
        // Start drag detection - this will call the Blueprint OnDragDetected event
        // Use DetectDrag which will trigger OnDragDetected when the mouse moves
        FReply Reply = FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - DetectDrag called, Reply.IsEventHandled: %s"), 
            Reply.IsEventHandled() ? TEXT("TRUE") : TEXT("FALSE"));
        return Reply;
    }
    else
    {
        if (!bDragEnabled)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Drag not enabled"));
        }
        if (!bHasIngredient)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Slot has no ingredient"));
        }
        if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Not left mouse button"));
        }
    }
    
    return FReply::Unhandled();
}

UPUIngredientDragDropOperation* UPUIngredientSlot::CreateIngredientDragDropOperation() const
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::CreateIngredientDragDropOperation - Creating drag operation for ingredient %s (ID: %d, Qty: %d, Location: %d)"), 
        *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID, IngredientInstance.Quantity, (int32)Location);

    // Create the drag drop operation
    UPUIngredientDragDropOperation* DragOperation = NewObject<UPUIngredientDragDropOperation>(GetWorld(), UPUIngredientDragDropOperation::StaticClass());

    if (DragOperation)
    {
        // Set up the drag operation with ingredient instance
        // This will generate a new GUID if InstanceID is 0 (pantry slots)
        DragOperation->SetupIngredientDrag(IngredientInstance);
        
        // Create and set the drag visual widget with the ingredient icon
        UPUIngredientSlot* DragVisual = CreateDragVisualWidget();
        if (DragVisual)
        {
            DragOperation->SetDragVisualWidget(DragVisual);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::CreateIngredientDragDropOperation - Created drag visual widget with ingredient icon"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::CreateIngredientDragDropOperation - Failed to create drag visual widget"));
        }
        
        UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::CreateIngredientDragDropOperation - Successfully created drag operation (After Setup: ID: %d, Qty: %d)"), 
            DragOperation->IngredientInstance.InstanceID, DragOperation->IngredientInstance.Quantity);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUIngredientSlot::CreateIngredientDragDropOperation - Failed to create drag operation"));
    }

    return DragOperation;
}

UPUIngredientSlot* UPUIngredientSlot::CreateDragVisualWidget() const
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::CreateDragVisualWidget - Creating drag visual widget for ingredient: %s"), 
        *IngredientInstance.IngredientData.DisplayName.ToString());

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUIngredientSlot::CreateDragVisualWidget - No world context available"));
        return nullptr;
    }

    // Get the slot class to use (same as this widget's class)
    TSubclassOf<UPUIngredientSlot> SlotClass = GetClass();
    if (!SlotClass)
    {
        SlotClass = UPUIngredientSlot::StaticClass();
    }

    // Create a new slot widget for the drag visual
    UPUIngredientSlot* DragVisualWidget = CreateWidget<UPUIngredientSlot>(GetWorld(), SlotClass);
    if (DragVisualWidget)
    {
        // IMPORTANT: Set location to ActiveIngredientArea for drag visual (not Pantry)
        // This ensures the icon shows properly regardless of source location
        DragVisualWidget->SetLocation(EPUIngredientSlotLocation::ActiveIngredientArea);
        
        // Create a copy of the ingredient instance for the drag visual
        // Ensure it has quantity > 0 so bHasIngredient is set to true
        FIngredientInstance VisualInstance = IngredientInstance;
        if (VisualInstance.Quantity <= 0)
        {
            VisualInstance.Quantity = 1; // Set to 1 for visual purposes
        }
        if (VisualInstance.InstanceID == 0)
        {
            // If InstanceID is 0, use a temporary ID (not a real GUID, just for display)
            VisualInstance.InstanceID = 999999; // Temporary ID for visual
        }
        
        // Set the ingredient instance (this will update the display including the icon)
        DragVisualWidget->SetIngredientInstance(VisualInstance);
        
        // Disable drag on the visual (it's just for display)
        DragVisualWidget->SetDragEnabled(false);
        
        // Force update the icon explicitly to ensure it's visible
        if (DragVisualWidget->IngredientIcon && IngredientInstance.IngredientData.PreviewTexture)
        {
            DragVisualWidget->IngredientIcon->SetBrushFromTexture(IngredientInstance.IngredientData.PreviewTexture);
            DragVisualWidget->IngredientIcon->SetVisibility(ESlateVisibility::Visible);
            DragVisualWidget->IngredientIcon->SetColorAndOpacity(FLinearColor::White);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::CreateDragVisualWidget - Explicitly set ingredient icon texture: %s"), 
                *IngredientInstance.IngredientData.PreviewTexture->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::CreateDragVisualWidget - IngredientIcon is null or PreviewTexture is null"));
            if (!DragVisualWidget->IngredientIcon)
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️   IngredientIcon component not found in drag visual widget"));
            }
            if (!IngredientInstance.IngredientData.PreviewTexture)
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️   PreviewTexture is null for ingredient: %s"), 
                    *IngredientInstance.IngredientData.DisplayName.ToString());
            }
        }
        
        // Update display to ensure everything is set correctly
        DragVisualWidget->UpdateDisplay();
        
        UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::CreateDragVisualWidget - Successfully created drag visual widget with ingredient icon"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUIngredientSlot::CreateDragVisualWidget - Failed to create drag visual widget"));
    }

    return DragVisualWidget;
}

void UPUIngredientSlot::DecreaseQuantity()
{
    if (RemainingQuantity > 0)
    {
        RemainingQuantity--;
        UpdateQuantityDisplay();
        
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::DecreaseQuantity - Quantity decreased to %d"), RemainingQuantity);
        
        // Call Blueprint event
        OnQuantityChanged(RemainingQuantity);
    }
}

void UPUIngredientSlot::ResetQuantity()
{
    RemainingQuantity = MaxQuantity;
    UpdateQuantityDisplay();
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::ResetQuantity - Quantity reset to %d"), RemainingQuantity);
    
    // Call Blueprint event
    OnQuantityChanged(RemainingQuantity);
}

void UPUIngredientSlot::ResetQuantityFromDishData()
{
    // Find the parent dish customization widget to get the dish data
    UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(GetTypedOuter<UPUDishCustomizationWidget>());
    if (!DishWidget)
    {
        // Try to find it by traversing up the widget hierarchy
        UWidget* Parent = GetParent();
        while (Parent && !DishWidget)
        {
            DishWidget = Cast<UPUDishCustomizationWidget>(Parent);
            if (Parent->GetParent())
            {
                Parent = Parent->GetParent();
            }
            else
            {
                break;
            }
        }
    }
    
    if (DishWidget && DishWidget->GetCustomizationComponent())
    {
        const FPUDishBase& CurrentDish = DishWidget->GetCustomizationComponent()->GetCurrentDishData();
        FIngredientInstance UpdatedInstance;
        if (CurrentDish.GetIngredientInstanceByID(IngredientInstance.InstanceID, UpdatedInstance))
        {
            // Reset to the original quantity from the dish data
            IngredientInstance.Quantity = UpdatedInstance.Quantity;
            RemainingQuantity = IngredientInstance.Quantity;
            MaxQuantity = IngredientInstance.Quantity;
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::ResetQuantityFromDishData - Reset quantity to %d from dish data"), IngredientInstance.Quantity);
            
            // Update all visual displays
            UpdateQuantityDisplay();
            UpdateIngredientIcon();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ResetQuantityFromDishData - Could not find ingredient instance with ID %d"), IngredientInstance.InstanceID);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ResetQuantityFromDishData - Could not find dish customization widget or component"));
    }
}

void UPUIngredientSlot::UpdatePlatingDisplay()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::UpdatePlatingDisplay - Updating plating display"));
    
    // Update the ingredient icon/texture
    if (IngredientIcon)
    {
        IngredientIcon->SetBrushFromTexture(IngredientInstance.IngredientData.PreviewTexture);
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::UpdatePlatingDisplay - Updated icon texture"));
    }
    
    // Update quantity and preparation displays
    UpdateQuantityDisplay();
    UpdatePreparationDisplay();
}

void UPUIngredientSlot::UpdateQuantityDisplay()
{
    if (QuantityText)
    {
        // Hide QuantityText in prep and cooking stages (show in plating)
        if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea)
        {
            QuantityText->SetVisibility(ESlateVisibility::Collapsed);
            // UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::UpdateQuantityDisplay - Hiding QuantityText (Location: %d)"), (int32)Location);
        }
        else if (bHasIngredient && IngredientInstance.InstanceID != 0)
        {
            // Read quantity directly from the ingredient instance
            int32 Quantity = IngredientInstance.Quantity;
            FString QuantityString = FString::Printf(TEXT("x%d"), Quantity);
            QuantityText->SetText(FText::FromString(QuantityString));
            QuantityText->SetVisibility(ESlateVisibility::Visible);
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::UpdateQuantityDisplay - Updated quantity: %s (from IngredientInstance.Quantity)"), *QuantityString);
        }
        else
        {
            // No ingredient, hide the text
            QuantityText->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::UpdateQuantityDisplay - No ingredient, hiding QuantityText"));
        }
    }
}

void UPUIngredientSlot::UpdatePreparationDisplay()
{
    if (PreparationText)
    {
        // Hide PreparationText in prep, cooking, and plating stages
        if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Plating)
        {
            PreparationText->SetVisibility(ESlateVisibility::Collapsed);
            // UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::UpdatePreparationDisplay - Hiding PreparationText (Location: %d)"), (int32)Location);
        }
        else
        {
            FString IconText = GetPreparationIconText();
            PreparationText->SetText(FText::FromString(IconText));
            PreparationText->SetVisibility(ESlateVisibility::Visible);
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::UpdatePreparationDisplay - Updated preparation icons: %s"), *IconText);
        }
    }
    
    // Call Blueprint event
    OnPreparationStateChanged();
}

FString UPUIngredientSlot::GetPreparationDisplayText() const
{
    if (IngredientInstance.Preparations.Num() == 0)
    {
        return TEXT("");
    }
    
    FString PrepNames;
    for (const FGameplayTag& Prep : IngredientInstance.Preparations)
    {
        if (!PrepNames.IsEmpty()) 
        {
            PrepNames += TEXT(", ");
        }
        PrepNames += Prep.ToString().Replace(TEXT("Prep."), TEXT(""));
    }
    
    return PrepNames;
}

FString UPUIngredientSlot::GetPreparationIconText() const
{
    if (IngredientInstance.Preparations.Num() == 0)
    {
        return TEXT("");
    }
    
    FString IconString;
    for (const FGameplayTag& Prep : IngredientInstance.Preparations)
    {
        FString PrepName = Prep.ToString().Replace(TEXT("Prep."), TEXT(""));
        
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Processing preparation tag: %s (cleaned: %s)"), 
            *Prep.ToString(), *PrepName);
        
        // Map preparation tags to text abbreviations
        if (PrepName.Contains(TEXT("Dehydrate")) || PrepName.Contains(TEXT("Dried")))
        {
            IconString += TEXT("[D]");
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Dehydrate/Dried: [D]"));
        }
        else if (PrepName.Contains(TEXT("Mince")) || PrepName.Contains(TEXT("Minced")))
        {
            IconString += TEXT("[M]");
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Mince/Minced: [M]"));
        }
        else if (PrepName.Contains(TEXT("Boiled")))
        {
            IconString += TEXT("[B]");
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Boiled: [B]"));
        }
        else if (PrepName.Contains(TEXT("Chopped")))
        {
            IconString += TEXT("[C]");
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Chopped: [C]"));
        }
        else if (PrepName.Contains(TEXT("Caramelized")))
        {
            IconString += TEXT("[CR]");
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Caramelized: [CR]"));
        }
        else
        {
            // Default abbreviation for unknown preparations
            IconString += TEXT("[?]");
            UE_LOG(LogTemp, Warning, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Unknown preparation: %s (cleaned: %s)"), 
                *Prep.ToString(), *PrepName);
        }
    }
    
    return IconString;
}

void UPUIngredientSlot::SpawnIngredientAtPosition(const FVector2D& ScreenPosition)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::SpawnIngredientAtPosition - START - Ingredient %s at screen position (%.2f,%.2f)"), 
        *IngredientInstance.IngredientData.DisplayName.ToString(), ScreenPosition.X, ScreenPosition.Y);

    // Convert screen position to world position using raycast
    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::SpawnIngredientAtPosition - No player controller"));
        return;
    }

    // Get camera location and rotation
    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
    
    // Get viewport size
    int32 ViewportSizeX, ViewportSizeY;
    PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::SpawnIngredientAtPosition - Viewport: %dx%d, Mouse: (%.0f,%.0f)"), 
        ViewportSizeX, ViewportSizeY, ScreenPosition.X, ScreenPosition.Y);
    
    // Find the dish customization station in the world
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);
    
    AActor* DishStation = nullptr;
    for (AActor* Actor : FoundActors)
    {
        if (Actor && (Actor->GetName().Contains(TEXT("CookingStation")) || Actor->GetName().Contains(TEXT("DishCustomization"))))
        {
            DishStation = Actor;
            break;
        }
    }
    
    // Declare spawn position at function level
    FVector SpawnPosition;
    
    if (DishStation)
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Found dish customization station: %s"), *DishStation->GetName());
        
        // Get the station's location and bounds
        FVector StationLocation = DishStation->GetActorLocation();
        FVector StationBounds = DishStation->GetComponentsBoundingBox().GetSize();
        
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Station location: (%.2f,%.2f,%.2f), bounds: (%.2f,%.2f,%.2f)"), 
            StationLocation.X, StationLocation.Y, StationLocation.Z, StationBounds.X, StationBounds.Y, StationBounds.Z);
        
        // Calculate spawn position on the station surface
        // Use a small random offset to avoid stacking ingredients exactly on top of each other
        float RandomOffsetX = FMath::RandRange(-50.0f, 50.0f);
        float RandomOffsetY = FMath::RandRange(-50.0f, 50.0f);
        
        SpawnPosition = StationLocation + FVector(RandomOffsetX, RandomOffsetY, StationBounds.Z * 0.2f);
        
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Spawning on dish customization station at: (%.2f,%.2f,%.2f)"), 
            SpawnPosition.X, SpawnPosition.Y, SpawnPosition.Z);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ No dish customization station found! Spawning at default position."));
        
        // Fallback: spawn near the player
        SpawnPosition = CameraLocation + (CameraRotation.Vector() * 300.0f);
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Fallback spawn position: (%.2f,%.2f,%.2f)"), 
            SpawnPosition.X, SpawnPosition.Y, SpawnPosition.Z);
    }
    
    // Find the dish customization component and spawn the ingredient
    UPUDishCustomizationComponent* DishComponent = nullptr;
    for (AActor* Actor : FoundActors)
    {
        if (Actor)
        {
            DishComponent = Actor->FindComponentByClass<UPUDishCustomizationComponent>();
            if (DishComponent)
            {
                // Check if we're in plating mode before spawning
                if (!DishComponent->IsPlatingMode())
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::SpawnIngredientAtPosition - Not in plating mode, aborting spawn"));
                    return;
                }
                
                UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::SpawnIngredientAtPosition - Calling SpawnIngredientIn3DByInstanceID on customization component"));
                DishComponent->SpawnIngredientIn3DByInstanceID(IngredientInstance.InstanceID, SpawnPosition);
                UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::SpawnIngredientAtPosition - SpawnIngredientIn3DByInstanceID call completed"));
                break;
            }
        }
    }
    
    // If we didn't find a component or weren't in plating mode, return early
    if (!DishComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::SpawnIngredientAtPosition - Could not find dish customization component"));
        return;
    }
    
    // After spawning, decrease the quantity by 1 and update the display
    // (PlaceIngredient tracks placements in the component, but we need to update our local display)
    if (IngredientInstance.Quantity > 0)
    {
        // Decrease quantity by 1 since we just spawned one
        IngredientInstance.Quantity--;
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::SpawnIngredientAtPosition - Decreased quantity to %d (spawned 1)"), IngredientInstance.Quantity);
        
        // Update the display to reflect the new quantity
        UpdateQuantityDisplay();
        UpdateIngredientIcon();
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientSlot::SpawnIngredientAtPosition - END"));
}

void UPUIngredientSlot::SetTextVisibility(bool bShowQuantity, bool bShowDescription)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetTextVisibility - Setting text visibility: Quantity=%s, Description=%s"), 
        bShowQuantity ? TEXT("TRUE") : TEXT("FALSE"), bShowDescription ? TEXT("TRUE") : TEXT("FALSE"));
    
    // Control quantity text visibility
    if (QuantityText)
    {
        QuantityText->SetVisibility(bShowQuantity ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetTextVisibility - Set quantity text visibility to: %s"), 
            bShowQuantity ? TEXT("Visible") : TEXT("Hidden"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::SetTextVisibility - QuantityText component not found"));
    }
    
    // Control preparation/description text visibility
    if (PreparationText)
    {
        PreparationText->SetVisibility(bShowDescription ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetTextVisibility - Set preparation text visibility to: %s"), 
            bShowDescription ? TEXT("Visible") : TEXT("Hidden"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::SetTextVisibility - PreparationText component not found"));
    }
}

void UPUIngredientSlot::HideAllText()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::HideAllText - Hiding all text elements"));
    LogTextComponentStatus();
    SetTextVisibility(false, false);
    
    // Force hide using different visibility modes
    if (QuantityText)
    {
        QuantityText->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::HideAllText - Force collapsed quantity text"));
    }
    
    if (PreparationText)
    {
        PreparationText->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::HideAllText - Force collapsed preparation text"));
    }
}

void UPUIngredientSlot::ShowAllText()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ShowAllText - Showing all text elements"));
    LogTextComponentStatus();
    SetTextVisibility(true, true);
}

void UPUIngredientSlot::LogTextComponentStatus()
{
    UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::LogTextComponentStatus - Checking text component status"));
    
    UE_LOG(LogTemp, Display, TEXT("🔍 QuantityText: %s"), QuantityText ? TEXT("FOUND") : TEXT("NULL"));
    UE_LOG(LogTemp, Display, TEXT("🔍 PreparationText: %s"), PreparationText ? TEXT("FOUND") : TEXT("NULL"));
    UE_LOG(LogTemp, Display, TEXT("🔍 HoverText: %s"), HoverText ? TEXT("FOUND") : TEXT("NULL"));
    UE_LOG(LogTemp, Display, TEXT("🔍 IngredientIcon: %s"), IngredientIcon ? TEXT("FOUND") : TEXT("NULL"));
    
    if (QuantityText)
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 QuantityText visibility: %s"), 
            QuantityText->GetVisibility() == ESlateVisibility::Visible ? TEXT("VISIBLE") : TEXT("HIDDEN"));
    }
    
    if (PreparationText)
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 PreparationText visibility: %s"), 
            PreparationText->GetVisibility() == ESlateVisibility::Visible ? TEXT("VISIBLE") : TEXT("HIDDEN"));
    }
}

void UPUIngredientSlot::OnRadialMenuItemSelected(const FRadialMenuItem& SelectedItem)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::OnRadialMenuItemSelected - Item selected: %s (Tag: %s)"),
        *SelectedItem.Label.ToString(), *SelectedItem.ActionTag.ToString());

    if (!SelectedItem.bIsEnabled)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::OnRadialMenuItemSelected - Item is disabled, ignoring selection"));
        return;
    }

    // Check if it's a preparation tag or action tag
    FString TagString = SelectedItem.ActionTag.ToString();
    if (TagString.StartsWith(TEXT("Prep.")))
    {
        // It's a preparation - toggle it (apply if not applied, remove if applied)
        if (SelectedItem.bIsApplied)
        {
            RemovePreparationFromIngredient(SelectedItem.ActionTag);
        }
        else
        {
            ApplyPreparationToIngredient(SelectedItem.ActionTag);
        }
    }
    else if (TagString.StartsWith(TEXT("Action.")))
    {
        // It's an action - execute it
        ExecuteAction(SelectedItem.ActionTag);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::OnRadialMenuItemSelected - Unknown action tag: %s"), *TagString);
    }
    
    // Hide the menu after selection
    HideRadialMenu();
}

void UPUIngredientSlot::OnRadialMenuClosed()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::OnRadialMenuClosed - Radial menu closed"));
    bRadialMenuVisible = false;
}

FVector2D UPUIngredientSlot::GetSlotScreenPosition() const
{
    // Get the slot widget's geometry
    FGeometry SlotGeometry = GetCachedGeometry();
    
    // Get the absolute position and size
    FVector2D AbsolutePosition = SlotGeometry.GetAbsolutePosition();
    FVector2D AbsoluteSize = SlotGeometry.GetAbsoluteSize();
    
    // Calculate center position of the slot
    FVector2D CenterPosition = AbsolutePosition + (AbsoluteSize * 0.5f);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::GetSlotScreenPosition - Slot center: (%.2f, %.2f)"),
        CenterPosition.X, CenterPosition.Y);
    
    return CenterPosition;
}

TArray<FRadialMenuItem> UPUIngredientSlot::BuildPreparationMenuItems() const
{
    TArray<FRadialMenuItem> MenuItems;

    if (!bHasIngredient)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::BuildPreparationMenuItems - Slot has no ingredient"));
        return MenuItems;
    }

    // Check if we have a preparation data table
    if (!IngredientInstance.IngredientData.PreparationDataTable.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::BuildPreparationMenuItems - No preparation data table available"));
        return MenuItems;
    }

    UDataTable* LoadedPreparationDataTable = IngredientInstance.IngredientData.PreparationDataTable.LoadSynchronous();
    if (!LoadedPreparationDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::BuildPreparationMenuItems - Failed to load preparation data table"));
        return MenuItems;
    }

    // Get all preparations from the data table
    TArray<FPUPreparationBase*> PreparationRows;
    LoadedPreparationDataTable->GetAllRows<FPUPreparationBase>(TEXT("BuildPreparationMenuItems"), PreparationRows);

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::BuildPreparationMenuItems - Found %d total preparations"), PreparationRows.Num());

    // Build menu items for each preparation
    for (FPUPreparationBase* PreparationData : PreparationRows)
    {
        if (!PreparationData || !PreparationData->PreparationTag.IsValid())
        {
            continue;
        }

        // Check if this preparation is compatible with the ingredient
        // We need to check compatibility with current preparations
        bool bIsCompatible = PreparationData->CanApplyToIngredient(IngredientInstance.Preparations);
        
        // Check if this preparation is already applied
        bool bIsApplied = IngredientInstance.Preparations.HasTag(PreparationData->PreparationTag);

        // Create menu item
        FRadialMenuItem MenuItem;
        MenuItem.Label = PreparationData->DisplayName.IsEmpty() ? 
            FText::FromString(PreparationData->PreparationTag.ToString()) : 
            PreparationData->DisplayName;
        MenuItem.Icon = PreparationData->PreviewTexture;
        MenuItem.ActionTag = PreparationData->PreparationTag;
        MenuItem.bIsEnabled = bIsCompatible || bIsApplied; // Enable if compatible or already applied (to allow removal)
        MenuItem.bIsApplied = bIsApplied;
        MenuItem.Tooltip = PreparationData->Description;

        MenuItems.Add(MenuItem);

        UE_LOG(LogTemp, Display, TEXT("🎯   Preparation: %s (Compatible: %s, Applied: %s)"),
            *MenuItem.Label.ToString(),
            bIsCompatible ? TEXT("YES") : TEXT("NO"),
            bIsApplied ? TEXT("YES") : TEXT("NO"));
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::BuildPreparationMenuItems - Built %d menu items"), MenuItems.Num());
    return MenuItems;
}

TArray<FRadialMenuItem> UPUIngredientSlot::BuildActionMenuItems() const
{
    TArray<FRadialMenuItem> MenuItems;

    if (!bHasIngredient)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::BuildActionMenuItems - Slot has no ingredient"));
        return MenuItems;
    }

    // Action: Remove Ingredient
    FRadialMenuItem RemoveItem;
    RemoveItem.Label = FText::FromString(TEXT("Remove"));
    RemoveItem.ActionTag = FGameplayTag::RequestGameplayTag(FName("Action.Remove"));
    RemoveItem.bIsEnabled = true;
    RemoveItem.Tooltip = FText::FromString(TEXT("Remove this ingredient from the dish"));
    MenuItems.Add(RemoveItem);

    // Action: Clear Preparations (only if there are preparations)
    if (IngredientInstance.Preparations.Num() > 0)
    {
        FRadialMenuItem ClearPrepsItem;
        ClearPrepsItem.Label = FText::FromString(TEXT("Clear Preparations"));
        ClearPrepsItem.ActionTag = FGameplayTag::RequestGameplayTag(FName("Action.ClearPreparations"));
        ClearPrepsItem.bIsEnabled = true;
        ClearPrepsItem.Tooltip = FText::FromString(TEXT("Remove all preparations from this ingredient"));
        MenuItems.Add(ClearPrepsItem);
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::BuildActionMenuItems - Built %d action menu items"), MenuItems.Num());
    return MenuItems;
}

bool UPUIngredientSlot::ApplyPreparationToIngredient(const FGameplayTag& PreparationTag)
{
    if (!bHasIngredient || IngredientInstance.InstanceID == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ApplyPreparationToIngredient - Invalid ingredient instance"));
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ApplyPreparationToIngredient - Applying preparation %s to instance %d"),
        *PreparationTag.ToString(), IngredientInstance.InstanceID);

    // Find the dish customization component to get the dish data
    UPUDishCustomizationComponent* DishComponent = GetDishCustomizationComponent();
    if (!DishComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ApplyPreparationToIngredient - Could not find dish customization component"));
        return false;
    }

    // Get the current dish data
    FPUDishBase CurrentDish = DishComponent->GetCurrentDishData();

    // Apply the preparation using the blueprint library
    bool bSuccess = UPUDishBlueprintLibrary::ApplyPreparationByID(CurrentDish, IngredientInstance.InstanceID, PreparationTag);
    
    if (bSuccess)
    {
        // Update the dish data in the component
        DishComponent->UpdateCurrentDishData(CurrentDish);
        
        // Update the ingredient instance to reflect the change
        FIngredientInstance UpdatedInstance;
        if (CurrentDish.GetIngredientInstanceByID(IngredientInstance.InstanceID, UpdatedInstance))
        {
            SetIngredientInstance(UpdatedInstance);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::ApplyPreparationToIngredient - Preparation applied successfully"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ApplyPreparationToIngredient - Failed to apply preparation"));
    }

    return bSuccess;
}

bool UPUIngredientSlot::RemovePreparationFromIngredient(const FGameplayTag& PreparationTag)
{
    if (!bHasIngredient || IngredientInstance.InstanceID == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::RemovePreparationFromIngredient - Invalid ingredient instance"));
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::RemovePreparationFromIngredient - Removing preparation %s from instance %d"),
        *PreparationTag.ToString(), IngredientInstance.InstanceID);

    // Find the dish customization component to get the dish data
    UPUDishCustomizationComponent* DishComponent = GetDishCustomizationComponent();
    if (!DishComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::RemovePreparationFromIngredient - Could not find dish customization component"));
        return false;
    }

    // Get the current dish data
    FPUDishBase CurrentDish = DishComponent->GetCurrentDishData();

    // Remove the preparation using the blueprint library
    bool bSuccess = UPUDishBlueprintLibrary::RemovePreparationByID(CurrentDish, IngredientInstance.InstanceID, PreparationTag);
    
    if (bSuccess)
    {
        // Update the dish data in the component
        DishComponent->UpdateCurrentDishData(CurrentDish);
        
        // Update the ingredient instance to reflect the change
        FIngredientInstance UpdatedInstance;
        if (CurrentDish.GetIngredientInstanceByID(IngredientInstance.InstanceID, UpdatedInstance))
        {
            SetIngredientInstance(UpdatedInstance);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::RemovePreparationFromIngredient - Preparation removed successfully"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::RemovePreparationFromIngredient - Failed to remove preparation"));
    }

    return bSuccess;
}

void UPUIngredientSlot::ExecuteAction(const FGameplayTag& ActionTag)
{
    FString ActionString = ActionTag.ToString();
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ExecuteAction - Executing action: %s"), *ActionString);

    if (ActionString == TEXT("Action.Remove"))
    {
        // Remove the ingredient instance - try to get the widget first (preferred method)
        UPUDishCustomizationWidget* DishWidget = GetDishCustomizationWidget();
        if (DishWidget)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ExecuteAction - Found dish widget, removing instance ID: %d"), IngredientInstance.InstanceID);
            DishWidget->RemoveIngredientInstance(IngredientInstance.InstanceID);
            ClearSlot();
            UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::ExecuteAction - Ingredient removed successfully via widget"));
        }
        else
        {
            // Fallback: try to get the component directly
            UPUDishCustomizationComponent* DishComponent = GetDishCustomizationComponent();
            if (DishComponent)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::ExecuteAction - Found dish component, removing instance ID: %d"), IngredientInstance.InstanceID);
                FPUDishBase CurrentDish = DishComponent->GetCurrentDishData();
                bool bSuccess = UPUDishBlueprintLibrary::RemoveIngredientInstanceByID(CurrentDish, IngredientInstance.InstanceID);
                if (bSuccess)
                {
                    DishComponent->UpdateCurrentDishData(CurrentDish);
                    ClearSlot();
                    UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::ExecuteAction - Ingredient removed successfully via component"));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ExecuteAction - Failed to remove ingredient instance ID: %d"), IngredientInstance.InstanceID);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ UPUIngredientSlot::ExecuteAction - Could not find dish customization widget or component! Cannot remove ingredient."));
            }
        }
    }
    else if (ActionString == TEXT("Action.ClearPreparations"))
    {
        // Remove all preparations
        TArray<FGameplayTag> PreparationTags;
        IngredientInstance.Preparations.GetGameplayTagArray(PreparationTags);
        
        for (const FGameplayTag& PrepTag : PreparationTags)
        {
            RemovePreparationFromIngredient(PrepTag);
        }
        
        UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::ExecuteAction - All preparations cleared"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::ExecuteAction - Unknown action: %s"), *ActionString);
    }
}

UPUDishCustomizationComponent* UPUIngredientSlot::GetDishCustomizationComponent() const
{
    // Try to find the dish customization component by traversing up the widget hierarchy
    UWidget* Parent = GetParent();
    while (Parent)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(Parent))
        {
            return DishWidget->GetCustomizationComponent();
        }
        if (UPUCookingStageWidget* CookingWidget = Cast<UPUCookingStageWidget>(Parent))
        {
            // Cooking stage widget might have a reference to the component
            // Try to get it from the parent widget hierarchy
            UUserWidget* RootWidget = GetTypedOuter<UUserWidget>();
            while (RootWidget)
            {
                if (UPUDishCustomizationWidget* FoundDishWidget = Cast<UPUDishCustomizationWidget>(RootWidget))
                {
                    return FoundDishWidget->GetCustomizationComponent();
                }
                RootWidget = RootWidget->GetTypedOuter<UUserWidget>();
            }
        }
        Parent = Parent->GetParent();
    }
    
    // Try to get it from the outer widget
    UUserWidget* RootWidget = GetTypedOuter<UUserWidget>();
    while (RootWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(RootWidget))
        {
            return DishWidget->GetCustomizationComponent();
        }
        RootWidget = RootWidget->GetTypedOuter<UUserWidget>();
    }
    
    return nullptr;
}

UPUDishCustomizationWidget* UPUIngredientSlot::GetDishCustomizationWidget() const
{
    UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - START (Slot: %s)"), *GetName());
    
    // First, check if we have a cached reference
    if (CachedDishWidget.IsValid())
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - Found cached widget: %s"), 
            *CachedDishWidget.Get()->GetName());
        return CachedDishWidget.Get();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::GetDishCustomizationWidget - Cached widget is NULL or invalid"));
    }

    // Try to get it from the outer widget (slots are created with dish widget as outer)
    UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - Trying GetOuter() traversal"));
    UObject* Outer = GetOuter();
    int32 OuterDepth = 0;
    while (Outer && OuterDepth < 10) // Limit depth to prevent infinite loops
    {
        UE_LOG(LogTemp, Display, TEXT("🔍   Outer[%d]: %s (Class: %s)"), OuterDepth, *Outer->GetName(), *Outer->GetClass()->GetName());
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(Outer))
        {
            UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::GetDishCustomizationWidget - Found via GetOuter(): %s"), *DishWidget->GetName());
            // Cache it for future use
            const_cast<UPUIngredientSlot*>(this)->CachedDishWidget = DishWidget;
            return DishWidget;
        }
        Outer = Outer->GetOuter();
        OuterDepth++;
    }

    // Try to find the dish customization widget by traversing up the widget hierarchy
    UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - Trying GetParent() traversal"));
    UWidget* Parent = GetParent();
    int32 ParentDepth = 0;
    while (Parent && ParentDepth < 10) // Limit depth to prevent infinite loops
    {
        UE_LOG(LogTemp, Display, TEXT("🔍   Parent[%d]: %s (Class: %s)"), ParentDepth, *Parent->GetName(), *Parent->GetClass()->GetName());
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(Parent))
        {
            UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::GetDishCustomizationWidget - Found via GetParent(): %s"), *DishWidget->GetName());
            // Cache it for future use
            const_cast<UPUIngredientSlot*>(this)->CachedDishWidget = DishWidget;
            return DishWidget;
        }
        if (UPUCookingStageWidget* CookingWidget = Cast<UPUCookingStageWidget>(Parent))
        {
            UE_LOG(LogTemp, Display, TEXT("🔍   Found CookingStageWidget, trying to get dish widget from it"));
            // Cooking stage widget might have a reference to the dish widget
            // Try to get it from the parent widget hierarchy
            UUserWidget* RootWidget = GetTypedOuter<UUserWidget>();
            while (RootWidget)
            {
                if (UPUDishCustomizationWidget* FoundDishWidget = Cast<UPUDishCustomizationWidget>(RootWidget))
                {
                    UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::GetDishCustomizationWidget - Found via CookingStageWidget: %s"), *FoundDishWidget->GetName());
                    // Cache it for future use
                    const_cast<UPUIngredientSlot*>(this)->CachedDishWidget = FoundDishWidget;
                    return FoundDishWidget;
                }
                RootWidget = RootWidget->GetTypedOuter<UUserWidget>();
            }
        }
        Parent = Parent->GetParent();
        ParentDepth++;
    }
    
    // Try GetTypedOuter as a last resort
    UE_LOG(LogTemp, Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - Trying GetTypedOuter() traversal"));
    UUserWidget* RootWidget = GetTypedOuter<UUserWidget>();
    int32 TypedOuterDepth = 0;
    while (RootWidget && TypedOuterDepth < 10)
    {
        UE_LOG(LogTemp, Display, TEXT("🔍   TypedOuter[%d]: %s (Class: %s)"), TypedOuterDepth, *RootWidget->GetName(), *RootWidget->GetClass()->GetName());
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(RootWidget))
        {
            UE_LOG(LogTemp, Display, TEXT("✅ UPUIngredientSlot::GetDishCustomizationWidget - Found via GetTypedOuter(): %s"), *DishWidget->GetName());
            // Cache it for future use
            const_cast<UPUIngredientSlot*>(this)->CachedDishWidget = DishWidget;
            return DishWidget;
        }
        RootWidget = RootWidget->GetTypedOuter<UUserWidget>();
        TypedOuterDepth++;
    }
    
    UE_LOG(LogTemp, Error, TEXT("❌ UPUIngredientSlot::GetDishCustomizationWidget - Could not find dish widget! Slot: %s"), *GetName());
    return nullptr;
}

void UPUIngredientSlot::SetDishCustomizationWidget(UPUDishCustomizationWidget* InDishWidget)
{
    CachedDishWidget = InDishWidget;
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUIngredientSlot::SetDishCustomizationWidget - Cached dish widget reference: %s"), 
        InDishWidget ? *InDishWidget->GetName() : TEXT("NULL"));
}

