#include "PUIngredientSlot.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ImageUtils.h"
#include "PUIngredientQuantityControl.h"
#include "PUIngredientDragDropOperation.h"
#include "Input/Events.h"
#include "../DishCustomization/PUPreparationBase.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "Engine/DataTable.h"
#include "PUDishCustomizationWidget.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SlateWrapperTypes.h"
#include "GameplayTagContainer.h"
#include "PURadialMenu.h"
#include "../DishCustomization/PUDishBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"

// Debug output toggles (kept in code, but disabled by default to avoid log spam).
namespace
{
    constexpr bool bPU_LogIngredientSlotDebug = false;
}

UPUIngredientSlot::UPUIngredientSlot(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bHasIngredient(false)
    , Location(EPUIngredientSlotLocation::ActiveIngredientArea)
    , QuantityControlWidget(nullptr)
    , RadialMenuWidget(nullptr)
    , bRadialMenuVisible(false)
    , bDragEnabled(true)  // Enable drag by default for testing
{
}

void UPUIngredientSlot::NativeConstruct()
{
    Super::NativeConstruct();

    if (bPU_LogIngredientSlotDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeConstruct - Slot widget constructed: %s"), *GetName());
    }

    // Try to find and cache the dish widget if not already set
    if (!CachedDishWidget.IsValid())
    {
        if (bPU_LogIngredientSlotDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::NativeConstruct - Cached dish widget not set, attempting to find it"));
        }
        UPUDishCustomizationWidget* FoundWidget = GetDishCustomizationWidget();
        if (FoundWidget)
        {
            if (bPU_LogIngredientSlotDebug)
            {
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::NativeConstruct - Found and cached dish widget: %s"), *FoundWidget->GetName());
            }
        }
        else
        {
            // //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::NativeConstruct - Could not find dish widget during construction"));
        }
    }
    else
    {
        if (UPUDishCustomizationWidget* DishWidget = CachedDishWidget.Get())
        {
            if (bPU_LogIngredientSlotDebug)
            {
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::NativeConstruct - Cached dish widget already set: %s"), 
                //    *DishWidget->GetName());
            }
        }
    }

    // Hide hover text by default
    if (HoverText)
    {
        HoverText->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Hide IngredientSelect by default (shown on hover/focus)
    if (IngredientSelect)
    {
        IngredientSelect->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Preload material instances to ensure they're available at runtime
    // This fixes the issue where materials don't work on startup/builds
    // Try to load even if IsValid() returns false (sometimes the path exists but IsValid() fails)
    if (PreparationMaterialInstance.IsValid() || !PreparationMaterialInstance.ToSoftObjectPath().IsNull())
    {
        // Force load the material instance to ensure it's in memory
        UMaterialInterface* BaseMaterial = PreparationMaterialInstance.LoadSynchronous();
        if (BaseMaterial)
        {
            // Pre-create the dynamic material instance so it's ready when needed
            if (!PreparationDynamicMaterial)
            {
                PreparationDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::NativeConstruct - Failed to load PreparationMaterialInstance: %s"), 
                *PreparationMaterialInstance.ToSoftObjectPath().ToString());
        }
    }
    
    // Also preload suspicious material instance
    // Try to load even if IsValid() returns false (sometimes the path exists but IsValid() fails)
    if (SuspiciousMaterialInstance.IsValid() || !SuspiciousMaterialInstance.ToSoftObjectPath().IsNull())
    {
        UMaterialInterface* BaseMaterial = SuspiciousMaterialInstance.LoadSynchronous();
        if (BaseMaterial)
        {
            if (!SuspiciousDynamicMaterial)
            {
                SuspiciousDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::NativeConstruct - Failed to load SuspiciousMaterialInstance: %s"), 
                *SuspiciousMaterialInstance.ToSoftObjectPath().ToString());
        }
    }

    // Initialize plate background (always 100% opacity, outline hidden by default)
    UpdatePlateBackgroundOpacity();

    // Hide QuantityText in prep and cooking stages (shown in plating)
    if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Prepped)
    {
        if (QuantityText)
        {
            QuantityText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    // Hide PreparationText in prep, cooking, plating, and prepped stages
    if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Plating || Location == EPUIngredientSlotLocation::Prepped)
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
        if (bPU_LogIngredientSlotDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeConstruct - Initial ingredient instance found, applying: %s"), 
            //    *InitialIngredientInstance.IngredientData.DisplayName.ToString());
        }
        SetIngredientInstance(InitialIngredientInstance);
    }
    else
    {
        // Update display on construction (only if no initial instance was set)
        UpdateDisplay();
    }

    // Initialize time/temperature sliders
    InitializeTimeTempSliders();

    // Make slots focusable for controller navigation (especially important for prep stage)
    // Prep, Pantry, and ActiveIngredientArea slots should be focusable
    if (Location == EPUIngredientSlotLocation::Prep || 
        Location == EPUIngredientSlotLocation::Pantry || 
        Location == EPUIngredientSlotLocation::ActiveIngredientArea ||
        Location == EPUIngredientSlotLocation::Prepped)
    {
        SetIsFocusable(true);
    }
}

void UPUIngredientSlot::NativeDestruct()
{
    // Clean up quantity control widget delegate bindings
    if (QuantityControlWidget && IsValid(QuantityControlWidget) && bQuantityControlEventsBound)
    {
        QuantityControlWidget->OnQuantityControlChanged.RemoveDynamic(this, &UPUIngredientSlot::OnQuantityControlChanged);
        QuantityControlWidget->OnQuantityControlRemoved.RemoveDynamic(this, &UPUIngredientSlot::OnQuantityControlRemoved);
        bQuantityControlEventsBound = false;
    }

    // Clean up quantity control widget
    if (QuantityControlWidget)
    {
        if (QuantityControlContainer && IsValid(QuantityControlContainer))
        {
            QuantityControlContainer->RemoveChild(QuantityControlWidget);
        }
        if (IsValid(QuantityControlWidget))
        {
            QuantityControlWidget->RemoveFromParent();
        }
        QuantityControlWidget = nullptr;
    }

    // Clean up radial menu widget delegate bindings
    if (RadialMenuWidget && IsValid(RadialMenuWidget) && bRadialMenuEventsBound)
    {
        RadialMenuWidget->OnMenuItemSelected.RemoveDynamic(this, &UPUIngredientSlot::OnRadialMenuItemSelected);
        RadialMenuWidget->OnMenuClosed.RemoveDynamic(this, &UPUIngredientSlot::OnRadialMenuClosed);
        bRadialMenuEventsBound = false;
    }

    // Clean up radial menu widget
    if (RadialMenuWidget)
    {
        if (RadialMenuContainer && IsValid(RadialMenuContainer))
        {
            RadialMenuContainer->RemoveChild(RadialMenuWidget);
        }
        if (IsValid(RadialMenuWidget))
        {
            RadialMenuWidget->RemoveFromParent();
        }
        RadialMenuWidget = nullptr;
    }

    // Clean up time/temperature slider delegate bindings
    if (TimeSlider && IsValid(TimeSlider) && TimeSlider->OnValueChanged.IsBound())
    {
        TimeSlider->OnValueChanged.RemoveDynamic(this, &UPUIngredientSlot::OnTimeSliderValueChanged);
    }

    if (TemperatureSlider && IsValid(TemperatureSlider) && TemperatureSlider->OnValueChanged.IsBound())
    {
        TemperatureSlider->OnValueChanged.RemoveDynamic(this, &UPUIngredientSlot::OnTemperatureSliderValueChanged);
    }

    // Clear external container reference to prevent invalid pointer access during GC
    // Note: QuantityControlContainer is a BindWidget property, so it's managed by the widget tree
    RadialMenuContainer = nullptr;

    // Clean up dynamic material instance
    SuspiciousDynamicMaterial = nullptr;

    Super::NativeDestruct();
}

void UPUIngredientSlot::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateSliderFocusVisuals();
}

void UPUIngredientSlot::SetIngredientInstance(const FIngredientInstance& InIngredientInstance)
{
    // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Setting ingredient instance (Slot: %s)"), *GetName());
    // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Input Instance ID: %d, Qty: %d, Preparations: %d"),
    //     InIngredientInstance.InstanceID, InIngredientInstance.Quantity, InIngredientInstance.Preparations.Num());

    // Store the instance data
    IngredientInstance = InIngredientInstance;
    
    // IMPORTANT: Sync preparations between Preparations and ActivePreparations
    // ActivePreparations is the source of truth (updated by ApplyPreparation)
    // If ActivePreparations has more preparations, use that (it's more up-to-date)
    if (IngredientInstance.IngredientData.ActivePreparations.Num() > IngredientInstance.Preparations.Num())
    {
        // ActivePreparations has more (or newer) preparations - sync to Preparations
        IngredientInstance.Preparations = IngredientInstance.IngredientData.ActivePreparations;
        if (bPU_LogIngredientSlotDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Synced %d ActivePreparations from IngredientData to Preparations"), 
            //    IngredientInstance.IngredientData.ActivePreparations.Num());
        }
    }
    else if (IngredientInstance.Preparations.Num() > 0 && IngredientInstance.IngredientData.ActivePreparations.Num() == 0)
    {
        // Preparations has data but ActivePreparations is empty - sync the other way
        IngredientInstance.IngredientData.ActivePreparations = IngredientInstance.Preparations;
        if (bPU_LogIngredientSlotDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Synced %d Preparations to ActivePreparations"), 
            //    IngredientInstance.Preparations.Num());
        }
    }
    else if (IngredientInstance.Preparations.Num() > 0 && IngredientInstance.IngredientData.ActivePreparations.Num() > 0)
    {
        // Both have data - merge them (ActivePreparations takes precedence)
        IngredientInstance.Preparations = IngredientInstance.IngredientData.ActivePreparations;
        if (bPU_LogIngredientSlotDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Merged preparations: ActivePreparations has %d, using that"), 
            //    IngredientInstance.IngredientData.ActivePreparations.Num());
        }
    }
    // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Final: Preparations=%d, ActivePreparations=%d"), 
    //     IngredientInstance.Preparations.Num(), IngredientInstance.IngredientData.ActivePreparations.Num());
    
    // Set plating-specific properties
    MaxQuantity = InIngredientInstance.Quantity;
    RemainingQuantity = MaxQuantity;
    
    // If quantity is 0, treat as empty (but still store ingredient data for pantry texture display)
    // Also check if InstanceID is 0 - if so, this is likely a pantry slot placeholder
    if (InIngredientInstance.Quantity <= 0 || InIngredientInstance.InstanceID == 0)
    {
        bHasIngredient = false;
        // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Quantity is 0 or InstanceID is 0, treating as empty (but storing ingredient data for display)"));
    }
    else
    {
        bHasIngredient = true;
    }

    // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Stored Ingredient: %s (ID: %d, Qty: %d, Preparations: %d, HasIngredient: %s)"),
    //     *IngredientInstance.IngredientData.DisplayName.ToString(),
    //     IngredientInstance.InstanceID,
    //     IngredientInstance.Quantity,
    //     IngredientInstance.Preparations.Num(),
    //     bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"));

    // Recalculate aspects from base + time/temp + quantity (if we have an ingredient)
    if (bHasIngredient)
    {
        RecalculateAspectsFromBase();
    }

    // Calculate and cache average color if ingredient has preparations
    if (bHasIngredient && IngredientInstance.Preparations.Num() > 0)
    {
        GetAverageColorFromIngredientTexture();
    }
    else
    {
        // Reset cached color if no preparations
        CachedAverageColor = FLinearColor::White;
    }

    // Update all display elements
    UpdateDisplay();

    // Update time/temp sliders to sync with new instance
    UpdateTimeTempSliders();
    UpdateSliderVisibility();

    // If ingredient is added to Prep area, automatically create/update prepped slot
    if (bHasIngredient && Location == EPUIngredientSlotLocation::Prep)
    {
        UPUDishCustomizationWidget* DishWidget = GetDishCustomizationWidget();
        if (DishWidget)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetIngredientInstance - Ingredient added to Prep area, creating/updating prepped slot"));
            DishWidget->CreateOrUpdatePreppedSlot(IngredientInstance);
        }
    }

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
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ClearSlot - Clearing slot: %s"), *GetName());

    // If we're in prep or active ingredient area and have an ingredient, remove the prepped slot
    if ((Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea) && bHasIngredient)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ClearSlot - Removing prepped slot for ingredient with preparations"));
        UPUDishCustomizationWidget* DishWidget = GetDishCustomizationWidget();
        if (DishWidget)
        {
            DishWidget->RemovePreppedSlot(IngredientInstance);
        }
    }

    bHasIngredient = false;
    IngredientInstance = FIngredientInstance(); // Reset to default
    CachedAverageColor = FLinearColor::White; // Reset cached color
    
    // Clean up dynamic materials
    PreparationDynamicMaterial = nullptr;

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
        // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetLocation - Location changed to: %d"), (int32)Location);

        // Update slider visibility when location changes
        UpdateSliderVisibility();

        // Immediately hide QuantityText if in prep or cooking stage (shown in plating)
    // Hide QuantityText in prep and cooking stages (shown in plating)
    if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Prepped)
        {
            if (QuantityText)
            {
                QuantityText->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
        if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Plating || Location == EPUIngredientSlotLocation::Prepped)
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
    // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateDisplay - Updating display (Slot: %s, Empty: %s, Location: %d)"),
    //     *GetName(), IsEmpty() ? TEXT("TRUE") : TEXT("FALSE"), (int32)Location);

    // For pantry, prep, and prepped slots, we want to show the texture even if "empty" (quantity 0)
    // For other locations, clear display if empty
    bool bShouldClear = IsEmpty();
    if (Location == EPUIngredientSlotLocation::Pantry || Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::Prepped)
    {
        // Pantry/Prep/Prepped slots: only clear if we don't have ingredient data at all
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
        UpdatePrepBowls();
        UpdateQuantityControl();
        UpdateSliderVisibility();
        
        // Update quantity and preparation text displays (they will hide themselves if in prep/cooking stage)
        UpdateQuantityDisplay();
        UpdatePreparationDisplay();
        
        // For prepped slots, always show hover text
        if (Location == EPUIngredientSlotLocation::Prepped)
        {
            UpdateHoverTextVisibility(true);
        }
        
        // Handle PlateBackground visibility based on location
        if (PlateBackground)
        {
            if (Location == EPUIngredientSlotLocation::Prepped || Location == EPUIngredientSlotLocation::ActiveIngredientArea)
            {
                // Hide PlateBackground for Prepped and ActiveIngredientArea locations (prep bowls are shown instead)
                PlateBackground->SetVisibility(ESlateVisibility::Collapsed);
            }
            else
            {
                // Show PlateBackground for other locations
                PlateBackground->SetVisibility(ESlateVisibility::Visible);
            }
        }
        
        // FORCE quantity control to be visible in cooking stage (ActiveIngredientArea) and prep stage after UpdateDisplay (but NOT in plating mode)
        if (Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Prep)
        {
            bool bIsPlatingMode = false;
            if (UPUDishCustomizationComponent* Component = GetDishCustomizationComponent())
            {
                bIsPlatingMode = Component->IsPlatingMode();
            }
            
            // Only show quantity controls if we're NOT in plating mode
            if (!bIsPlatingMode)
            {
                if (QuantityControlWidget)
                {
                    QuantityControlWidget->SetVisibility(ESlateVisibility::Visible);
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateDisplay - FORCED QuantityControlWidget to Visible in %s stage"), 
                    //    Location == EPUIngredientSlotLocation::Prep ? TEXT("prep") : TEXT("cooking"));
                }
                if (QuantityControlContainer)
                {
                    QuantityControlContainer->SetVisibility(ESlateVisibility::Visible);
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateDisplay - FORCED QuantityControlContainer to Visible in %s stage"), 
                    //    Location == EPUIngredientSlotLocation::Prep ? TEXT("prep") : TEXT("cooking"));
                }
            }
            else
            {
                // Hide in plating mode
                if (QuantityControlWidget)
                {
                    QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateDisplay - Hiding QuantityControlWidget (in plating mode)"));
                }
                if (QuantityControlContainer)
                {
                    QuantityControlContainer->SetVisibility(ESlateVisibility::Collapsed);
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateDisplay - Hiding QuantityControlContainer (in plating mode)"));
                }
            }
        }
    }
}

void UPUIngredientSlot::UpdateIngredientIcon()
{
    if (!IngredientIcon)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdateIngredientIcon - IngredientIcon component not found"));
        return;
    }

    // For pantry, prep, and prepped slots, show texture even if empty (to display ingredient texture)
    // For active ingredient area slots, only show if we have an ingredient
    bool bShouldShowTexture = false;
    if (Location == EPUIngredientSlotLocation::Pantry || Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::Prepped)
    {
        // Pantry/Prep/Prepped slots: show texture if we have ingredient data (even if quantity is 0)
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
        // Check if ingredient is suspicious (2+ preparations)
        TArray<FGameplayTag> PrepTags;
        IngredientInstance.Preparations.GetGameplayTagArray(PrepTags);
        bool bIsSuspicious = PrepTags.Num() >= 2;
        bool bHasPreparations = PrepTags.Num() > 0;
        // Use preparation/suspicious materials in both Prepped (prep stage bowls) and
        // ActiveIngredientArea (cooking stage) so the main ingredient icon visually reflects
        // chopped/minced/pureed/suspicious state in cooking as well.
        bool bUsePrepMaterials =
            (Location == EPUIngredientSlotLocation::Prepped ||
             Location == EPUIngredientSlotLocation::ActiveIngredientArea);
        
        // Determine which material instance to use
        UMaterialInstanceDynamic* MaterialToUse = nullptr;
        TSoftObjectPtr<UMaterialInterface> MaterialInstanceToUse;
        
        if (bUsePrepMaterials && bIsSuspicious && (SuspiciousMaterialInstance.IsValid() || !SuspiciousMaterialInstance.ToSoftObjectPath().IsNull()))
        {
            // Use suspicious material (takes priority)
            MaterialInstanceToUse = SuspiciousMaterialInstance;
            
            // Create or reuse dynamic material instance
            if (!SuspiciousDynamicMaterial)
            {
                UMaterialInterface* BaseMaterial = SuspiciousMaterialInstance.LoadSynchronous();
                if (BaseMaterial)
                {
                    SuspiciousDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateIngredientIcon - Created dynamic material instance for suspicious ingredient icon"));
                }
                else
                {
                    // Material failed to load - log warning
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateIngredientIcon - Failed to load SuspiciousMaterialInstance: %s"), 
                        *SuspiciousMaterialInstance.ToSoftObjectPath().ToString());
                }
            }
            MaterialToUse = SuspiciousDynamicMaterial;
            
            // Clean up preparation material since we're using suspicious
            if (PreparationDynamicMaterial)
            {
                PreparationDynamicMaterial = nullptr;
            }
        }
        else if (bUsePrepMaterials && bHasPreparations && (PreparationMaterialInstance.IsValid() || !PreparationMaterialInstance.ToSoftObjectPath().IsNull()))
        {
            // Use preparation material
            MaterialInstanceToUse = PreparationMaterialInstance;
            
            // Create or reuse dynamic material instance
            if (!PreparationDynamicMaterial)
            {
                UMaterialInterface* BaseMaterial = PreparationMaterialInstance.LoadSynchronous();
                if (BaseMaterial)
                {
                    PreparationDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateIngredientIcon - Created dynamic material instance for preparation-tinted ingredient icon"));
                }
                else
                {
                    // Material failed to load - log warning and try to use texture fallback
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientSlot::UpdateIngredientIcon - Failed to load PreparationMaterialInstance: %s. Using texture fallback."), 
                        *PreparationMaterialInstance.ToSoftObjectPath().ToString());
                }
            }
            MaterialToUse = PreparationDynamicMaterial;
            
            // Clean up suspicious material since we're using preparation
            if (SuspiciousDynamicMaterial)
            {
                SuspiciousDynamicMaterial = nullptr;
            }
        }
        
        // Apply material instance if we have one
        if (MaterialToUse)
        {
            // Always update material parameters to ensure they're current
            // Set texture parameter (this is the swapped texture when in prepped location)
            MaterialToUse->SetTextureParameterValue(TEXT("Texture"), TextureToUse);
            
            // Update parameters based on which material we're using
            if (bIsSuspicious && MaterialToUse == SuspiciousDynamicMaterial)
            {
                // Set color parameter using cached average color as Vector4 (same color from base ingredient)
                FVector4 ColorVector(CachedAverageColor.R, CachedAverageColor.G, CachedAverageColor.B, CachedAverageColor.A);
                MaterialToUse->SetVectorParameterValue(TEXT("Color"), ColorVector);
                
                // Set pixelate factor parameter for suspicious material (2+ preps) - use the configured value (default 10)
                MaterialToUse->SetScalarParameterValue(TEXT("PixelateFactor"), SuspiciousPixelateFactor);
                
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateIngredientIcon - Applied suspicious material to ingredient icon (Texture: %s, Color: RGB(%.3f, %.3f, %.3f), Pixelate: %.2f)"), 
                //    *TextureToUse->GetName(), CachedAverageColor.R, CachedAverageColor.G, CachedAverageColor.B, SuspiciousPixelateFactor);
            }
            else if (bHasPreparations && MaterialToUse == PreparationDynamicMaterial)
            {
                // Set color parameter using cached average color as Vector4
                FVector4 ColorVector(CachedAverageColor.R, CachedAverageColor.G, CachedAverageColor.B, CachedAverageColor.A);
                MaterialToUse->SetVectorParameterValue(TEXT("Color"), ColorVector);
                
                // Set pixelate factor to 1000 for first preparation (essentially no pixelation)
                MaterialToUse->SetScalarParameterValue(TEXT("PixelateFactor"), 1000.0f);
                
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateIngredientIcon - Applied preparation-tinted material to ingredient icon (Texture: %s, Color: RGB(%.3f, %.3f, %.3f), Pixelate: 1000.0)"), 
                //    *TextureToUse->GetName(), CachedAverageColor.R, CachedAverageColor.G, CachedAverageColor.B);
            }
            
            // Apply the material to the ingredient icon
            IngredientIcon->SetBrushFromMaterial(MaterialToUse);
        }
        else
        {
            // No material instance available, use normal texture
            IngredientIcon->SetBrushFromTexture(TextureToUse);
        }
        
        IngredientIcon->SetVisibility(ESlateVisibility::Visible);
        
        // Grey out the icon if quantity is 0, but NOT for Pantry, Prep, or Prepped locations
        if (IngredientInstance.Quantity <= 0 && Location != EPUIngredientSlotLocation::Pantry && Location != EPUIngredientSlotLocation::Prep && Location != EPUIngredientSlotLocation::Prepped)
        {
            // Grey color: 0.5, 0.5, 0.5, 1.0
            IngredientIcon->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateIngredientIcon - Set texture and GREYED OUT (quantity is 0) for location: %d (Texture: %s)"),
            //    (int32)Location, *TextureToUse->GetName());
        }
        else
        {
            // Normal white color: 1.0, 1.0, 1.0, 1.0
            IngredientIcon->SetColorAndOpacity(FLinearColor::White);
        // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateIngredientIcon - Set texture for location: %d (Texture: %s)"),
        //     (int32)Location, *TextureToUse->GetName());
        }
    }
    else
    {
        IngredientIcon->SetVisibility(ESlateVisibility::Collapsed);
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdateIngredientIcon - No texture found for ingredient: %s (Location: %d)"),
        //    *IngredientInstance.IngredientData.DisplayName.ToString(), (int32)Location);
    }
}

UTexture2D* UPUIngredientSlot::GetPreparationTexture(const FGameplayTag& PreparationTag) const
{
    if (!PreparationDataTable)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetPreparationTexture - PreparationDataTable not set"));
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
            if (Preparation->IconTexture)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetPreparationTexture - Found texture for preparation: %s"), *PrepName);
                return Preparation->IconTexture;
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetPreparationTexture - Preparation %s has no IconTexture"), *PrepName);
            }
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetPreparationTexture - Preparation not found in data table: %s"), *PrepName);
        }
    }
    
    return nullptr;
}

UTexture2D* UPUIngredientSlot::GetPreparationPrepTexture(const FGameplayTag& PreparationTag, UDataTable* PrepDataTable) const
{
    if (!PrepDataTable)
    {
        return nullptr;
    }

    // Get the preparation name from the tag (everything after the last period) and convert to lowercase
    FString PrepFullTag = PreparationTag.ToString();
    int32 PrepLastPeriodIndex;
    if (PrepFullTag.FindLastChar('.', PrepLastPeriodIndex))
    {
        FString PrepName = PrepFullTag.RightChop(PrepLastPeriodIndex + 1).ToLower();
        FName PrepRowName = FName(*PrepName);
        
        if (FPUPreparationBase* Preparation = PrepDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetPreparationPrepTexture")))
        {
            if (Preparation->PrepTexture)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetPreparationPrepTexture - Found PrepTexture for preparation: %s"), *PrepName);
                return Preparation->PrepTexture;
            }
            else
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetPreparationPrepTexture - Preparation %s has no PrepTexture"), *PrepName);
            }
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetPreparationPrepTexture - Preparation not found in data table: %s"), *PrepName);
        }
    }
    
    return nullptr;
}

void UPUIngredientSlot::UpdatePrepBowls()
{
    // Update prep bowls for Prepped and ActiveIngredientArea location slots
    if (Location != EPUIngredientSlotLocation::Prepped && Location != EPUIngredientSlotLocation::ActiveIngredientArea)
    {
        // Hide prep bowls for other locations
        if (PrepBowlFront) PrepBowlFront->SetVisibility(ESlateVisibility::Collapsed);
        if (PrepBowlBack) PrepBowlBack->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // Check if we have valid texture arrays
    if (PrepBowlFrontTextures.Num() == 0 || PrepBowlBackTextures.Num() == 0)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdatePrepBowls - Prep bowl texture arrays are empty (Front: %d, Back: %d)"),
        //    PrepBowlFrontTextures.Num(), PrepBowlBackTextures.Num());
        // Hide prep bowls if no textures available
        if (PrepBowlFront) PrepBowlFront->SetVisibility(ESlateVisibility::Collapsed);
        if (PrepBowlBack) PrepBowlBack->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // Use a deterministic random stream seeded by the ingredient instance ID so the same
    // instance always gets the same bowl selection across widgets and refreshes.
    int32 InstanceID = IngredientInstance.InstanceID;

    // If we somehow don't have a valid instance ID, fall back to zero (first textures).
    if (InstanceID == 0)
    {
        InstanceID = 1;
    }

    FRandomStream Stream(InstanceID);

    int32 FrontIndex = 0;
    int32 BackIndex = 0;

    if (bUseRandomPrepBowls)
    {
        // Random selection: any front with any back (but deterministic per instance)
        FrontIndex = Stream.RandRange(0, PrepBowlFrontTextures.Num() - 1);
        BackIndex  = Stream.RandRange(0, PrepBowlBackTextures.Num() - 1);

        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepBowls - Deterministic random selection (InstanceID: %d, Front: %d, Back: %d)"),
        //    IngredientInstance.InstanceID, FrontIndex, BackIndex);
    }
    else
    {
        // Paired selection: use same index from both arrays (deterministic per instance)
        int32 MaxIndex = FMath::Min(PrepBowlFrontTextures.Num(), PrepBowlBackTextures.Num()) - 1;
        int32 SelectedIndex = Stream.RandRange(0, MaxIndex);
        FrontIndex = BackIndex = SelectedIndex;

        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepBowls - Deterministic paired selection (InstanceID: %d, Index: %d)"),
        //    IngredientInstance.InstanceID, SelectedIndex);
    }

    UTexture2D* SelectedFrontTexture = PrepBowlFrontTextures.IsValidIndex(FrontIndex)
        ? PrepBowlFrontTextures[FrontIndex]
        : nullptr;
    UTexture2D* SelectedBackTexture = PrepBowlBackTextures.IsValidIndex(BackIndex)
        ? PrepBowlBackTextures[BackIndex]
        : nullptr;

    // Set front bowl texture and visibility
    if (PrepBowlFront)
    {
        if (SelectedFrontTexture)
        {
            PrepBowlFront->SetBrushFromTexture(SelectedFrontTexture);
            PrepBowlFront->SetVisibility(ESlateVisibility::Visible);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepBowls - Set front bowl texture: %s"),
            //    *SelectedFrontTexture->GetName());
        }
        else
        {
            PrepBowlFront->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Set back bowl texture and visibility
    if (PrepBowlBack)
    {
        if (SelectedBackTexture)
        {
            PrepBowlBack->SetBrushFromTexture(SelectedBackTexture);
            PrepBowlBack->SetVisibility(ESlateVisibility::Visible);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepBowls - Set back bowl texture: %s"),
            //    *SelectedBackTexture->GetName());
        }
        else
        {
            PrepBowlBack->SetVisibility(ESlateVisibility::Collapsed);
        }
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

    // Get preparation tags
    TArray<FGameplayTag> PrepTags;
    IngredientInstance.Preparations.GetGameplayTagArray(PrepTags);
    int32 PrepCount = PrepTags.Num();

    if (PrepCount >= 2)
    {
        // Show suspicious icon, hide individual prep icons
        if (PrepIcon1) PrepIcon1->SetVisibility(ESlateVisibility::Collapsed);
        if (PrepIcon2) PrepIcon2->SetVisibility(ESlateVisibility::Collapsed);
        if (SuspiciousIcon)
        {
            SuspiciousIcon->SetVisibility(ESlateVisibility::Visible);
            // TODO: Set suspicious icon texture when available
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepIcons - Showing suspicious icon (2+ preps)"));
        }
    }
    else if (PrepCount == 1)
    {
        // Show single prep icon
        if (SuspiciousIcon) SuspiciousIcon->SetVisibility(ESlateVisibility::Collapsed);
        if (PrepIcon2) PrepIcon2->SetVisibility(ESlateVisibility::Collapsed);

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
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdatePrepIcons - Set prep icon 1 texture: %s"), *PrepTexture->GetName());
                }
                else
                {
                    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdatePrepIcons - No texture found for first preparation: %s"), *PrepTags[0].ToString());
                }
            }
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
    // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - START (HasIngredient: %s, Widget: %s, Class: %s, Container: %s)"),
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
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Events unbound from quantity control"));
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
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - QuantityControlContainer is not set! Cannot add quantity control."));
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
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Found existing quantity control widget in container"));
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
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Created quantity control widget dynamically"));
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - Failed to create quantity control widget"));
        }
    }
    else if (!QuantityControlWidget && !QuantityControlClass)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - No quantity control widget found and QuantityControlClass is not set!"));
        //UE_LOG(LogTemp,Warning, TEXT("⚠️   Either place a WBP_IngredientQuantityControl widget in the QuantityControlContainer in Blueprint,"));
        //UE_LOG(LogTemp,Warning, TEXT("⚠️   OR set the QuantityControlClass property in the slot widget's defaults."));
        return;
    }

    // Bind to quantity control events (only once!)
    if (QuantityControlWidget && !bQuantityControlEventsBound)
    {
        QuantityControlWidget->OnQuantityControlChanged.AddDynamic(this, &UPUIngredientSlot::OnQuantityControlChanged);
        QuantityControlWidget->OnQuantityControlRemoved.AddDynamic(this, &UPUIngredientSlot::OnQuantityControlRemoved);
        bQuantityControlEventsBound = true;
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Events bound to quantity control"));
    }

    // Update quantity control with current ingredient instance
    if (QuantityControlWidget)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - QuantityControlWidget EXISTS, Location: %d (ActiveIngredientArea=%d)"),
        //    (int32)Location, (int32)EPUIngredientSlotLocation::ActiveIngredientArea);
        
        // Hide quantity control in plating mode, pantry, and prepped stages
        // Show it in cooking stage (ActiveIngredientArea) and prep stage, but NOT in plating mode
        bool bIsPlatingMode = false;
        if (UPUDishCustomizationComponent* Component = GetDishCustomizationComponent())
        {
            bIsPlatingMode = Component->IsPlatingMode();
        }
        
        if (bIsPlatingMode || 
            Location == EPUIngredientSlotLocation::Plating || 
            Location == EPUIngredientSlotLocation::Pantry ||
            Location == EPUIngredientSlotLocation::Prepped)
        {
            QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
            if (QuantityControlContainer)
            {
                QuantityControlContainer->SetVisibility(ESlateVisibility::Collapsed);
            }
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Hiding quantity control and container (PlatingMode: %s, Location: %d)"), 
            //    bIsPlatingMode ? TEXT("TRUE") : TEXT("FALSE"), (int32)Location);
            return;
        }
        
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - SHOWING quantity control (Location: %d)"), (int32)Location);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Setting ingredient instance on quantity control"));
        //UE_LOG(LogTemp,Display, TEXT("🎯   Slot Instance - ID: %d, Qty: %d, Ingredient: %s, Preparations: %d, HasIngredient: %s"),
        //    IngredientInstance.InstanceID,
        //    IngredientInstance.Quantity,
        //    *IngredientInstance.IngredientData.DisplayName.ToString(),
        //    IngredientInstance.Preparations.Num(),
        //    bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"));
        
        // Verify the instance has a valid ID before setting
        if (IngredientInstance.InstanceID == 0)
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - WARNING: IngredientInstance has InstanceID = 0! This might be a problem."));
        }
        
        QuantityControlWidget->SetIngredientInstance(IngredientInstance);
        QuantityControlWidget->SetVisibility(ESlateVisibility::Visible);
        
        // Ensure the container is also visible
        if (QuantityControlContainer)
        {
            QuantityControlContainer->SetVisibility(ESlateVisibility::Visible);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - Set QuantityControlContainer to Visible"));
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - QuantityControlContainer is NULL! Container cannot be shown."));
        }
        
        // Verify it was set correctly
        const FIngredientInstance& SetInstance = QuantityControlWidget->GetIngredientInstance();
        ESlateVisibility CurrentVisibility = QuantityControlWidget->GetVisibility();
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateQuantityControl - After setting, Quantity Control has Instance ID: %d, Qty: %d, Visibility: %d"),
        //    SetInstance.InstanceID, SetInstance.Quantity, (int32)CurrentVisibility);
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::UpdateQuantityControl - QuantityControlWidget is NULL! Cannot show quantity control."));
        //UE_LOG(LogTemp,Warning, TEXT("⚠️   QuantityControlContainer: %s, QuantityControlClass: %s"),
        //    QuantityControlContainer ? TEXT("EXISTS") : TEXT("NULL"),
        //    QuantityControlClass ? TEXT("SET") : TEXT("NULL"));
    }
}

void UPUIngredientSlot::ClearDisplay()
{
    // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ClearDisplay - Clearing display"));

    // Hide icon
    if (IngredientIcon)
    {
        IngredientIcon->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Hide prep icons
    if (PrepIcon1) PrepIcon1->SetVisibility(ESlateVisibility::Collapsed);
    if (PrepIcon2) PrepIcon2->SetVisibility(ESlateVisibility::Collapsed);
    if (SuspiciousIcon) SuspiciousIcon->SetVisibility(ESlateVisibility::Collapsed);

    // Hide prep bowls
    if (PrepBowlFront) PrepBowlFront->SetVisibility(ESlateVisibility::Collapsed);
    if (PrepBowlBack) PrepBowlBack->SetVisibility(ESlateVisibility::Collapsed);

    // Hide hover text
    if (HoverText)
    {
        HoverText->SetVisibility(ESlateVisibility::Collapsed);
    }

    // DON'T remove quantity control in cooking stage (ActiveIngredientArea) or prep stage - just hide it
    // Only remove it completely in other stages
    if (Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Prep)
    {
        // In cooking stage or prep stage, just hide it, don't remove it
        if (QuantityControlWidget)
        {
            QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ClearDisplay - Hiding quantity control in %s stage (not removing)"), 
            //    Location == EPUIngredientSlotLocation::Prep ? TEXT("prep") : TEXT("cooking"));
        }
        if (QuantityControlContainer)
        {
            QuantityControlContainer->SetVisibility(ESlateVisibility::Collapsed);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ClearDisplay - Hiding quantity control container in %s stage (not removing)"), 
            //    Location == EPUIngredientSlotLocation::Prep ? TEXT("prep") : TEXT("cooking"));
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
                    // //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ClearDisplay - Removed quantity control widget found in container"));
                    break;
                }
            }
        }
    }
}

UTexture2D* UPUIngredientSlot::GetTextureForLocation() const
{
    // For pantry, prep, and prepped slots, show texture even if bHasIngredient is false (for display purposes)
    // For other locations, require bHasIngredient to be true
    if (Location != EPUIngredientSlotLocation::Pantry && Location != EPUIngredientSlotLocation::Prep && Location != EPUIngredientSlotLocation::Prepped && !bHasIngredient)
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
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetTextureForLocation - Pantry slot using PreviewTexture as fallback for ingredient: %s"),
                //    *IngredientInstance.IngredientData.DisplayName.ToString());
            }
        }
        if (!Texture)
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetTextureForLocation - Pantry slot has no PantryTexture or PreviewTexture for ingredient: %s"),
            //    *IngredientInstance.IngredientData.DisplayName.ToString());
        }
        return Texture;
    }
    else if (Location == EPUIngredientSlotLocation::Prep)
    {
        // Prep slots use PreviewTexture (or could use a specific prep texture in the future)
        UTexture2D* Texture = IngredientInstance.IngredientData.PreviewTexture;
        if (!Texture)
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetTextureForLocation - Prep slot has no PreviewTexture for ingredient: %s"),
            //    *IngredientInstance.IngredientData.DisplayName.ToString());
        }
        return Texture;
    }
    else if (Location == EPUIngredientSlotLocation::Prepped || Location == EPUIngredientSlotLocation::ActiveIngredientArea)
    {
        // Prepped slots (and cooking stage ActiveIngredientArea slots) should use the
        // preparation's PrepTexture if a preparation is applied so the main ingredient
        // icon shows the chopped/minced/pureed version.
        // Fallback chain: PrepTexture -> PreppedTexture -> PreviewTexture
        UTexture2D* Texture = nullptr;
        
        // First, check if the ingredient has any preparations applied
        if (IngredientInstance.Preparations.Num() > 0)
        {
            // Get the preparation data table
            UDataTable* PrepDataTable = nullptr;
            
            // Try to get from the ingredient's data table first
            if (IngredientInstance.IngredientData.PreparationDataTable.IsValid())
            {
                PrepDataTable = IngredientInstance.IngredientData.PreparationDataTable.LoadSynchronous();
            }
            
            // Fallback to the slot's preparation data table if available
            if (!PrepDataTable && PreparationDataTable)
            {
                PrepDataTable = PreparationDataTable;
            }
            
            if (PrepDataTable)
            {
                // Get the first preparation's PrepTexture (you could extend this to handle multiple preparations)
                TArray<FGameplayTag> PreparationTags;
                IngredientInstance.Preparations.GetGameplayTagArray(PreparationTags);
                
                if (PreparationTags.Num() > 0)
                {
                    Texture = GetPreparationPrepTexture(PreparationTags[0], PrepDataTable);
                    if (Texture)
                    {
                        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetTextureForLocation - Prepped slot using PrepTexture from preparation: %s"),
                        //    *PreparationTags[0].ToString());
                    }
                }
            }
        }
        
        // Fallback to PreppedTexture if no preparation texture was found
        if (!Texture)
        {
            Texture = IngredientInstance.IngredientData.PreppedTexture;
        }
        
        // Fallback to PreviewTexture if still no texture
        if (!Texture)
        {
            Texture = IngredientInstance.IngredientData.PreviewTexture;
            if (Texture)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetTextureForLocation - Prepped slot using PreviewTexture as fallback for ingredient: %s"),
                //    *IngredientInstance.IngredientData.DisplayName.ToString());
            }
        }
        
        if (!Texture)
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetTextureForLocation - Prepped slot has no PrepTexture, PreppedTexture, or PreviewTexture for ingredient: %s"),
            //    *IngredientInstance.IngredientData.DisplayName.ToString());
        }
        
        return Texture;
    }
    else // Plating
    {
        return IngredientInstance.IngredientData.PreviewTexture;
    }
}

void UPUIngredientSlot::GetAverageColorFromIngredientTexture()
{
    if (!bHasIngredient)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetAverageColorFromIngredientTexture - No ingredient in slot"));
        return;
    }

    // Get the ORIGINAL ingredient texture (NOT the swapped prep texture)
    // Use PreppedTexture if available, otherwise fallback to PreviewTexture
    UTexture2D* Texture = IngredientInstance.IngredientData.PreppedTexture;
    if (!Texture)
    {
        Texture = IngredientInstance.IngredientData.PreviewTexture;
    }
    
    if (!Texture)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetAverageColorFromIngredientTexture - No original ingredient texture found (PreppedTexture or PreviewTexture) for ingredient: %s"),
        //    *IngredientInstance.IngredientData.DisplayName.ToString());
        return;
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎨 UPUIngredientSlot::GetAverageColorFromIngredientTexture - Using ORIGINAL ingredient texture: %s (not prep texture)"),
    //    *Texture->GetName());

    // Get texture dimensions
    const int32 Width = Texture->GetSizeX();
    const int32 Height = Texture->GetSizeY();
    
    if (Width <= 0 || Height <= 0)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetAverageColorFromIngredientTexture - Invalid texture dimensions: %dx%d"),
        //    Width, Height);
        return;
    }

    // Get the source image from the texture using FImageUtils
    FImage SourceImage;
    if (!FImageUtils::GetTexture2DSourceImage(Texture, SourceImage))
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetAverageColorFromIngredientTexture - Failed to get source image from texture: %s"),
        //    *Texture->GetName());
        return;
    }

    // Calculate average color
    int64 TotalR = 0;
    int64 TotalG = 0;
    int64 TotalB = 0;
    int32 PixelCount = 0;

    // Access the raw data from FImage
    // FImage stores data in RawData array, format depends on the image format
    // Typically RGBA or RGB format
    const TArray64<uint8>& RawData = SourceImage.RawData;
    const int32 DataSize = RawData.Num();
    
    // Assume RGBA format (4 bytes per pixel) - adjust if needed
    const int32 BytesPerPixel = 4;
    const int32 ExpectedDataSize = Width * Height * BytesPerPixel;
    
    if (DataSize < ExpectedDataSize)
    {
        // Try RGB format (3 bytes per pixel)
        const int32 BytesPerPixelRGB = 3;
        const int32 ExpectedDataSizeRGB = Width * Height * BytesPerPixelRGB;
        
        if (DataSize >= ExpectedDataSizeRGB)
        {
            // RGB format
            for (int32 Y = 0; Y < Height; Y++)
            {
                for (int32 X = 0; X < Width; X++)
                {
                    const int32 PixelIndex = (Y * Width + X) * BytesPerPixelRGB;
                    
                    if (PixelIndex + 2 < DataSize)
                    {
                        // Unreal textures are typically BGRA format, so swap R and B
                        uint8 B = RawData[PixelIndex];
                        uint8 G = RawData[PixelIndex + 1];
                        uint8 R = RawData[PixelIndex + 2];

                        TotalR += R;
                        TotalG += G;
                        TotalB += B;
                        PixelCount++;
                    }
                }
            }
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetAverageColorFromIngredientTexture - Unexpected data size: %d (expected at least %d or %d)"),
            //    DataSize, ExpectedDataSize, ExpectedDataSizeRGB);
            return;
        }
    }
    else
    {
        // RGBA format
        for (int32 Y = 0; Y < Height; Y++)
        {
            for (int32 X = 0; X < Width; X++)
            {
                const int32 PixelIndex = (Y * Width + X) * BytesPerPixel;
                
                if (PixelIndex + 2 < DataSize)
                {
                    // Unreal textures are typically BGRA format, so swap R and B
                    uint8 B = RawData[PixelIndex];
                    uint8 G = RawData[PixelIndex + 1];
                    uint8 R = RawData[PixelIndex + 2];

                    TotalR += R;
                    TotalG += G;
                    TotalB += B;
                    PixelCount++;
                }
            }
        }
    }

    if (PixelCount > 0)
    {
        uint8 AvgR = static_cast<uint8>(TotalR / PixelCount);
        uint8 AvgG = static_cast<uint8>(TotalG / PixelCount);
        uint8 AvgB = static_cast<uint8>(TotalB / PixelCount);

        // Cache the average color (convert from 0-255 range to 0-1 range for FLinearColor)
        FLinearColor BaseColor = FLinearColor(
            AvgR / 255.0f,
            AvgG / 255.0f,
            AvgB / 255.0f,
            1.0f
        );

        // Check if ingredient is suspicious (2+ preparations) - if so, skip saturation boost
        TArray<FGameplayTag> PrepTags;
        IngredientInstance.Preparations.GetGameplayTagArray(PrepTags);
        bool bIsSuspicious = PrepTags.Num() >= 2;

        // Boost saturation to make colors more vibrant (but not for suspicious ingredients)
        if (bIsSuspicious)
        {
            // Use base color without saturation boost for suspicious ingredients
            CachedAverageColor = BaseColor;
            //UE_LOG(LogTemp,Display, TEXT("🎨 UPUIngredientSlot::GetAverageColorFromIngredientTexture - Average color: RGB(%d, %d, %d) -> Base(%.3f, %.3f, %.3f) [SUSPICIOUS - No saturation boost] from texture: %s (Size: %dx%d, Pixels: %d)"),
            //    AvgR, AvgG, AvgB, 
            //    BaseColor.R, BaseColor.G, BaseColor.B,
            //    *Texture->GetName(), Width, Height, PixelCount);
        }
        else
        {
            // Boost saturation for non-suspicious ingredients
            CachedAverageColor = BoostColorSaturation(BaseColor, ColorSaturationMultiplier);
            //UE_LOG(LogTemp,Display, TEXT("🎨 UPUIngredientSlot::GetAverageColorFromIngredientTexture - Average color: RGB(%d, %d, %d) -> Base(%.3f, %.3f, %.3f) -> Boosted(%.3f, %.3f, %.3f) from texture: %s (Size: %dx%d, Pixels: %d, Saturation: %.2fx)"),
            //    AvgR, AvgG, AvgB, 
            //    BaseColor.R, BaseColor.G, BaseColor.B,
            //    CachedAverageColor.R, CachedAverageColor.G, CachedAverageColor.B,
            //    *Texture->GetName(), Width, Height, PixelCount, ColorSaturationMultiplier);
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetAverageColorFromIngredientTexture - No pixels processed"));
    }

}

FLinearColor UPUIngredientSlot::BoostColorSaturation(const FLinearColor& Color, float SaturationMultiplier) const
{
    // Clamp multiplier to valid range
    float Multiplier = FMath::Clamp(SaturationMultiplier, 1.0f, 3.0f);
    
    // If multiplier is 1.0, no change needed
    if (FMath::IsNearlyEqual(Multiplier, 1.0f))
    {
        return Color;
    }

    // Convert RGB to HSV
    // FLinearColor uses RGB, we need to manually convert to HSV
    float R = Color.R;
    float G = Color.G;
    float B = Color.B;
    
    float Max = FMath::Max3(R, G, B);
    float Min = FMath::Min3(R, G, B);
    float Delta = Max - Min;
    
    float H = 0.0f;
    float S = (Max > 0.0f) ? (Delta / Max) : 0.0f;
    float V = Max;
    
    // Calculate Hue
    if (Delta > 0.0f)
    {
        if (FMath::IsNearlyEqual(Max, R))
        {
            H = 60.0f * FMath::Fmod(((G - B) / Delta), 6.0f);
        }
        else if (FMath::IsNearlyEqual(Max, G))
        {
            H = 60.0f * (((B - R) / Delta) + 2.0f);
        }
        else // Max == B
        {
            H = 60.0f * (((R - G) / Delta) + 4.0f);
        }
        
        if (H < 0.0f)
        {
            H += 360.0f;
        }
    }
    
    // Boost saturation
    S = FMath::Clamp(S * Multiplier, 0.0f, 1.0f);
    
    // Convert HSV back to RGB
    float C = V * S;
    float X = C * (1.0f - FMath::Abs(FMath::Fmod(H / 60.0f, 2.0f) - 1.0f));
    float m = V - C;
    
    float NewR = 0.0f, NewG = 0.0f, NewB = 0.0f;
    
    if (H < 60.0f)
    {
        NewR = C; NewG = X; NewB = 0.0f;
    }
    else if (H < 120.0f)
    {
        NewR = X; NewG = C; NewB = 0.0f;
    }
    else if (H < 180.0f)
    {
        NewR = 0.0f; NewG = C; NewB = X;
    }
    else if (H < 240.0f)
    {
        NewR = 0.0f; NewG = X; NewB = C;
    }
    else if (H < 300.0f)
    {
        NewR = X; NewG = 0.0f; NewB = C;
    }
    else // H < 360.0f
    {
        NewR = C; NewG = 0.0f; NewB = X;
    }
    
    // Add the lightness component
    NewR = FMath::Clamp(NewR + m, 0.0f, 1.0f);
    NewG = FMath::Clamp(NewG + m, 0.0f, 1.0f);
    NewB = FMath::Clamp(NewB + m, 0.0f, 1.0f);
    
    return FLinearColor(NewR, NewG, NewB, Color.A);
}

bool UPUIngredientSlot::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UPUIngredientDragDropOperation* IngredientDragOp = Cast<UPUIngredientDragDropOperation>(InOperation);
    if (IngredientDragOp)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDragOver - Drag over slot: %s (Location: %d, Empty: %s)"),
        //    *GetName(), (int32)Location, IsEmpty() ? TEXT("TRUE") : TEXT("FALSE"));

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
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Drop on slot: %s (Ingredient: %s, Location: %d, Empty: %s)"),
        //    *GetName(), *IngredientDragOp->IngredientInstance.IngredientData.DisplayName.ToString(), (int32)Location, IsEmpty() ? TEXT("TRUE") : TEXT("FALSE"));

        // In cooking stage (ActiveIngredientArea) or prep stage (Prep), handle both empty slots (move) and occupied slots (swap)
        if (Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Prep)
        {
            // SAFETY CHECK: If InstanceID is 0, this is from pantry - generate new GUID and set quantity to 1
            if (IngredientDragOp->IngredientInstance.InstanceID == 0)
            {
                //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::NativeOnDrop - Detected pantry drag (ID: 0), generating new GUID and setting quantity to 1"));
                IngredientDragOp->IngredientInstance.InstanceID = GenerateGUIDBasedInstanceID();
                IngredientDragOp->IngredientInstance.Quantity = 1;
                // Ensure IngredientTag is set (should be from IngredientData, but set it explicitly for consistency)
                if (!IngredientDragOp->IngredientInstance.IngredientTag.IsValid() && IngredientDragOp->IngredientInstance.IngredientData.IngredientTag.IsValid())
                {
                    IngredientDragOp->IngredientInstance.IngredientTag = IngredientDragOp->IngredientInstance.IngredientData.IngredientTag;
                }
                if (bPU_LogIngredientSlotDebug)
                {
                    //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::NativeOnDrop - Generated new InstanceID: %d, Quantity: 1, Tag: %s"), 
                    //    IngredientDragOp->IngredientInstance.InstanceID, *IngredientDragOp->IngredientInstance.IngredientTag.ToString());
                }
            }
            
            // Store the target slot's ingredient instance if it exists (for swapping)
            FIngredientInstance TargetSlotIngredient = IngredientInstance;
            bool bTargetSlotHasIngredient = !IsEmpty();
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Cooking stage drop (Target empty: %s)"), 
            //    bTargetSlotHasIngredient ? TEXT("FALSE") : TEXT("TRUE"));
            
            if (bTargetSlotHasIngredient)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯   Target slot has ingredient - ID: %d, Qty: %d, Ingredient: %s"),
                //    TargetSlotIngredient.InstanceID,
                //    TargetSlotIngredient.Quantity,
                //    *TargetSlotIngredient.IngredientData.DisplayName.ToString());
            }
            
            //UE_LOG(LogTemp,Display, TEXT("🎯   Drag Operation IngredientInstance - ID: %d, Qty: %d, Ingredient: %s, Preparations: %d"),
            //    IngredientDragOp->IngredientInstance.InstanceID,
            //    IngredientDragOp->IngredientInstance.Quantity,
            //    *IngredientDragOp->IngredientInstance.IngredientData.DisplayName.ToString(),
            //    IngredientDragOp->IngredientInstance.Preparations.Num());

            // Set the target slot with the dragged ingredient
            SetIngredientInstance(IngredientDragOp->IngredientInstance);
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - After SetIngredientInstance, slot has - ID: %d, Qty: %d, HasIngredient: %s, Location: %d"),
            //    IngredientInstance.InstanceID,
            //    IngredientInstance.Quantity,
            //    bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"),
            //    (int32)Location);
            
            // FORCE update and show quantity control after drop
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - FORCING quantity control to be visible after drop"));
            UpdateQuantityControl();
            
            // Double-check visibility is set correctly
            if (QuantityControlWidget)
            {
                QuantityControlWidget->SetVisibility(ESlateVisibility::Visible);
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Explicitly set QuantityControlWidget to Visible"));
            }
            if (QuantityControlContainer)
            {
                QuantityControlContainer->SetVisibility(ESlateVisibility::Visible);
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Explicitly set QuantityControlContainer to Visible"));
            }

            // Find the source slot (the slot we dragged from) in the cooking stage
            // If target slot was empty: clear the source slot (move)
            // If target slot had an ingredient: swap by setting source slot with target's ingredient (swap)
            // Try to find the parent widget (PUDishCustomizationWidget)
            UUserWidget* ParentWidget = GetTypedOuter<UUserWidget>();
            TArray<UPUIngredientSlot*> AllSlots;
            
            // Try PUDishCustomizationWidget first
            if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(ParentWidget))
            {
                AllSlots = DishWidget->GetCreatedIngredientSlots();
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Found PUDishCustomizationWidget, got %d slots"), AllSlots.Num());
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
                        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Found PUDishCustomizationWidget via hierarchy, got %d slots"), AllSlots.Num());
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
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Searching %d slots for source slot with InstanceID: %d"), 
            //    AllSlots.Num(), IngredientDragOp->IngredientInstance.InstanceID);
            
            bool bFoundSourceSlot = false;
            for (UPUIngredientSlot* SourceSlot : AllSlots)
            {
                if (SourceSlot && SourceSlot != this)
                {
                    const FIngredientInstance& SourceInstance = SourceSlot->GetIngredientInstance();
                    //UE_LOG(LogTemp,Display, TEXT("🎯   Checking slot: %s (Location: %d, InstanceID: %d, HasIngredient: %s)"),
                    //    *SourceSlot->GetName(),
                    //    (int32)SourceSlot->GetLocation(),
                    //    SourceInstance.InstanceID,
                    //    SourceSlot->IsEmpty() ? TEXT("FALSE") : TEXT("TRUE"));
                    
                    if ((SourceSlot->GetLocation() == EPUIngredientSlotLocation::ActiveIngredientArea || 
                         SourceSlot->GetLocation() == EPUIngredientSlotLocation::Prep) &&
                        SourceInstance.InstanceID == IngredientDragOp->IngredientInstance.InstanceID &&
                        !SourceSlot->IsEmpty())
                    {
                        if (bTargetSlotHasIngredient)
                        {
                            // SWAP: Set source slot with target slot's ingredient
                            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - SWAPPING: Setting source slot %s with target slot's ingredient (ID: %d)"), 
                            //    *SourceSlot->GetName(), TargetSlotIngredient.InstanceID);
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
                            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - MOVING: Clearing source slot: %s"), *SourceSlot->GetName());
                            SourceSlot->ClearSlot();
                        }
                        bFoundSourceSlot = true;
                        break;
                    }
                }
            }
            
            if (!bFoundSourceSlot)
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::NativeOnDrop - Could not find source slot with InstanceID: %d!"), 
                //    IngredientDragOp->IngredientInstance.InstanceID);
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
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDrop - Ensuring quantity control stays hidden in plating stage after drop"));
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
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDragEnter - Drag entered slot: %s"),
        //    *GetName());
        OnDragOverSlot();
    }
}

void UPUIngredientSlot::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UPUIngredientDragDropOperation* IngredientDragOp = Cast<UPUIngredientDragDropOperation>(InOperation);
    if (IngredientDragOp)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDragLeave - Drag left slot: %s"),
        //    *GetName());
        OnDragLeaveSlot();
    }
    
    // Ensure quantity control stays hidden in plating stage after drag ends
    if (Location == EPUIngredientSlotLocation::Plating && QuantityControlWidget)
    {
        QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnDragLeave - Ensuring quantity control stays hidden in plating stage"));
    }
}

FReply UPUIngredientSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Mouse button down on slot: %s (Button: %s, DragEnabled: %s, Location: %d)"),
    //    *GetName(), *InMouseEvent.GetEffectingButton().ToString(), bDragEnabled ? TEXT("TRUE") : TEXT("FALSE"), (int32)Location);

    // Special handling for pantry slots - clicking should select the ingredient (not drag)
    if (Location == EPUIngredientSlotLocation::Pantry && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Pantry slots: if we have valid ingredient data, treat click as selection (not drag)
        if (IngredientInstance.IngredientData.IngredientTag.IsValid())
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Pantry slot clicked, triggering OnEmptySlotClicked for selection"));
            // Use OnEmptySlotClicked event for pantry slot selection (bound to OnPantrySlotClicked in cooking stage)
            OnEmptySlotClicked.Broadcast(this);
            return FReply::Handled();
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::NativeOnMouseButtonDown - Pantry slot clicked but no valid ingredient tag"));
        }
    }

    // If drag is enabled and we have an ingredient, don't handle left clicks here
    // Let the drag system handle it (NativeOnPreviewMouseButtonDown will handle drag)
    // BUT: Don't do this for pantry slots - we want clicks to work for selection
    // AND: Don't do this for prep slots - we want clicks to work for menu access
    // AND: Don't do this if Shift is pressed (Shift+Click shows menu instead)
    bool bShiftPressed = InMouseEvent.IsShiftDown();
    if (Location != EPUIngredientSlotLocation::Pantry && Location != EPUIngredientSlotLocation::Prep && 
        bDragEnabled && bHasIngredient && 
        InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !bShiftPressed)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Drag enabled, letting drag system handle left click"));
        return FReply::Unhandled(); // Let drag system handle it
    }

    if (IsEmpty())
    {
        // Empty slot clicked - open pantry (only for non-pantry locations)
        if (Location != EPUIngredientSlotLocation::Pantry)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Empty slot clicked, opening pantry"));
            OnEmptySlotClicked.Broadcast(this);
            return FReply::Handled();
        }
    }

    // Slot has ingredient - handle left/right click
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Show prep menu if:
        // 1. This is a Prep slot (Prep slots always show menu on click, regardless of drag), OR
        // 2. Shift is held down AND drag is enabled (allows menu access even when drag is enabled)
        //
        // IMPORTANT:
        // - For non-Prep slots created via CreateSlotsFromDishData with bEnableDrag = false,
        //   we SHOULD NOT show the radial menu on simple left-click. In that case bDragEnabled
        //   is false and Location != Prep, so the menu will not open.
        bool bShouldShowMenu =
            (Location == EPUIngredientSlotLocation::Prep) ||
            (bShiftPressed && bDragEnabled);
        if (bShouldShowMenu)
        {
            if (bShiftPressed)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Shift+Left click, showing prep radial menu (drag enabled but using modifier)"));
            }
            else if (Location == EPUIngredientSlotLocation::Prep)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Left click on Prep slot, showing prep radial menu"));
            }
            else
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Left click (drag disabled), showing prep radial menu"));
            }
            ShowRadialMenu(true);
            return FReply::Handled();
        }
        // Otherwise, let drag system handle it (if drag is enabled)
    }
    else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        // Right click - show combined menu (preparations + actions)
        // Only allow right-click radial menu in Prep slots (plate area).
        // Disable in all other locations, including ActiveIngredientArea and Prepped (bowls).
        if (Location != EPUIngredientSlotLocation::Prep)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Right click radial menu disabled for this slot location: %d"), (int32)Location);
            return FReply::Unhandled();
        }
        
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseButtonDown - Right click, showing combined radial menu"));
        ShowRadialMenu(true, true); // bIsPrepMenu=true, bIncludeActions=true
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

void UPUIngredientSlot::ShowRadialMenu(bool bIsPrepMenu, bool bIncludeActions)
{
    // Radial menus are only allowed in Prep slots (plate area).
    if (Location != EPUIngredientSlotLocation::Prep)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Blocked radial menu for non-Prep slot (Location: %d)"), (int32)Location);
        return;
    }

    if (IsEmpty())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - Cannot show menu on empty slot"));
        return;
    }
    
    // If a radial menu is already visible, don't open another one
    if (bRadialMenuVisible && RadialMenuWidget && RadialMenuWidget->IsMenuVisible())
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::ShowRadialMenu - Radial menu already visible, ignoring request"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Showing radial menu (Prep: %s, IncludeActions: %s)"),
    //    bIsPrepMenu ? TEXT("TRUE") : TEXT("FALSE"), bIncludeActions ? TEXT("TRUE") : TEXT("FALSE"));

    if (!RadialMenuWidgetClass)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - RadialMenuWidgetClass not set"));
        return;
    }

    // Create the menu widget if it doesn't exist
    if (!RadialMenuWidget)
    {
        RadialMenuWidget = CreateWidget<UPURadialMenu>(GetWorld(), RadialMenuWidgetClass);
        if (!RadialMenuWidget)
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUIngredientSlot::ShowRadialMenu - Failed to create radial menu widget"));
            return;
        }

        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Radial menu widget created"));
    }

    // Bind to menu events only once to avoid duplicate callbacks
    if (!bRadialMenuEventsBound && RadialMenuWidget)
    {
        RadialMenuWidget->OnMenuItemSelected.AddUniqueDynamic(this, &UPUIngredientSlot::OnRadialMenuItemSelected);
        RadialMenuWidget->OnMenuClosed.AddUniqueDynamic(this, &UPUIngredientSlot::OnRadialMenuClosed);
        bRadialMenuEventsBound = true;
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Bound radial menu events"));
    }

    // Get the slot's screen position
    FVector2D SlotScreenPosition = GetSlotScreenPosition();
    
    // Set preparation data table on radial menu widget if we have one (for prep menu or combined menu)
    // This allows the radial menu to have its own data table reference that can be set in Blueprint
    // If the radial menu doesn't have one set, use the slot's preparation data table
    if ((bIsPrepMenu || bIncludeActions) && RadialMenuWidget)
    {
        // Only set if radial menu doesn't already have one (allows Blueprint override)
        if (!RadialMenuWidget->GetPreparationDataTable() && PreparationDataTable)
        {
            RadialMenuWidget->SetPreparationDataTable(PreparationDataTable);
        }
    }
    
    // Build menu items - combine prep and actions if requested
    TArray<FRadialMenuItem> MenuItems;
    
    if (bIsPrepMenu)
    {
        MenuItems = BuildPreparationMenuItems();
    }
    
    if (bIncludeActions)
    {
        TArray<FRadialMenuItem> ActionItems = BuildActionMenuItems();
        MenuItems.Append(ActionItems);
    }
    
    // If neither prep nor actions, fall back to just prep (for backward compatibility)
    if (!bIsPrepMenu && !bIncludeActions)
    {
        MenuItems = BuildActionMenuItems();
    }
    
    if (MenuItems.Num() == 0)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - No menu items to display"));
        return;
    }

    // Add to container if available, otherwise add to viewport (do this BEFORE setting menu items)
    if (RadialMenuContainer && IsValid(RadialMenuContainer) && RadialMenuWidget && IsValid(RadialMenuWidget))
    {
        // Add to container first (if not already added)
        if (!RadialMenuWidget->GetParent())
        {
            RadialMenuContainer->AddChild(RadialMenuWidget);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Added menu to container: %s"), *RadialMenuContainer->GetName());
        }
        
        // Get container size - use multiple methods to ensure we get valid size
        // Re-check validity before each access since GC might invalidate it
        if (IsValid(RadialMenuContainer))
        {
            FVector2D ContainerSize = RadialMenuContainer->GetDesiredSize();
            if (ContainerSize.X == 0 || ContainerSize.Y == 0)
            {
                if (IsValid(RadialMenuContainer))
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
            }
            
            // If we have container size, use container center directly (more reliable than converting screen position)
            if (ContainerSize.X > 0 && ContainerSize.Y > 0)
            {
                FVector2D ContainerCenter = ContainerSize * 0.5f;
                
                // Set menu items and show at container center
                if (RadialMenuWidget && IsValid(RadialMenuWidget))
                {
                    RadialMenuWidget->SetMenuItems(MenuItems);
                    RadialMenuWidget->ShowMenuAtPosition(ContainerCenter);
                }
                
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Using container center: (%.2f, %.2f)"), 
                //    ContainerCenter.X, ContainerCenter.Y);
            }
            else if (IsValid(RadialMenuContainer))
            {
                // Fallback: try to convert screen position to container-relative
                FGeometry ContainerGeometry = RadialMenuContainer->GetCachedGeometry();
                FVector2D ContainerScreenPosition = ContainerGeometry.GetAbsolutePosition();
                FVector2D ContainerRelativePosition = SlotScreenPosition - ContainerScreenPosition;
                
                // Set menu items and show
                if (RadialMenuWidget && IsValid(RadialMenuWidget))
                {
                    RadialMenuWidget->SetMenuItems(MenuItems);
                    RadialMenuWidget->ShowMenuAtPosition(ContainerRelativePosition);
                }
                
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - Container size not available, using converted position"));
            }
        }
    }
    else
    {
        // Fallback to viewport if no container
        // Set menu items and show
        if (RadialMenuWidget && IsValid(RadialMenuWidget))
        {
            RadialMenuWidget->SetMenuItems(MenuItems);
            RadialMenuWidget->ShowMenuAtPosition(SlotScreenPosition);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - No container set, added to viewport"));
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ShowRadialMenu - RadialMenuWidget is invalid!"));
        }
    }
    
    bRadialMenuVisible = true;
    
    // Set focus to the radial menu widget so it can receive controller input
    if (RadialMenuWidget && IsValid(RadialMenuWidget))
    {
        // Use a small delay to ensure the widget is fully visible before setting focus
        if (UWorld* World = GetWorld())
        {
            FTimerHandle FocusTimerHandle;
            World->GetTimerManager().SetTimer(FocusTimerHandle, [WeakMenu = TWeakObjectPtr<UPURadialMenu>(RadialMenuWidget)]()
            {
                if (WeakMenu.IsValid())
                {
                    UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::ShowRadialMenu - Setting focus to radial menu: %s"), 
                        *WeakMenu->GetName());
                    
                    // Make the menu widget focusable and set focus
                    WeakMenu->SetIsFocusable(true);
                    WeakMenu->SetKeyboardFocus();
                    FSlateApplication::Get().SetUserFocus(0, WeakMenu->TakeWidget());
                    
                    // Retry if focus wasn't set
                    if (!WeakMenu->HasKeyboardFocus())
                    {
                        FTimerHandle RetryTimerHandle;
                        if (UWorld* World = WeakMenu->GetWorld())
                        {
                            World->GetTimerManager().SetTimer(RetryTimerHandle, [WeakMenu]()
                            {
                                if (WeakMenu.IsValid())
                                {
                                    WeakMenu->SetKeyboardFocus();
                                    UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::ShowRadialMenu - Retry: Focus set to radial menu (HasFocus: %s)"), 
                                        WeakMenu->HasKeyboardFocus() ? TEXT("YES") : TEXT("NO"));
                                }
                            }, 0.1f, false);
                        }
                    }
                }
            }, 0.1f, false);
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ShowRadialMenu - Radial menu shown with %d items"), MenuItems.Num());
}

void UPUIngredientSlot::HideRadialMenu()
{
    if (RadialMenuWidget)
    {
        RadialMenuWidget->HideMenu();
        bRadialMenuVisible = false;
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::HideRadialMenu - Radial menu hidden"));
    }
}

void UPUIngredientSlot::SetRadialMenuContainer(UPanelWidget* InContainer)
{
    RadialMenuContainer = InContainer;
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetRadialMenuContainer - Set radial menu container: %s"), 
    //    InContainer ? *InContainer->GetName() : TEXT("NULL"));
}

void UPUIngredientSlot::OnQuantityControlChanged(const FIngredientInstance& InIngredientInstance)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::OnQuantityControlChanged - Quantity control changed (ID: %d, Qty: %d)"),
    //    InIngredientInstance.InstanceID, InIngredientInstance.Quantity);

    // Update instance if IDs match
    if (bHasIngredient && IngredientInstance.InstanceID == InIngredientInstance.InstanceID)
    {
        IngredientInstance = InIngredientInstance;

        // Recalculate aspects from base + time/temp + quantity
        RecalculateAspectsFromBase();

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
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::OnQuantityControlRemoved - Quantity control removed (ID: %d)"),
    //    InstanceID);

    // Clear the slot
    ClearSlot();
}

void UPUIngredientSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseEnter - Mouse entered slot: %s"),
    //    *GetName());

    // Show hover text (works for prep/pantry slots even when "empty")
    UpdateHoverTextVisibility(true);

    bIsHovered = true;
    UpdateIngredientSelectVisibility(true);
}

void UPUIngredientSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnMouseLeave - Mouse left slot: %s"),
    //    *GetName());

    // Hide hover text (but not for prepped slots - they should always show hover text)
    if (Location != EPUIngredientSlotLocation::Prepped)
    {
        UpdateHoverTextVisibility(false);
    }

    bIsHovered = false;
    UpdateIngredientSelectVisibility(HasKeyboardFocus());
}

void UPUIngredientSlot::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
    Super::NativeOnAddedToFocusPath(InFocusEvent);

    // Show hover text (works for prep/pantry slots even when "empty")
    UpdateHoverTextVisibility(true);

    UpdateIngredientSelectVisibility(true);
}

void UPUIngredientSlot::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
    Super::NativeOnRemovedFromFocusPath(InFocusEvent);

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnRemovedFromFocusPath - Focus removed from slot: %s"),
    //    *GetName());

    // Hide hover text
    UpdateHoverTextVisibility(false);

    UpdateIngredientSelectVisibility(bIsHovered);
}

FText UPUIngredientSlot::GetIngredientDisplayText() const
{
    // For prep/pantry/prepped slots, show text even if "empty" (quantity 0) as long as we have ingredient data
    // For other locations, require bHasIngredient to be true
    bool bShouldShowText = false;
    if (Location == EPUIngredientSlotLocation::Pantry || Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::Prepped)
    {
        // Prep/Pantry/Prepped slots: show text if we have ingredient data (even if quantity is 0)
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

    // For prep area slots, show only the base ingredient name (no preparations)
    // For other areas (prepped, pantry, etc.), show the full name with preparations
    if (Location == EPUIngredientSlotLocation::Prep)
    {
        // Just return the base ingredient name without preparations
        FText Result = IngredientInstance.IngredientData.DisplayName;
        //UE_LOG(LogTemp,Display, TEXT("🎯   Prep area - returning base name: %s"), *Result.ToString());
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetIngredientDisplayText - END"));
        return Result;
    }

    // Debug: Log what preparations we have
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetIngredientDisplayText - START"));
    //UE_LOG(LogTemp,Display, TEXT("🎯   Ingredient: %s"), *IngredientInstance.IngredientData.DisplayName.ToString());
    //UE_LOG(LogTemp,Display, TEXT("🎯   Preparations count: %d"), IngredientInstance.Preparations.Num());
    
    TArray<FGameplayTag> PrepTags;
    IngredientInstance.Preparations.GetGameplayTagArray(PrepTags);
    for (const FGameplayTag& PrepTag : PrepTags)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯   Preparation tag: %s"), *PrepTag.ToString());
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯   PreparationDataTable set: %s"), PreparationDataTable ? TEXT("YES") : TEXT("NO"));

    // Get preparation data table from slot property
    FPUIngredientBase IngredientDataCopy = IngredientInstance.IngredientData;
    
    // If we have a preparation data table set on the slot, use it
    if (PreparationDataTable)
    {
        // Set the preparation data table on the ingredient data copy
        IngredientDataCopy.PreparationDataTable = PreparationDataTable;
        //UE_LOG(LogTemp,Display, TEXT("🎯   Set PreparationDataTable on ingredient data copy"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️   PreparationDataTable not set on slot!"));
    }

    // Sync Preparations with ActivePreparations before calling GetCurrentDisplayName()
    // GetCurrentDisplayName() uses ActivePreparations to look up preparations from the data table
    IngredientDataCopy.ActivePreparations = IngredientInstance.Preparations;
    
    //UE_LOG(LogTemp,Display, TEXT("🎯   After sync, ActivePreparations count: %d"), IngredientDataCopy.ActivePreparations.Num());
    
    // Use the ingredient's GetCurrentDisplayName() which already handles preparations correctly
    // This method looks up preparations from the data table and uses NamePrefix properly
    // It formats as "PrepName IngredientName" for single prep, combines prefixes for multiple preps,
    // and uses "Suspicious IngredientName" for 2+ preparations
    FText Result = IngredientDataCopy.GetCurrentDisplayName();
    
    //UE_LOG(LogTemp,Display, TEXT("🎯   GetCurrentDisplayName() returned: %s"), *Result.ToString());
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetIngredientDisplayText - END"));
    
    return Result;
}

void UPUIngredientSlot::UpdateHoverTextVisibility(bool bShow)
{
    if (!HoverText)
    {
        return;
    }

    // For prep/pantry/prepped slots, show text if we have ingredient data (even if "empty")
    // For other locations, require bHasIngredient
    bool bCanShowText = false;
    if (Location == EPUIngredientSlotLocation::Pantry || Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::Prepped)
    {
        bCanShowText = IngredientInstance.IngredientData.IngredientTag.IsValid();
    }
    else
    {
        bCanShowText = bHasIngredient;
    }

    // For prepped slots, always show hover text (ignore bShow parameter)
    bool bShouldShow = bShow;
    if (Location == EPUIngredientSlotLocation::Prepped)
    {
        bShouldShow = true;
    }

    if (bShouldShow && bCanShowText)
    {
        // Update text content
        FText DisplayText = GetIngredientDisplayText();
        HoverText->SetText(DisplayText);
        HoverText->SetVisibility(ESlateVisibility::Visible);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateHoverTextVisibility - Showing hover text: %s"),
        //    *DisplayText.ToString());
    }
    else
    {
        // Don't hide hover text for prepped slots
        if (Location != EPUIngredientSlotLocation::Prepped)
        {
            HoverText->SetVisibility(ESlateVisibility::Collapsed);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::UpdateHoverTextVisibility - Hiding hover text"));
        }
    }
}

void UPUIngredientSlot::UpdateIngredientSelectVisibility(bool bShow)
{
    if (!IngredientSelect)
    {
        return;
    }
    if (bShow)
    {
        IngredientSelect->SetVisibility(ESlateVisibility::Visible);
        if (IngredientSelectAnim)
        {
            PlayAnimation(IngredientSelectAnim, 0.0f, 1, EUMGSequencePlayMode::Forward, 5.0f, false);
        }
    }
    else
    {
        IngredientSelect->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UPUIngredientSlot::SetSelected(bool bSelected)
{
    if (bIsSelected != bSelected)
    {
        bIsSelected = bSelected;
        UpdatePlateBackgroundOpacity();
    }
}

void UPUIngredientSlot::UpdatePlateBackgroundOpacity()
{
    if (!PlateBackground)
    {
        return;
    }

    // Always keep plate background at 100% opacity
    FLinearColor CurrentColor = PlateBackground->GetColorAndOpacity();
    CurrentColor.A = 1.0f;
    PlateBackground->SetColorAndOpacity(CurrentColor);
    
    // Outline disabled - IngredientSelect image used for hover/focus instead
    FSlateBrush Brush = PlateBackground->GetBrush();
    FLinearColor OutlineColor = Brush.OutlineSettings.Color.GetSpecifiedColor();
    OutlineColor.A = 0.0f;
    Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
    PlateBackground->SetBrush(Brush);
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
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetIngredientData - Could not find customization component"));
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
    
    //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::GenerateGUIDBasedInstanceID - Generated GUID-based InstanceID: %d from GUID: %s"), 
    //    UniqueID, *NewGUID.ToString());
    
    return UniqueID;
}

int32 UPUIngredientSlot::GenerateUniqueInstanceID() const
{
    // Call the static GUID-based function
    int32 UniqueID = GenerateGUIDBasedInstanceID();
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GenerateUniqueInstanceID - Generated unique ID %d for ingredient slot"), UniqueID);
    
    return UniqueID;
}

void UPUIngredientSlot::SetDragEnabled(bool bEnabled)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetDragEnabled - Setting drag enabled to %s"), 
    //    bEnabled ? TEXT("TRUE") : TEXT("FALSE"));
    
    bDragEnabled = bEnabled;
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetDragEnabled - Drag enabled set to: %s"), bEnabled ? TEXT("TRUE") : TEXT("FALSE"));
}

FReply UPUIngredientSlot::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Preview mouse button down (Drag enabled: %s, HasIngredient: %s, Button: %s)"), 
    //    bDragEnabled ? TEXT("TRUE") : TEXT("FALSE"), 
    //    bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"),
    //    *InMouseEvent.GetEffectingButton().ToString());

    // If the radial menu is currently visible, don't start drag detection from this slot.
    // Let the radial menu / its buttons handle the click so a single click selects an item.
    if (bRadialMenuVisible)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Radial menu visible, skipping drag detection"));
        return FReply::Unhandled();
    }
    
    // Check if the click is on the quantity control widget - if so, let it handle the click
    // This prevents drag detection from interfering with button clicks
    // Only do this if we have an ingredient (empty slots should allow clicks to open pantry)
    if (bHasIngredient && QuantityControlWidget && QuantityControlWidget->GetVisibility() == ESlateVisibility::Visible && QuantityControlContainer)
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
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Click is on quantity control (pos: %.2f,%.2f, container: %.2f,%.2f size: %.2f,%.2f), letting it handle the event"), 
            //    ClickPosition.X, ClickPosition.Y, ContainerAbsolutePosition.X, ContainerAbsolutePosition.Y, ContainerSize.X, ContainerSize.Y);
            return FReply::Unhandled(); // Let the quantity control buttons handle the click
        }
    }
    
    // Only handle left mouse button and only if drag is enabled
    // For pantry slots, we want clicks to work for selection, but still allow dragging if mouse moves
    // For prep slots, we want clicks to work for menu access, but still allow dragging if mouse moves
    bool bCanDrag = false;
    bool bIsPantrySlot = (Location == EPUIngredientSlotLocation::Pantry);
    bool bIsPrepSlot = (Location == EPUIngredientSlotLocation::Prep);
    
    if (bIsPantrySlot)
    {
        // For pantry slots, we want clicks to work for selection
        // Don't set up drag detection here - let the click handler process it
        // If user wants to drag, they can do it by actually dragging (we'll handle that separately if needed)
        if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Pantry slot clicked, allowing click handler to process (no drag detection)"));
            // Return Unhandled so NativeOnMouseButtonDown can process the click for selection
            return FReply::Unhandled();
        }
    }
    else
    {
        // Other slots (including Prep) require bHasIngredient to be true for dragging
        // Prep slots can drag, but clicks will also work because DetectDrag only triggers on actual drag movement
        bCanDrag = bDragEnabled && bHasIngredient;
    }
    
    // Only start drag if Shift is NOT pressed (Shift+Click shows menu instead)
    bool bShiftPressed = InMouseEvent.IsShiftDown();
    
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bCanDrag && !bIsPantrySlot && !bShiftPressed)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Starting drag detection for ingredient: %s (Location: %d)"), 
        //    *IngredientInstance.IngredientData.DisplayName.ToString(), (int32)Location);
        
        // Hide quantity control widget when dragging in plating stage
        if (Location == EPUIngredientSlotLocation::Plating && QuantityControlWidget)
        {
            QuantityControlWidget->SetVisibility(ESlateVisibility::Collapsed);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Hiding quantity control during drag in plating stage"));
        }
        
        // Start drag detection - this will call the Blueprint OnDragDetected event
        // Use DetectDrag which will trigger OnDragDetected when the mouse moves
        FReply Reply = FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - DetectDrag called, Reply.IsEventHandled: %s"), 
        //    Reply.IsEventHandled() ? TEXT("TRUE") : TEXT("FALSE"));
        return Reply;
    }
    else if (bShiftPressed && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Shift is pressed - don't start drag, let the click handler show the menu
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Shift pressed, skipping drag detection (menu will be shown)"));
        return FReply::Unhandled();
    }
    else
    {
        if (!bDragEnabled)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Drag not enabled"));
        }
        if (!bHasIngredient)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Slot has no ingredient"));
        }
        if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::NativeOnPreviewMouseButtonDown - Not left mouse button"));
        }
    }
    
    return FReply::Unhandled();
}

UPUIngredientDragDropOperation* UPUIngredientSlot::CreateIngredientDragDropOperation() const
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::CreateIngredientDragDropOperation - Creating drag operation for ingredient %s (ID: %d, Qty: %d, Location: %d)"), 
    //    *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID, IngredientInstance.Quantity, (int32)Location);

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
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::CreateIngredientDragDropOperation - Created drag visual widget with ingredient icon"));
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::CreateIngredientDragDropOperation - Failed to create drag visual widget"));
        }
        
        //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::CreateIngredientDragDropOperation - Successfully created drag operation (After Setup: ID: %d, Qty: %d)"), 
        //    DragOperation->IngredientInstance.InstanceID, DragOperation->IngredientInstance.Quantity);
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUIngredientSlot::CreateIngredientDragDropOperation - Failed to create drag operation"));
    }

    return DragOperation;
}

UPUIngredientSlot* UPUIngredientSlot::CreateDragVisualWidget() const
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::CreateDragVisualWidget - Creating drag visual widget for ingredient: %s"), 
    //    *IngredientInstance.IngredientData.DisplayName.ToString());

    if (!GetWorld())
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUIngredientSlot::CreateDragVisualWidget - No world context available"));
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
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::CreateDragVisualWidget - Explicitly set ingredient icon texture: %s"), 
                //    *IngredientInstance.IngredientData.PreviewTexture->GetName());
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::CreateDragVisualWidget - IngredientIcon is null or PreviewTexture is null"));
            if (!DragVisualWidget->IngredientIcon)
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️   IngredientIcon component not found in drag visual widget"));
            }
            if (!IngredientInstance.IngredientData.PreviewTexture)
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️   PreviewTexture is null for ingredient: %s"), 
                //    *IngredientInstance.IngredientData.DisplayName.ToString());
            }
        }
        
        // Update display to ensure everything is set correctly
        DragVisualWidget->UpdateDisplay();
        
        //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::CreateDragVisualWidget - Successfully created drag visual widget with ingredient icon"));
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUIngredientSlot::CreateDragVisualWidget - Failed to create drag visual widget"));
    }

    return DragVisualWidget;
}

void UPUIngredientSlot::DecreaseQuantity()
{
    if (RemainingQuantity > 0 && IngredientInstance.Quantity > 0)
    {
        RemainingQuantity--;
        // Keep IngredientInstance.Quantity in sync with RemainingQuantity
        // This is the source of truth for the display
        IngredientInstance.Quantity = RemainingQuantity;
        UpdateQuantityDisplay();
        
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::DecreaseQuantity - Quantity decreased to %d (RemainingQuantity: %d, IngredientInstance.Quantity: %d)"), 
        //    RemainingQuantity, RemainingQuantity, IngredientInstance.Quantity);
        
        // Call Blueprint event
        OnQuantityChanged(RemainingQuantity);
    }
}

void UPUIngredientSlot::ResetQuantity()
{
    RemainingQuantity = MaxQuantity;
    UpdateQuantityDisplay();
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::ResetQuantity - Quantity reset to %d"), RemainingQuantity);
    
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
            
            // Update bHasIngredient flag based on new quantity
            bHasIngredient = (IngredientInstance.Quantity > 0 && IngredientInstance.InstanceID != 0);
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::ResetQuantityFromDishData - Reset quantity to %d from dish data (bHasIngredient: %s)"), 
            //    IngredientInstance.Quantity, bHasIngredient ? TEXT("TRUE") : TEXT("FALSE"));
            
            // Update all visual displays
            UpdateQuantityDisplay();
            UpdateIngredientIcon();
            
            // Also update the full display to ensure everything is refreshed
            UpdateDisplay();
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ResetQuantityFromDishData - Could not find ingredient instance with ID %d"), IngredientInstance.InstanceID);
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ResetQuantityFromDishData - Could not find dish customization widget or component"));
    }
}

void UPUIngredientSlot::UpdatePlatingDisplay()
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::UpdatePlatingDisplay - Updating plating display"));
    
    // Update the ingredient icon/texture
    if (IngredientIcon)
    {
        IngredientIcon->SetBrushFromTexture(IngredientInstance.IngredientData.PreviewTexture);
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::UpdatePlatingDisplay - Updated icon texture"));
    }
    
    // Update quantity and preparation displays
    UpdateQuantityDisplay();
    UpdatePreparationDisplay();
}

void UPUIngredientSlot::UpdateQuantityDisplay()
{
    if (QuantityText)
    {
        // Check if we're in plating mode
        bool bIsPlatingMode = false;
        if (UPUDishCustomizationComponent* Component = GetDishCustomizationComponent())
        {
            bIsPlatingMode = Component->IsPlatingMode();
        }
        
        // Hide QuantityText in prep and cooking stages (show in plating mode)
        // But show it in plating mode even if location is ActiveIngredientArea
        if (!bIsPlatingMode && (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Prepped))
        {
            QuantityText->SetVisibility(ESlateVisibility::Collapsed);
            // //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::UpdateQuantityDisplay - Hiding QuantityText (Location: %d, PlatingMode: false)"), (int32)Location);
        }
        else if (bHasIngredient && IngredientInstance.InstanceID != 0)
        {
            // Read quantity directly from the ingredient instance
            int32 Quantity = IngredientInstance.Quantity;
            FString QuantityString = FString::Printf(TEXT("x%d"), Quantity);
            QuantityText->SetText(FText::FromString(QuantityString));
            QuantityText->SetVisibility(ESlateVisibility::Visible);
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::UpdateQuantityDisplay - Updated quantity: %s (from IngredientInstance.Quantity, PlatingMode: %s)"), 
            //    *QuantityString, bIsPlatingMode ? TEXT("TRUE") : TEXT("FALSE"));
        }
        else
        {
            // No ingredient, hide the text
            QuantityText->SetVisibility(ESlateVisibility::Collapsed);
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::UpdateQuantityDisplay - No ingredient, hiding QuantityText"));
        }
    }
}

void UPUIngredientSlot::UpdatePreparationDisplay()
{
    if (PreparationText)
    {
        // Hide PreparationText in prep, cooking, plating, and prepped stages
        if (Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea || Location == EPUIngredientSlotLocation::Plating || Location == EPUIngredientSlotLocation::Prepped)
        {
            PreparationText->SetVisibility(ESlateVisibility::Collapsed);
            // //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::UpdatePreparationDisplay - Hiding PreparationText (Location: %d)"), (int32)Location);
        }
        else
        {
            FString IconText = GetPreparationIconText();
            PreparationText->SetText(FText::FromString(IconText));
            PreparationText->SetVisibility(ESlateVisibility::Visible);
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::UpdatePreparationDisplay - Updated preparation icons: %s"), *IconText);
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
        
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Processing preparation tag: %s (cleaned: %s)"), 
        //    *Prep.ToString(), *PrepName);
        
        // Map preparation tags to text abbreviations
        if (PrepName.Contains(TEXT("Dehydrate")) || PrepName.Contains(TEXT("Dried")))
        {
            IconString += TEXT("[D]");
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Dehydrate/Dried: [D]"));
        }
        else if (PrepName.Contains(TEXT("Mince")) || PrepName.Contains(TEXT("Minced")))
        {
            IconString += TEXT("[M]");
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Mince/Minced: [M]"));
        }
        else if (PrepName.Contains(TEXT("Boiled")))
        {
            IconString += TEXT("[B]");
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Boiled: [B]"));
        }
        else if (PrepName.Contains(TEXT("Chopped")))
        {
            IconString += TEXT("[C]");
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Chopped: [C]"));
        }
        else if (PrepName.Contains(TEXT("Caramelized")))
        {
            IconString += TEXT("[CR]");
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Matched Caramelized: [CR]"));
        }
        else
        {
            // Default abbreviation for unknown preparations
            IconString += TEXT("[?]");
            //UE_LOG(LogTemp,Warning, TEXT("🎯 UPUIngredientSlot::GetPreparationIconText - Unknown preparation: %s (cleaned: %s)"), 
            //    *Prep.ToString(), *PrepName);
        }
    }
    
    return IconString;
}

void UPUIngredientSlot::SpawnIngredientAtPosition(const FVector2D& ScreenPosition)
{
    // Find the dish customization component (attached to cooking station)
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);

    UPUDishCustomizationComponent* DishComponent = nullptr;
    for (AActor* Actor : FoundActors)
    {
        if (Actor)
        {
            DishComponent = Actor->FindComponentByClass<UPUDishCustomizationComponent>();
            if (DishComponent)
            {
                break;
            }
        }
    }

    if (!DishComponent)
    {
        return;
    }

    // Check if 3D spawning is allowed (cooking or plating stage)
    if (!DishComponent->CanSpawnIngredientsIn3D())
    {
        return;
    }

    // Spawn directly above the cooking station/pan - no mouse/camera deprojection
    FVector SpawnPosition = DishComponent->GetSpawnPositionAboveStation();
    DishComponent->SpawnIngredientIn3DByInstanceID(IngredientInstance.InstanceID, SpawnPosition);
    
    // NOTE: Do NOT decrease quantity here - UpdateIngredientSlotQuantity() will be called
    // by SpawnIngredientIn3DByInstanceID, which will call DecreaseQuantity() to keep
    // both RemainingQuantity and IngredientInstance.Quantity in sync.
    // This prevents double-decrementing the quantity.
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUIngredientSlot::SpawnIngredientAtPosition - END (quantity update handled by UpdateIngredientSlotQuantity)"));
}

void UPUIngredientSlot::SetTextVisibility(bool bShowQuantity, bool bShowDescription)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetTextVisibility - Setting text visibility: Quantity=%s, Description=%s"), 
    //    bShowQuantity ? TEXT("TRUE") : TEXT("FALSE"), bShowDescription ? TEXT("TRUE") : TEXT("FALSE"));
    
    // Control quantity text visibility
    if (QuantityText)
    {
        QuantityText->SetVisibility(bShowQuantity ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetTextVisibility - Set quantity text visibility to: %s"), 
        //    bShowQuantity ? TEXT("Visible") : TEXT("Hidden"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::SetTextVisibility - QuantityText component not found"));
    }
    
    // Control preparation/description text visibility
    if (PreparationText)
    {
        PreparationText->SetVisibility(bShowDescription ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetTextVisibility - Set preparation text visibility to: %s"), 
        //    bShowDescription ? TEXT("Visible") : TEXT("Hidden"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::SetTextVisibility - PreparationText component not found"));
    }
}

void UPUIngredientSlot::HideAllText()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::HideAllText - Hiding all text elements"));
    LogTextComponentStatus();
    SetTextVisibility(false, false);
    
    // Force hide using different visibility modes
    if (QuantityText)
    {
        QuantityText->SetVisibility(ESlateVisibility::Collapsed);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::HideAllText - Force collapsed quantity text"));
    }
    
    if (PreparationText)
    {
        PreparationText->SetVisibility(ESlateVisibility::Collapsed);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::HideAllText - Force collapsed preparation text"));
    }
}

void UPUIngredientSlot::ShowAllText()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ShowAllText - Showing all text elements"));
    LogTextComponentStatus();
    SetTextVisibility(true, true);
}

void UPUIngredientSlot::LogTextComponentStatus()
{
    //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::LogTextComponentStatus - Checking text component status"));
    
    //UE_LOG(LogTemp,Display, TEXT("🔍 QuantityText: %s"), QuantityText ? TEXT("FOUND") : TEXT("NULL"));
    //UE_LOG(LogTemp,Display, TEXT("🔍 PreparationText: %s"), PreparationText ? TEXT("FOUND") : TEXT("NULL"));
    //UE_LOG(LogTemp,Display, TEXT("🔍 HoverText: %s"), HoverText ? TEXT("FOUND") : TEXT("NULL"));
    //UE_LOG(LogTemp,Display, TEXT("🔍 IngredientIcon: %s"), IngredientIcon ? TEXT("FOUND") : TEXT("NULL"));
    
    if (QuantityText)
    {
        //UE_LOG(LogTemp,Display, TEXT("🔍 QuantityText visibility: %s"), 
        //    QuantityText->GetVisibility() == ESlateVisibility::Visible ? TEXT("VISIBLE") : TEXT("HIDDEN"));
    }
    
    if (PreparationText)
    {
        //UE_LOG(LogTemp,Display, TEXT("🔍 PreparationText visibility: %s"), 
        //    PreparationText->GetVisibility() == ESlateVisibility::Visible ? TEXT("VISIBLE") : TEXT("HIDDEN"));
    }
}

void UPUIngredientSlot::OnRadialMenuItemSelected(const FRadialMenuItem& SelectedItem)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::OnRadialMenuItemSelected - Item selected: %s (Tag: %s)"),
    //    *SelectedItem.Label.ToString(), *SelectedItem.ActionTag.ToString());

    if (!SelectedItem.bIsEnabled)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::OnRadialMenuItemSelected - Item is disabled, ignoring selection"));
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
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::OnRadialMenuItemSelected - Unknown action tag: %s"), *TagString);
    }
    
    // Hide the menu after selection
    HideRadialMenu();
}

void UPUIngredientSlot::OnRadialMenuClosed()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::OnRadialMenuClosed - Radial menu closed"));
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
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::GetSlotScreenPosition - Slot center: (%.2f, %.2f)"),
        //CenterPosition.X, CenterPosition.Y);
    
    return CenterPosition;
}

TArray<FRadialMenuItem> UPUIngredientSlot::BuildPreparationMenuItems() const
{
    TArray<FRadialMenuItem> MenuItems;

    if (!bHasIngredient)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::BuildPreparationMenuItems - Slot has no ingredient"));
        return MenuItems;
    }

    // Get preparation data table from radial menu widget (not from ingredient)
    UDataTable* PrepDataTable = nullptr;
    if (RadialMenuWidget)
    {
        PrepDataTable = RadialMenuWidget->GetPreparationDataTable();
    }

    if (!PrepDataTable)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::BuildPreparationMenuItems - No preparation data table set on radial menu widget"));
        return MenuItems;
    }

    // Get all preparations from the data table
    TArray<FPUPreparationBase*> PreparationRows;
    PrepDataTable->GetAllRows<FPUPreparationBase>(TEXT("BuildPreparationMenuItems"), PreparationRows);

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::BuildPreparationMenuItems - Found %d total preparations"), PreparationRows.Num());
    
    // Sort preparations by their tag name to ensure consistent ordering
    PreparationRows.Sort([](const FPUPreparationBase& A, const FPUPreparationBase& B) {
        return A.PreparationTag.ToString() < B.PreparationTag.ToString();
    });
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::BuildPreparationMenuItems - Sorted preparations by tag name"));

    // Build menu items for each preparation
    for (FPUPreparationBase* PreparationData : PreparationRows)
    {
        if (!PreparationData || !PreparationData->PreparationTag.IsValid())
        {
            continue;
        }

        // Check if this preparation is compatible with the ingredient
        // We need to pass the ingredient's base tags, not the preparations
        // Combine ingredient base tag with current preparations for compatibility check
        FGameplayTagContainer IngredientTags;
        if (IngredientInstance.IngredientData.IngredientTag.IsValid())
        {
            IngredientTags.AddTag(IngredientInstance.IngredientData.IngredientTag);
        }
        // Also include current preparations in the check (some preparations might be incompatible with other preparations)
        IngredientTags.AppendTags(IngredientInstance.Preparations);
        
        bool bIsCompatible = PreparationData->CanApplyToIngredient(IngredientTags);
        
        // Check if this preparation is already applied
        bool bIsApplied = IngredientInstance.Preparations.HasTag(PreparationData->PreparationTag);

        // Create menu item
        FRadialMenuItem MenuItem;
        MenuItem.Label = PreparationData->DisplayName.IsEmpty() ? 
            FText::FromString(PreparationData->PreparationTag.ToString()) : 
            PreparationData->DisplayName;
        // Validate texture pointer before assignment to prevent GC issues
        MenuItem.Icon = IsValid(PreparationData->IconTexture) ? PreparationData->IconTexture : nullptr;
        MenuItem.ActionTag = PreparationData->PreparationTag;
        MenuItem.bIsEnabled = bIsCompatible || bIsApplied; // Enable if compatible or already applied (to allow removal)
        MenuItem.bIsApplied = bIsApplied;
        MenuItem.Tooltip = PreparationData->Description;

        MenuItems.Add(MenuItem);

        //UE_LOG(LogTemp,Display, TEXT("🎯   Preparation: %s (Compatible: %s, Applied: %s)"),
        //    *MenuItem.Label.ToString(),
        //    bIsCompatible ? TEXT("YES") : TEXT("NO"),
        //    bIsApplied ? TEXT("YES") : TEXT("NO"));
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::BuildPreparationMenuItems - Built %d menu items"), MenuItems.Num());
    return MenuItems;
}

TArray<FRadialMenuItem> UPUIngredientSlot::BuildActionMenuItems() const
{
    TArray<FRadialMenuItem> MenuItems;

    if (!bHasIngredient)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::BuildActionMenuItems - Slot has no ingredient"));
        return MenuItems;
    }

    // Action: Remove Ingredient
    FRadialMenuItem RemoveItem;
    RemoveItem.Label = FText::FromString(TEXT("Remove"));
    RemoveItem.ActionTag = FGameplayTag::RequestGameplayTag(FName("Action.Remove"));
    RemoveItem.bIsEnabled = true;
    RemoveItem.Tooltip = FText::FromString(TEXT("Remove this ingredient from the dish"));
    MenuItems.Add(RemoveItem);

    // NOTE: Clear Preparations radial button temporarily disabled.
    // Previous behavior (for reference):
    //
    // // Action: Clear Preparations (only if there are preparations)
    // if (IngredientInstance.Preparations.Num() > 0)
    // {
    //     FRadialMenuItem ClearPrepsItem;
    //     ClearPrepsItem.Label = FText::FromString(TEXT("Clear Preparations"));
    //     ClearPrepsItem.ActionTag = FGameplayTag::RequestGameplayTag(FName("Action.ClearPreparations"));
    //     ClearPrepsItem.bIsEnabled = true;
    //     ClearPrepsItem.Tooltip = FText::FromString(TEXT("Remove all preparations from this ingredient"));
    //     MenuItems.Add(ClearPrepsItem);
    // }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::BuildActionMenuItems - Built %d action menu items"), MenuItems.Num());
    return MenuItems;
}

bool UPUIngredientSlot::ApplyPreparationToIngredient(const FGameplayTag& PreparationTag)
{
    if (!bHasIngredient || IngredientInstance.InstanceID == 0)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ApplyPreparationToIngredient - Invalid ingredient instance"));
        return false;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ApplyPreparationToIngredient - Applying preparation %s to instance %d"),
    //    *PreparationTag.ToString(), IngredientInstance.InstanceID);

    // Get the dish customization widget and update dish data
    UPUDishCustomizationWidget* DishWidget = GetDishCustomizationWidget();
    if (DishWidget)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ApplyPreparationToIngredient - Found dish customization widget, applying preparation"));
        
        // Get the current dish data from the dish customization widget
        FPUDishBase CurrentDish = DishWidget->GetCurrentDishData();
        
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ApplyPreparationToIngredient - Dish has %d ingredient instances, looking for ID: %d"), 
        //    CurrentDish.IngredientInstances.Num(), IngredientInstance.InstanceID);

        // Apply the preparation using the blueprint library
        bool bSuccess = UPUDishBlueprintLibrary::ApplyPreparationByID(CurrentDish, IngredientInstance.InstanceID, PreparationTag);
        
        if (bSuccess)
        {
            // Update the dish customization widget's dish data
            DishWidget->UpdateDishData(CurrentDish);
            
            // Update the ingredient instance to reflect the change
            FIngredientInstance UpdatedInstance;
            if (CurrentDish.GetIngredientInstanceByID(IngredientInstance.InstanceID, UpdatedInstance))
            {
                SetIngredientInstance(UpdatedInstance);
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::ApplyPreparationToIngredient - Preparation applied successfully"));
                
                // Get average color from ingredient texture (recalculate to ensure it's up to date)
                GetAverageColorFromIngredientTexture();
                
                // Update the icon to apply the newly calculated color and pixelation
                UpdateIngredientIcon();
                
                // If we're in prep or active ingredient area and have preparations, create/update the prepped slot
                if ((Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea) && UpdatedInstance.Preparations.Num() > 0)
                {
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ApplyPreparationToIngredient - In prep/active ingredient area, creating/updating prepped slot"));
                    DishWidget->CreateOrUpdatePreppedSlot(UpdatedInstance);
                }
            }
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ApplyPreparationToIngredient - Failed to apply preparation"));
        }
        
        return bSuccess;
    }
    
    // Fallback: try to get the component directly
    UPUDishCustomizationComponent* DishComponent = GetDishCustomizationComponent();
    if (DishComponent)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ApplyPreparationToIngredient - Found dish component, applying preparation via component"));
        
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
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::ApplyPreparationToIngredient - Preparation applied successfully via component"));
                
                // Get average color from ingredient texture (recalculate to ensure it's up to date)
                GetAverageColorFromIngredientTexture();
                
                // Update the icon to apply the newly calculated color and pixelation
                UpdateIngredientIcon();
                
                // If we're in prep or active ingredient area and have preparations, create/update the prepped slot
                if ((Location == EPUIngredientSlotLocation::Prep || Location == EPUIngredientSlotLocation::ActiveIngredientArea) && UpdatedInstance.Preparations.Num() > 0)
                {
                    UPUDishCustomizationWidget* PreppedDishWidget = GetDishCustomizationWidget();
                    if (PreppedDishWidget)
                    {
                        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ApplyPreparationToIngredient - In prep/active ingredient area, creating/updating prepped slot (via component path)"));
                        PreppedDishWidget->CreateOrUpdatePreppedSlot(UpdatedInstance);
                    }
                }
            }
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ApplyPreparationToIngredient - Failed to apply preparation"));
        }

        return bSuccess;
    }
    
    //UE_LOG(LogTemp,Error, TEXT("❌ UPUIngredientSlot::ApplyPreparationToIngredient - Could not find dish customization widget or component! Cannot apply preparation."));
    return false;
}

bool UPUIngredientSlot::RemovePreparationFromIngredient(const FGameplayTag& PreparationTag)
{
    if (!bHasIngredient || IngredientInstance.InstanceID == 0)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::RemovePreparationFromIngredient - Invalid ingredient instance"));
        return false;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RemovePreparationFromIngredient - Removing preparation %s from instance %d"),
    //    *PreparationTag.ToString(), IngredientInstance.InstanceID);

    // Get the dish customization widget and update dish data
    UPUDishCustomizationWidget* DishWidget = GetDishCustomizationWidget();
    if (DishWidget)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RemovePreparationFromIngredient - Found dish customization widget, removing preparation"));
        
        // Get the current dish data from the dish customization widget
        FPUDishBase CurrentDish = DishWidget->GetCurrentDishData();
        
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RemovePreparationFromIngredient - Dish has %d ingredient instances, looking for ID: %d"), 
        //    CurrentDish.IngredientInstances.Num(), IngredientInstance.InstanceID);

        // If we're in active ingredient area, remove the old prepped slot first (before removing the prep)
        if (Location == EPUIngredientSlotLocation::ActiveIngredientArea)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RemovePreparationFromIngredient - In active ingredient area, removing old prepped slot"));
            DishWidget->RemovePreppedSlot(IngredientInstance);
        }
        
        // Remove the preparation using the blueprint library
        bool bSuccess = UPUDishBlueprintLibrary::RemovePreparationByID(CurrentDish, IngredientInstance.InstanceID, PreparationTag);
        
        if (bSuccess)
        {
            // Update the dish customization widget's dish data
            DishWidget->UpdateDishData(CurrentDish);
            
            // Update the ingredient instance to reflect the change
            FIngredientInstance UpdatedInstance;
            if (CurrentDish.GetIngredientInstanceByID(IngredientInstance.InstanceID, UpdatedInstance))
            {
                SetIngredientInstance(UpdatedInstance);
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::RemovePreparationFromIngredient - Preparation removed successfully"));
                
                // If we're in active ingredient area and still have preparations, create/update the prepped slot with new state
                if (Location == EPUIngredientSlotLocation::ActiveIngredientArea && UpdatedInstance.Preparations.Num() > 0)
                {
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RemovePreparationFromIngredient - In active ingredient area, creating/updating prepped slot with remaining preps"));
                    DishWidget->CreateOrUpdatePreppedSlot(UpdatedInstance);
                }
            }
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::RemovePreparationFromIngredient - Failed to remove preparation"));
        }
        
        return bSuccess;
    }
    
    // Fallback: try to get the component directly
    UPUDishCustomizationComponent* DishComponent = GetDishCustomizationComponent();
    if (DishComponent)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RemovePreparationFromIngredient - Found dish component, removing preparation via component"));
        
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
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::RemovePreparationFromIngredient - Preparation removed successfully via component"));
            }
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::RemovePreparationFromIngredient - Failed to remove preparation"));
        }

        return bSuccess;
    }
    
    //UE_LOG(LogTemp,Error, TEXT("❌ UPUIngredientSlot::RemovePreparationFromIngredient - Could not find dish customization widget or component! Cannot remove preparation."));
    return false;
}

void UPUIngredientSlot::ExecuteAction(const FGameplayTag& ActionTag)
{
    FString ActionString = ActionTag.ToString();
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ExecuteAction - Executing action: %s"), *ActionString);

    if (ActionString == TEXT("Action.Remove"))
    {
        // Get the dish customization widget and remove the ingredient
        UPUDishCustomizationWidget* DishWidget = GetDishCustomizationWidget();
        if (DishWidget)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ExecuteAction - Found dish widget, removing instance ID: %d"), IngredientInstance.InstanceID);
            DishWidget->RemoveIngredientInstance(IngredientInstance.InstanceID);
            ClearSlot();
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::ExecuteAction - Ingredient removed successfully via widget"));
        }
        else
        {
            // Fallback: try to get the component directly
            UPUDishCustomizationComponent* DishComponent = GetDishCustomizationComponent();
            if (DishComponent)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::ExecuteAction - Found dish component, removing instance ID: %d"), IngredientInstance.InstanceID);
                FPUDishBase CurrentDish = DishComponent->GetCurrentDishData();
                bool bSuccess = UPUDishBlueprintLibrary::RemoveIngredientInstanceByID(CurrentDish, IngredientInstance.InstanceID);
                if (bSuccess)
                {
                    DishComponent->UpdateCurrentDishData(CurrentDish);
                    ClearSlot();
                    //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::ExecuteAction - Ingredient removed successfully via component"));
                }
                else
                {
                    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ExecuteAction - Failed to remove ingredient instance ID: %d"), IngredientInstance.InstanceID);
                }
            }
            else
            {
                //UE_LOG(LogTemp,Error, TEXT("❌ UPUIngredientSlot::ExecuteAction - Could not find dish customization widget or component! Cannot remove ingredient."));
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
        
        //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::ExecuteAction - All preparations cleared"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::ExecuteAction - Unknown action: %s"), *ActionString);
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
        // Continue searching parent hierarchy
        {
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
    //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - START (Slot: %s)"), *GetName());
    
    // First, check if we have a cached reference
    if (CachedDishWidget.IsValid())
    {
        //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - Found cached widget: %s"), 
        //    *CachedDishWidget.Get()->GetName());
        return CachedDishWidget.Get();
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::GetDishCustomizationWidget - Cached widget is NULL or invalid"));
    }

    // Try to get it from the outer widget (slots are created with dish widget as outer)
    //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - Trying GetOuter() traversal"));
    UObject* Outer = GetOuter();
    int32 OuterDepth = 0;
    while (Outer && OuterDepth < 10) // Limit depth to prevent infinite loops
    {
        //UE_LOG(LogTemp,Display, TEXT("🔍   Outer[%d]: %s (Class: %s)"), OuterDepth, *Outer->GetName(), *Outer->GetClass()->GetName());
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(Outer))
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::GetDishCustomizationWidget - Found via GetOuter(): %s"), *DishWidget->GetName());
            // Cache it for future use
            const_cast<UPUIngredientSlot*>(this)->CachedDishWidget = DishWidget;
            return DishWidget;
        }
        Outer = Outer->GetOuter();
        OuterDepth++;
    }

    // Try to find the dish customization widget by traversing up the widget hierarchy
    //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - Trying GetParent() traversal"));
    UWidget* Parent = GetParent();
    int32 ParentDepth = 0;
    while (Parent && ParentDepth < 10) // Limit depth to prevent infinite loops
    {
        //UE_LOG(LogTemp,Display, TEXT("🔍   Parent[%d]: %s (Class: %s)"), ParentDepth, *Parent->GetName(), *Parent->GetClass()->GetName());
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(Parent))
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::GetDishCustomizationWidget - Found via GetParent(): %s"), *DishWidget->GetName());
            // Cache it for future use
            const_cast<UPUIngredientSlot*>(this)->CachedDishWidget = DishWidget;
            return DishWidget;
        }
        // Continue searching parent hierarchy
        {
            UUserWidget* RootWidget = GetTypedOuter<UUserWidget>();
            while (RootWidget)
            {
                if (UPUDishCustomizationWidget* FoundDishWidget = Cast<UPUDishCustomizationWidget>(RootWidget))
                {
                    //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::GetDishCustomizationWidget - Found via CookingStageWidget: %s"), *FoundDishWidget->GetName());
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
    //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::GetDishCustomizationWidget - Trying GetTypedOuter() traversal"));
    UUserWidget* RootWidget = GetTypedOuter<UUserWidget>();
    int32 TypedOuterDepth = 0;
    while (RootWidget && TypedOuterDepth < 10)
    {
        //UE_LOG(LogTemp,Display, TEXT("🔍   TypedOuter[%d]: %s (Class: %s)"), TypedOuterDepth, *RootWidget->GetName(), *RootWidget->GetClass()->GetName());
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(RootWidget))
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUIngredientSlot::GetDishCustomizationWidget - Found via GetTypedOuter(): %s"), *DishWidget->GetName());
            // Cache it for future use
            const_cast<UPUIngredientSlot*>(this)->CachedDishWidget = DishWidget;
            return DishWidget;
        }
        RootWidget = RootWidget->GetTypedOuter<UUserWidget>();
        TypedOuterDepth++;
    }
    
    //UE_LOG(LogTemp,Error, TEXT("❌ UPUIngredientSlot::GetDishCustomizationWidget - Could not find dish widget! Slot: %s"), *GetName());
    return nullptr;
}

void UPUIngredientSlot::SetDishCustomizationWidget(UPUDishCustomizationWidget* InDishWidget)
{
    CachedDishWidget = InDishWidget;
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::SetDishCustomizationWidget - Cached dish widget reference: %s"), 
    //    InDishWidget ? *InDishWidget->GetName() : TEXT("NULL"));
}

// ============================================================================
// Time/Temperature Slider Functions
// ============================================================================

void UPUIngredientSlot::InitializeTimeTempSliders()
{
    // Cache original styles for focus-as-hover (restore when losing focus)
    if (TimeSlider)
    {
        CachedTimeSliderStyle = TimeSlider->GetWidgetStyle();
    }
    if (TemperatureSlider)
    {
        CachedTemperatureSliderStyle = TemperatureSlider->GetWidgetStyle();
    }
    // Set up time slider
    if (TimeSlider)
    {
        TimeSlider->SetMinValue(0.0f);
        TimeSlider->SetMaxValue(1.0f);
        TimeSlider->SetValue(IngredientInstance.TimeValue);
        
        // Bind event
        if (!TimeSlider->OnValueChanged.IsBound())
        {
            TimeSlider->OnValueChanged.AddDynamic(this, &UPUIngredientSlot::OnTimeSliderValueChanged);
        }
        
        TimeSlider->SetStepSize(0.25f); // Controller D-pad step size
        TimeSlider->RequiresControllerLock = false; // Allow D-pad to work immediately when focused (no A-button lock needed)
        TimeSlider->SynchronizeProperties(); // Push to underlying Slate widget
    }
    
    // Set up temperature slider
    if (TemperatureSlider)
    {
        TemperatureSlider->SetMinValue(0.0f);
        TemperatureSlider->SetMaxValue(1.0f);
        TemperatureSlider->SetValue(IngredientInstance.TemperatureValue);
        
        // Bind event
        if (!TemperatureSlider->OnValueChanged.IsBound())
        {
            TemperatureSlider->OnValueChanged.AddDynamic(this, &UPUIngredientSlot::OnTemperatureSliderValueChanged);
        }
        
        TemperatureSlider->SetStepSize(0.25f); // Controller D-pad step size
        TemperatureSlider->RequiresControllerLock = false; // Allow D-pad to work immediately when focused (no A-button lock needed)
        TemperatureSlider->SynchronizeProperties(); // Push to underlying Slate widget
    }
    
    // Update visibility and labels
    UpdateSliderVisibility();
    UpdateTimeLabelText();
    UpdateTemperatureLabelText();
}

void UPUIngredientSlot::UpdateSliderFocusVisuals()
{
    if (!ShouldShowSliders()) return;

    // Time slider: show hover style when focused
    if (TimeSlider && TimeSlider->IsVisible())
    {
        bool bHasFocus = TimeSlider->HasKeyboardFocus();
        if (bHasFocus && !bTimeSliderShowingHoverStyle)
        {
            FSliderStyle HoverStyle = CachedTimeSliderStyle;
            HoverStyle.SetNormalThumbImage(CachedTimeSliderStyle.HoveredThumbImage);
            TimeSlider->SetWidgetStyle(HoverStyle);
            bTimeSliderShowingHoverStyle = true;
        }
        else if (!bHasFocus && bTimeSliderShowingHoverStyle)
        {
            TimeSlider->SetWidgetStyle(CachedTimeSliderStyle);
            bTimeSliderShowingHoverStyle = false;
        }
    }
    else if (bTimeSliderShowingHoverStyle)
    {
        if (TimeSlider)
        {
            TimeSlider->SetWidgetStyle(CachedTimeSliderStyle);
        }
        bTimeSliderShowingHoverStyle = false;
    }

    // Temperature slider: show hover style when focused
    if (TemperatureSlider && TemperatureSlider->IsVisible())
    {
        bool bHasFocus = TemperatureSlider->HasKeyboardFocus();
        if (bHasFocus && !bTemperatureSliderShowingHoverStyle)
        {
            FSliderStyle HoverStyle = CachedTemperatureSliderStyle;
            HoverStyle.SetNormalThumbImage(CachedTemperatureSliderStyle.HoveredThumbImage);
            TemperatureSlider->SetWidgetStyle(HoverStyle);
            bTemperatureSliderShowingHoverStyle = true;
        }
        else if (!bHasFocus && bTemperatureSliderShowingHoverStyle)
        {
            TemperatureSlider->SetWidgetStyle(CachedTemperatureSliderStyle);
            bTemperatureSliderShowingHoverStyle = false;
        }
    }
    else if (bTemperatureSliderShowingHoverStyle)
    {
        if (TemperatureSlider)
        {
            TemperatureSlider->SetWidgetStyle(CachedTemperatureSliderStyle);
        }
        bTemperatureSliderShowingHoverStyle = false;
    }
}

void UPUIngredientSlot::RecalculateAspectsFromBase()
{
    if (!bHasIngredient)
    {
        return;
    }
    
    // Get the base ingredient from dish data (includes base aspects + preparations, but NOT time/temp/quantity)
    if (UPUDishCustomizationWidget* DishWidget = GetDishCustomizationWidget())
    {
        FPUDishBase CurrentDish = DishWidget->GetCurrentDishData();
        FPUIngredientBase BaseIngredient;
        
        // Use IngredientData.IngredientTag if IngredientTag is not valid
        FGameplayTag IngredientTagToUse = IngredientInstance.IngredientTag.IsValid() 
            ? IngredientInstance.IngredientTag 
            : IngredientInstance.IngredientData.IngredientTag;
        
        if (!IngredientTagToUse.IsValid())
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::RecalculateAspectsFromBase - No valid ingredient tag found!"));
            return;
        }
        
        // Ensure dish has ingredient data table (fallback to component's data table if needed)
        if (!CurrentDish.IngredientDataTable.IsValid())
        {
            if (UPUDishCustomizationComponent* Component = DishWidget->GetCustomizationComponent())
            {
                if (Component->IngredientDataTable)
                {
                    CurrentDish.IngredientDataTable = TSoftObjectPtr<UDataTable>(Component->IngredientDataTable);
                    //UE_LOG(LogTemp,Display, TEXT("🔍 UPUIngredientSlot::RecalculateAspectsFromBase - Using component's ingredient data table"));
                }
            }
        }
        
        // Sync preparations to ActivePreparations before getting base ingredient
        // (GetIngredient uses ActivePreparations to apply preparation modifiers)
        BaseIngredient.ActivePreparations = IngredientInstance.Preparations;
        
        // Get base ingredient from dish data table (includes preparations)
        // GetIngredient will apply preparations based on BaseIngredient.ActivePreparations
        if (CurrentDish.GetIngredient(IngredientTagToUse, BaseIngredient))
        {
            // Start with base aspects (base + preparations)
            FFlavorAspects PerUnitFlavor = BaseIngredient.FlavorAspects;
            FTextureAspects PerUnitTexture = BaseIngredient.TextureAspects;
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RecalculateAspectsFromBase - Base aspects: Umami=%.2f, Sweet=%.2f, Time=%.2f, Temp=%.2f, Qty=%d"),
            //    PerUnitFlavor.Umami, PerUnitFlavor.Sweet, IngredientInstance.TimeValue, IngredientInstance.TemperatureValue, IngredientInstance.Quantity);
            
            // Apply time/temp modifiers to get per-unit modified aspects
            BaseIngredient.CalculateTimeTempModifiedAspects(
                IngredientInstance.TimeValue,
                IngredientInstance.TemperatureValue,
                PerUnitFlavor,
                PerUnitTexture
            );
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RecalculateAspectsFromBase - After time/temp: Umami=%.2f, Sweet=%.2f"),
            //    PerUnitFlavor.Umami, PerUnitFlavor.Sweet);
            
            // Store per-unit aspects (quantity multiplication happens in GetTotalFlavorAspect when summing)
            // This keeps IngredientData consistent - it always represents a single unit
            IngredientInstance.IngredientData.FlavorAspects.Umami = PerUnitFlavor.Umami;
            IngredientInstance.IngredientData.FlavorAspects.Salt = PerUnitFlavor.Salt;
            IngredientInstance.IngredientData.FlavorAspects.Sweet = PerUnitFlavor.Sweet;
            IngredientInstance.IngredientData.FlavorAspects.Sour = PerUnitFlavor.Sour;
            IngredientInstance.IngredientData.FlavorAspects.Bitter = PerUnitFlavor.Bitter;
            IngredientInstance.IngredientData.FlavorAspects.Spicy = PerUnitFlavor.Spicy;
            
            IngredientInstance.IngredientData.TextureAspects.Rich = PerUnitTexture.Rich;
            IngredientInstance.IngredientData.TextureAspects.Juicy = PerUnitTexture.Juicy;
            IngredientInstance.IngredientData.TextureAspects.Tender = PerUnitTexture.Tender;
            IngredientInstance.IngredientData.TextureAspects.Chewy = PerUnitTexture.Chewy;
            IngredientInstance.IngredientData.TextureAspects.Crispy = PerUnitTexture.Crispy;
            IngredientInstance.IngredientData.TextureAspects.Crumbly = PerUnitTexture.Crumbly;
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RecalculateAspectsFromBase - Final per-unit aspects: Umami=%.2f, Sweet=%.2f (Qty=%d, Total Umami=%.2f)"),
            //    IngredientInstance.IngredientData.FlavorAspects.Umami, IngredientInstance.IngredientData.FlavorAspects.Sweet, 
            //    IngredientInstance.Quantity, IngredientInstance.IngredientData.FlavorAspects.Umami * IngredientInstance.Quantity);
            
            // Update dish data (triggers OnDishDataChanged and updates radar chart automatically)
            for (int32 i = 0; i < CurrentDish.IngredientInstances.Num(); i++)
            {
                if (CurrentDish.IngredientInstances[i].InstanceID == IngredientInstance.InstanceID)
                {
                    CurrentDish.IngredientInstances[i] = IngredientInstance;
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RecalculateAspectsFromBase - Updated ingredient instance %d in dish data"), IngredientInstance.InstanceID);
                    break;
                }
            }
            
            DishWidget->UpdateDishData(CurrentDish);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUIngredientSlot::RecalculateAspectsFromBase - Called UpdateDishData"));
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::RecalculateAspectsFromBase - Failed to get ingredient from dish data table! Tag: %s"),
            //    *IngredientTagToUse.ToString());
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUIngredientSlot::RecalculateAspectsFromBase - Could not find dish widget!"));
    }
}

void UPUIngredientSlot::SetTimeValue(float NewTimeValue)
{
    NewTimeValue = FMath::Clamp(NewTimeValue, 0.0f, 1.0f);
    IngredientInstance.TimeValue = NewTimeValue;
    
    // Update slider if it exists and value is different (to avoid recursion)
    if (TimeSlider && FMath::Abs(TimeSlider->GetValue() - NewTimeValue) > KINDA_SMALL_NUMBER)
    {
        TimeSlider->SetValue(NewTimeValue);
    }
    
    UpdateTimeLabelText();
    
    // Recalculate aspects from base + time/temp + quantity
    RecalculateAspectsFromBase();
    
    OnSlotIngredientChanged.Broadcast(IngredientInstance);
    OnTimeTemperatureChanged(NewTimeValue, IngredientInstance.TemperatureValue);
}

void UPUIngredientSlot::SetTemperatureValue(float NewTemperatureValue)
{
    NewTemperatureValue = FMath::Clamp(NewTemperatureValue, 0.0f, 1.0f);
    IngredientInstance.TemperatureValue = NewTemperatureValue;
    
    // Update slider if it exists and value is different (to avoid recursion)
    if (TemperatureSlider && FMath::Abs(TemperatureSlider->GetValue() - NewTemperatureValue) > KINDA_SMALL_NUMBER)
    {
        TemperatureSlider->SetValue(NewTemperatureValue);
    }
    
    UpdateTemperatureLabelText();
    
    // Recalculate aspects from base + time/temp + quantity
    RecalculateAspectsFromBase();
    
    OnSlotIngredientChanged.Broadcast(IngredientInstance);
    OnTimeTemperatureChanged(IngredientInstance.TimeValue, NewTemperatureValue);
}

void UPUIngredientSlot::OnTimeSliderValueChanged(float NewValue)
{
    SetTimeValue(NewValue);
}

void UPUIngredientSlot::OnTemperatureSliderValueChanged(float NewValue)
{
    SetTemperatureValue(NewValue);
}

void UPUIngredientSlot::UpdateTimeTempSliders()
{
    // Sync slider values with ingredient instance
    if (TimeSlider)
    {
        TimeSlider->SetValue(IngredientInstance.TimeValue);
    }
    
    if (TemperatureSlider)
    {
        TemperatureSlider->SetValue(IngredientInstance.TemperatureValue);
    }
    
    UpdateTimeLabelText();
    UpdateTemperatureLabelText();
}

void UPUIngredientSlot::UpdateSliderVisibility()
{
    bool bShouldShow = ShouldShowSliders();
    
    // Explicitly hide time and temp in prep stage
    if (Location == EPUIngredientSlotLocation::Prep)
    {
        bShouldShow = false;
    }
    
    if (TimeSlider)
    {
        TimeSlider->SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        TimeSlider->SetIsEnabled(bSlidersEnabled && bShouldShow);
    }
    
    if (TemperatureSlider)
    {
        TemperatureSlider->SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        TemperatureSlider->SetIsEnabled(bSlidersEnabled && bShouldShow);
    }
    
    if (TimeLabelText)
    {
        TimeLabelText->SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (TemperatureLabelText)
    {
        TemperatureLabelText->SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UPUIngredientSlot::SetSlidersEnabled(bool bEnabled)
{
    bSlidersEnabled = bEnabled;
    
    if (TimeSlider)
    {
        TimeSlider->SetIsEnabled(bEnabled && ShouldShowSliders());
    }
    
    if (TemperatureSlider)
    {
        TemperatureSlider->SetIsEnabled(bEnabled && ShouldShowSliders());
    }
}

void UPUIngredientSlot::UpdateTimeLabelText()
{
    if (!TimeLabelText) return;
    
    ETimeState State = FPUIngredientBase::MapTimeValueToState(IngredientInstance.TimeValue);
    FString Label;
    
    switch (State)
    {
        case ETimeState::None:
            Label = TEXT("None");
            break;
        case ETimeState::Low:
            Label = TEXT("Low");
            break;
        case ETimeState::Mid:
            Label = TEXT("Mid");
            break;
        case ETimeState::Long:
            Label = TEXT("Long");
            break;
        default:
            Label = TEXT("None");
            break;
    }
    
    TimeLabelText->SetText(FText::FromString(Label));
}

void UPUIngredientSlot::UpdateTemperatureLabelText()
{
    if (!TemperatureLabelText) return;
    
    ETemperatureState State = FPUIngredientBase::MapTemperatureValueToState(IngredientInstance.TemperatureValue);
    FString Label;
    
    switch (State)
    {
        case ETemperatureState::Raw:
            Label = TEXT("Raw");
            break;
        case ETemperatureState::Low:
            Label = TEXT("Low");
            break;
        case ETemperatureState::Med:
            Label = TEXT("Med");
            break;
        case ETemperatureState::Hot:
            Label = TEXT("Hot");
            break;
        default:
            Label = TEXT("Raw");
            break;
    }
    
    TemperatureLabelText->SetText(FText::FromString(Label));
}

bool UPUIngredientSlot::ShouldShowSliders() const
{
    // Show sliders only in ActiveIngredientArea (cooking stage) when we have an ingredient
    if (Location == EPUIngredientSlotLocation::ActiveIngredientArea)
    {
        return bHasIngredient && bShowTimeTempSliders;
    }
    
    return false;
}

FReply UPUIngredientSlot::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // Handle controller button presses
    FKey Key = InKeyEvent.GetKey();
    
    // Log input for debugging
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Key pressed: %s (Slot: %s, Location: %d, HasFocus: %s)"), 
        *Key.ToString(), *GetName(), (int32)Location, HasKeyboardFocus() ? TEXT("YES") : TEXT("NO"));
    
    // Gamepad A button (Xbox) / X button (PlayStation) - Select/Activate
    if (Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Enter || Key == EKeys::SpaceBar)
    {
        // If radial menu is visible, don't process input here - let the menu handle it
        if (bRadialMenuVisible && RadialMenuWidget && RadialMenuWidget->IsMenuVisible())
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Radial menu visible, passing input to menu"));
            return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
        }
        
        UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Select button pressed, calling HandleControllerSelect"));
        HandleControllerSelect();
        return FReply::Handled();
    }
    
    // DISABLED FOR NOW - Focus on navigation only
    // Gamepad X button (Xbox) / Square button (PlayStation) - Open menu
    //if (Key == EKeys::Gamepad_FaceButton_Left)
    //{
    //    UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Menu button pressed, calling HandleControllerMenu"));
    //    HandleControllerMenu();
    //    return FReply::Handled();
    //}
    
    // If radial menu is visible, block all navigation - let the menu handle input
    if (bRadialMenuVisible && RadialMenuWidget && RadialMenuWidget->IsMenuVisible())
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Radial menu visible, blocking navigation input"));
        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
    }
    
    // Handle quantity controls with left/right shoulder buttons
    // Left bumper = decrease, Right bumper = increase
    if (Key == EKeys::Gamepad_LeftShoulder)
    {
        // Only handle if slot has focus, has an ingredient, and has a quantity control
        if (HasKeyboardFocus() && bHasIngredient && QuantityControlWidget && QuantityControlWidget->IsVisible())
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Left bumper pressed, decreasing quantity"));
            // DecreaseQuantity() handles min/max limits internally
            QuantityControlWidget->DecreaseQuantity();
            return FReply::Handled();
        }
    }
    else if (Key == EKeys::Gamepad_RightShoulder)
    {
        // Only handle if slot has focus, has an ingredient, and has a quantity control
        if (HasKeyboardFocus() && bHasIngredient && QuantityControlWidget && QuantityControlWidget->IsVisible())
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Right bumper pressed, increasing quantity"));
            // IncreaseQuantity() handles min/max limits internally
            QuantityControlWidget->IncreaseQuantity();
            return FReply::Handled();
        }
    }
    
    // Handle D-pad and left stick navigation
    if (Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up || Key == EKeys::Up)
    {
        if (NavigationUp.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Navigating UP to slot: %s"), *NavigationUp->GetName());
            NavigationUp->SetKeyboardFocus();
            return FReply::Handled();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - UP navigation requested but NavigationUp is invalid"));
        }
    }
    else if (Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down || Key == EKeys::Down)
    {
        if (NavigationDown.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Navigating DOWN to slot: %s"), *NavigationDown->GetName());
            NavigationDown->SetKeyboardFocus();
            return FReply::Handled();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - DOWN navigation requested but NavigationDown is invalid"));
        }
    }
    else if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::Left)
    {
        if (NavigationLeft.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Navigating LEFT to slot: %s"), *NavigationLeft->GetName());
            NavigationLeft->SetKeyboardFocus();
            return FReply::Handled();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - LEFT navigation requested but NavigationLeft is invalid"));
        }
    }
    else if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right || Key == EKeys::Right)
    {
        if (NavigationRight.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Navigating RIGHT to slot: %s"), *NavigationRight->GetName());
            NavigationRight->SetKeyboardFocus();
            return FReply::Handled();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - RIGHT navigation requested but NavigationRight is invalid"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("🎮 UPUIngredientSlot::NativeOnKeyDown - Unhandled key: %s, passing to parent"), *Key.ToString());
    }
    
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPUIngredientSlot::HandleControllerSelect()
{
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::HandleControllerSelect - Called (Slot: %s, Location: %d, Empty: %s)"), 
        *GetName(), (int32)Location, IsEmpty() ? TEXT("YES") : TEXT("NO"));
    
    // For prep stage: If slot is empty, open pantry. If slot has ingredient, open radial menu
    if (Location == EPUIngredientSlotLocation::Prep)
    {
        if (IsEmpty())
        {
            // Empty slot - trigger empty slot click (opens pantry)
            OnEmptySlotClicked.Broadcast(this);
        }
        else
        {
            // Slot has ingredient - open radial menu for preparations AND actions (same as right-click)
            ShowRadialMenu(true, true);
        }
    }
    else if (Location == EPUIngredientSlotLocation::Pantry)
    {
        // Pantry slot - select ingredient (this will be handled by the dish widget)
        OnEmptySlotClicked.Broadcast(this);
    }
    else if (Location == EPUIngredientSlotLocation::ActiveIngredientArea)
    {
        // Active ingredient area - open radial menu
        if (bHasIngredient)
        {
            ShowRadialMenu(true, true);
        }
    }
}

void UPUIngredientSlot::HandleControllerMenu()
{
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUIngredientSlot::HandleControllerMenu - Called (Slot: %s, Location: %d, HasIngredient: %s)"), 
        *GetName(), (int32)Location, bHasIngredient ? TEXT("YES") : TEXT("NO"));
    
    // Open radial menu when X/Square is pressed
    if (bHasIngredient || Location == EPUIngredientSlotLocation::Prep)
    {
        bool bIsPrepMenu = (Location == EPUIngredientSlotLocation::Prep);
        bool bIncludeActions = (Location == EPUIngredientSlotLocation::ActiveIngredientArea);
        ShowRadialMenu(bIsPrepMenu, bIncludeActions);
    }
}

void UPUIngredientSlot::SetupNavigation(UPUIngredientSlot* UpSlot, UPUIngredientSlot* DownSlot, UPUIngredientSlot* LeftSlot, UPUIngredientSlot* RightSlot)
{
    // Store navigation references for manual navigation handling in NativeOnKeyDown
    NavigationUp = UpSlot;
    NavigationDown = DownSlot;
    NavigationLeft = LeftSlot;
    NavigationRight = RightSlot;
    
    // Note: We handle navigation manually in NativeOnKeyDown rather than using SetNavigationRule
    // because SetNavigationRule requires widget names (FName) and manual navigation gives us
    // more control over the navigation behavior.
}

void UPUIngredientSlot::ShowFocusVisuals()
{
    // Show hover text (works for prep/pantry slots even when "empty")
    UpdateHoverTextVisibility(true);

    UpdateIngredientSelectVisibility(true);
}



