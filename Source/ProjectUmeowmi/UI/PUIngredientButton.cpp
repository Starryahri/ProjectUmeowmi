#include "PUIngredientButton.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "PUIngredientDragDropOperation.h"
#include "PUCookingStageWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"

UPUIngredientButton::UPUIngredientButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPUIngredientButton::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Warning, TEXT("⚠️ DEPRECATED: PUIngredientButton is deprecated. Use PUIngredientSlot instead. Widget constructed: %s"), *GetName());
    
    // Bind button events
    if (IngredientButton)
    {
        IngredientButton->OnClicked.AddDynamic(this, &UPUIngredientButton::OnIngredientButtonClickedInternal);
        IngredientButton->OnHovered.AddDynamic(this, &UPUIngredientButton::OnIngredientButtonHoveredInternal);
        IngredientButton->OnUnhovered.AddDynamic(this, &UPUIngredientButton::OnIngredientButtonUnhoveredInternal);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::NativeConstruct - Button events bound"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUIngredientButton::NativeConstruct - IngredientButton component not found"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::NativeConstruct - Widget setup complete"));
}

void UPUIngredientButton::NativeDestruct()
{
    // UE_LOG(LogTemp, Display, TEXT("PUIngredientButton::NativeDestruct - Widget destructing"));
    
    // Unbind button events
    if (IngredientButton)
    {
        IngredientButton->OnClicked.RemoveDynamic(this, &UPUIngredientButton::OnIngredientButtonClickedInternal);
        IngredientButton->OnHovered.RemoveDynamic(this, &UPUIngredientButton::OnIngredientButtonHoveredInternal);
        IngredientButton->OnUnhovered.RemoveDynamic(this, &UPUIngredientButton::OnIngredientButtonUnhoveredInternal);
    }
    
    Super::NativeDestruct();
}

void UPUIngredientButton::SetIngredientData(const FPUIngredientBase& InIngredientData)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Setting ingredient data: %s"), 
        *InIngredientData.DisplayName.ToString());
    
    // Update ingredient data
    IngredientData = InIngredientData;
    
    // Update UI components
    if (IngredientNameText)
    {
        IngredientNameText->SetText(IngredientData.DisplayName);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Updated ingredient name text"));
    }
    
    if (IngredientIcon)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Ingredient icon component found"));
        
        if (IngredientData.PreviewTexture)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Preview texture found, setting icon"));
            IngredientIcon->SetBrushFromTexture(IngredientData.PreviewTexture);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Successfully set texture: %p"), IngredientData.PreviewTexture);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🎯 PUIngredientButton::SetIngredientData - Preview texture is null for ingredient: %s"), 
                *IngredientData.DisplayName.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯 PUIngredientButton::SetIngredientData - Ingredient icon component not found"));
    }
    
    // Call Blueprint event
    OnIngredientDataSet(InIngredientData);
    
    // Re-hide text after Blueprint event in case it was overridden
    HideAllText();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Ingredient data set successfully"));
}

void UPUIngredientButton::OnIngredientButtonClickedInternal()
{
    // If drag is enabled, ignore click events to prevent double-click issues
    if (bDragEnabled)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonClickedInternal - Ignoring click event (drag enabled) for ingredient: %s"), 
            *IngredientData.DisplayName.ToString());
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonClickedInternal - Button clicked for ingredient: %s"), 
        *IngredientData.DisplayName.ToString());
    
    // Broadcast the click event with ingredient data
    OnIngredientButtonClicked.Broadcast(IngredientData);
    
    // Call Blueprint event
    OnButtonClicked();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonClickedInternal - Click event broadcasted"));
}

void UPUIngredientButton::OnIngredientButtonHoveredInternal()
{
    // UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonHoveredInternal - Button hovered for ingredient: %s"), 
    //     *IngredientData.DisplayName.ToString());
    
    // Broadcast the hover event with ingredient data
    OnIngredientButtonHovered.Broadcast(IngredientData);
    
    // Call Blueprint event
    OnButtonHovered();
    
    // UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonHoveredInternal - Hover event broadcasted"));
}

void UPUIngredientButton::OnIngredientButtonUnhoveredInternal()
{
    // UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonUnhoveredInternal - Button unhovered for ingredient: %s"), 
    //     *IngredientData.DisplayName.ToString());
    
    // Broadcast the unhover event with ingredient data
    OnIngredientButtonUnhovered.Broadcast(IngredientData);
    
    // Call Blueprint event
    OnButtonUnhovered();
    
    // UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonUnhoveredInternal - Unhover event broadcasted"));
}

FReply UPUIngredientButton::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::NativeOnPreviewMouseButtonDown - Preview mouse button down on ingredient: %s (Drag enabled: %s)"), 
        *IngredientData.DisplayName.ToString(), bDragEnabled ? TEXT("TRUE") : TEXT("FALSE"));
    
    // Only handle left mouse button and only if drag is enabled
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDragEnabled)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::NativeOnPreviewMouseButtonDown - Starting drag detection for ingredient: %s"), 
            *IngredientData.DisplayName.ToString());
        
        // Start drag detection - this will call the Blueprint OnDragDetected event
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }
    
    return FReply::Unhandled();
}


void UPUIngredientButton::SetDragEnabled(bool bEnabled)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetDragEnabled - Setting drag enabled to %s for ingredient: %s"), 
        bEnabled ? TEXT("TRUE") : TEXT("FALSE"), *IngredientData.DisplayName.ToString());
    
    bDragEnabled = bEnabled;
    
    // Simple approach - just set the drag enabled flag
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetDragEnabled - Drag enabled set to: %s"), bEnabled ? TEXT("TRUE") : TEXT("FALSE"));
}


int32 UPUIngredientButton::GenerateUniqueInstanceID() const
{
    // Call the static GUID-based function from PUCookingStageWidget
    int32 UniqueID = UPUCookingStageWidget::GenerateGUIDBasedInstanceID();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::GenerateUniqueInstanceID - Generated unique ID %d for ingredient: %s"), 
        UniqueID, *IngredientData.DisplayName.ToString());
    
    return UniqueID;
}

// Plating-specific functions
void UPUIngredientButton::SetIngredientInstance(const FIngredientInstance& InInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::SetIngredientInstance - Setting ingredient instance: %s (ID: %d, Qty: %d)"), 
        *InInstance.IngredientData.DisplayName.ToString(), InInstance.InstanceID, InInstance.Quantity);
    
    IngredientInstance = InInstance;
    MaxQuantity = InInstance.Quantity;
    RemainingQuantity = MaxQuantity;
    
    // Update the base ingredient data as well
    IngredientData = InInstance.IngredientData;
    
    // Update all displays
    UpdatePlatingDisplay();
    
    // Call Blueprint event
    OnIngredientInstanceSet(InInstance);
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::SetIngredientInstance - Instance set successfully"));
}

void UPUIngredientButton::DecreaseQuantity()
{
    if (RemainingQuantity > 0)
    {
        RemainingQuantity--;
        UpdateQuantityDisplay();
        
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::DecreaseQuantity - Quantity decreased to %d"), RemainingQuantity);
        
        // Call Blueprint event
        OnQuantityChanged(RemainingQuantity);
        
        // Update button state
        if (IngredientButton)
        {
            IngredientButton->SetIsEnabled(RemainingQuantity > 0);
        }
    }
}


void UPUIngredientButton::ResetQuantity()
{
    RemainingQuantity = MaxQuantity;
    UpdateQuantityDisplay();
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::ResetQuantity - Quantity reset to %d for ingredient: %s"), 
        RemainingQuantity, *IngredientData.DisplayName.ToString());
    
    // Call Blueprint event
    OnQuantityChanged(RemainingQuantity);
    
    // Update button state
    if (IngredientButton)
    {
        IngredientButton->SetIsEnabled(RemainingQuantity > 0);
    }
}

void UPUIngredientButton::UpdatePlatingDisplay()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::UpdatePlatingDisplay - Updating plating display"));
    
    // Update the ingredient icon/texture
    if (IngredientIcon)
    {
        IngredientIcon->SetBrushFromTexture(IngredientInstance.IngredientData.PreviewTexture);
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::UpdatePlatingDisplay - Updated icon texture"));
    }
    
    // Update the main ingredient name to include preparation state
    if (IngredientNameText)
    {
        FString DisplayName = IngredientInstance.IngredientData.DisplayName.ToString();
        
        // Add preparation state to the name
        FString PrepText = GetPreparationDisplayText();
        if (!PrepText.IsEmpty())
        {
            DisplayName = FString::Printf(TEXT("%s (%s)"), *DisplayName, *PrepText);
        }
        
        IngredientNameText->SetText(FText::FromString(DisplayName));
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::UpdatePlatingDisplay - Updated name: %s"), *DisplayName);
    }
    
    // Update quantity and preparation displays
    UpdateQuantityDisplay();
    UpdatePreparationDisplay();
}

void UPUIngredientButton::UpdateQuantityDisplay()
{
    if (QuantityText)
    {
        FString QuantityString = FString::Printf(TEXT("x%d"), RemainingQuantity);
        QuantityText->SetText(FText::FromString(QuantityString));
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::UpdateQuantityDisplay - Updated quantity: %s"), *QuantityString);
    }
}

void UPUIngredientButton::UpdatePreparationDisplay()
{
    if (PreparationText)
    {
        FString IconText = GetPreparationIconText();
        PreparationText->SetText(FText::FromString(IconText));
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::UpdatePreparationDisplay - Updated preparation icons: %s"), *IconText);
    }
    
    // Call Blueprint event
    OnPreparationStateChanged();
}

void UPUIngredientButton::SpawnIngredientAtPosition(const FVector2D& ScreenPosition)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::SpawnIngredientAtPosition - START - Ingredient %s at screen position (%.2f,%.2f)"), 
        *IngredientInstance.IngredientData.DisplayName.ToString(), ScreenPosition.X, ScreenPosition.Y);

    // Convert screen position to world position using raycast
    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUIngredientButton::SpawnIngredientAtPosition - No player controller"));
        return;
    }

    // Get camera location and rotation
    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
    
    // Get viewport size
    int32 ViewportSizeX, ViewportSizeY;
    PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::SpawnIngredientAtPosition - Viewport: %dx%d, Mouse: (%.0f,%.0f)"), 
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
    for (AActor* Actor : FoundActors)
    {
        if (Actor)
        {
            UPUDishCustomizationComponent* DishComponent = Actor->FindComponentByClass<UPUDishCustomizationComponent>();
            if (DishComponent)
            {
                UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::SpawnIngredientAtPosition - Calling SpawnIngredientIn3DByInstanceID on customization component"));
                DishComponent->SpawnIngredientIn3DByInstanceID(IngredientInstance.InstanceID, SpawnPosition);
                UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::SpawnIngredientAtPosition - SpawnIngredientIn3DByInstanceID call completed"));
                break;
            }
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::SpawnIngredientAtPosition - END"));
}

UPUIngredientDragDropOperation* UPUIngredientButton::CreateIngredientDragDropOperation() const
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::CreateIngredientDragDropOperation - Creating drag operation for ingredient %s (ID: %d, Qty: %d)"), 
        *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID, IngredientInstance.Quantity);

    // Create the drag drop operation
    UPUIngredientDragDropOperation* DragOperation = NewObject<UPUIngredientDragDropOperation>(GetWorld(), UPUIngredientDragDropOperation::StaticClass());

    if (DragOperation)
    {
        // Set up the drag operation with ingredient instance
        DragOperation->SetupIngredientDrag(IngredientInstance);
        
        UE_LOG(LogTemp, Display, TEXT("✅ PUIngredientButton::CreateIngredientDragDropOperation - Successfully created drag operation"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PUIngredientButton::CreateIngredientDragDropOperation - Failed to create drag operation"));
    }

    return DragOperation;
}

FString UPUIngredientButton::GetPreparationDisplayText() const
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

FString UPUIngredientButton::GetPreparationIconText() const
{
    if (IngredientInstance.Preparations.Num() == 0)
    {
        return TEXT("");
    }
    
    FString IconString;
    for (const FGameplayTag& Prep : IngredientInstance.Preparations)
    {
        FString PrepName = Prep.ToString().Replace(TEXT("Prep."), TEXT(""));
        
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::GetPreparationIconText - Processing preparation tag: %s (cleaned: %s)"), 
            *Prep.ToString(), *PrepName);
        
        // Map preparation tags to text abbreviations
        if (PrepName.Contains(TEXT("Dehydrate")) || PrepName.Contains(TEXT("Dried")))
        {
            IconString += TEXT("[D]");
            UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::GetPreparationIconText - Matched Dehydrate/Dried: [D]"));
        }
        else if (PrepName.Contains(TEXT("Mince")) || PrepName.Contains(TEXT("Minced")))
        {
            IconString += TEXT("[M]");
            UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::GetPreparationIconText - Matched Mince/Minced: [M]"));
        }
        else if (PrepName.Contains(TEXT("Boiled")))
        {
            IconString += TEXT("[B]");
            UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::GetPreparationIconText - Matched Boiled: [B]"));
        }
        else if (PrepName.Contains(TEXT("Chopped")))
        {
            IconString += TEXT("[C]");
            UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::GetPreparationIconText - Matched Chopped: [C]"));
        }
        else if (PrepName.Contains(TEXT("Caramelized")))
        {
            IconString += TEXT("[CR]");
            UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::GetPreparationIconText - Matched Caramelized: [CR]"));
        }
        else
        {
            // Default abbreviation for unknown preparations
            IconString += TEXT("[?]");
            UE_LOG(LogTemp, Warning, TEXT("🎯 PUIngredientButton::GetPreparationIconText - Unknown preparation: %s (cleaned: %s)"), 
                *Prep.ToString(), *PrepName);
        }
    }
    
    return IconString;
}

void UPUIngredientButton::SetTextVisibility(bool bShowQuantity, bool bShowDescription)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetTextVisibility - Setting text visibility: Quantity=%s, Description=%s"), 
        bShowQuantity ? TEXT("TRUE") : TEXT("FALSE"), bShowDescription ? TEXT("TRUE") : TEXT("FALSE"));
    
    // Control quantity text visibility
    if (QuantityText)
    {
        QuantityText->SetVisibility(bShowQuantity ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetTextVisibility - Set quantity text visibility to: %s"), 
            bShowQuantity ? TEXT("Visible") : TEXT("Hidden"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUIngredientButton::SetTextVisibility - QuantityText component not found for ingredient: %s"), 
            *IngredientData.DisplayName.ToString());
    }
    
    // Control preparation/description text visibility
    if (PreparationText)
    {
        PreparationText->SetVisibility(bShowDescription ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetTextVisibility - Set preparation text visibility to: %s"), 
            bShowDescription ? TEXT("Visible") : TEXT("Hidden"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUIngredientButton::SetTextVisibility - PreparationText component not found for ingredient: %s"), 
            *IngredientData.DisplayName.ToString());
    }
}

void UPUIngredientButton::HideAllText()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::HideAllText - Hiding all text elements"));
    LogTextComponentStatus();
    SetTextVisibility(false, false);
    
    // Force hide using different visibility modes
    if (QuantityText)
    {
        QuantityText->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::HideAllText - Force collapsed quantity text"));
    }
    
    if (PreparationText)
    {
        PreparationText->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::HideAllText - Force collapsed preparation text"));
    }
}

void UPUIngredientButton::ShowAllText()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::ShowAllText - Showing all text elements"));
    LogTextComponentStatus();
    SetTextVisibility(true, true);
}

void UPUIngredientButton::LogTextComponentStatus()
{
    UE_LOG(LogTemp, Display, TEXT("🔍 PUIngredientButton::LogTextComponentStatus - Checking text component status for ingredient: %s"), 
        *IngredientData.DisplayName.ToString());
    
    UE_LOG(LogTemp, Display, TEXT("🔍 QuantityText: %s"), QuantityText ? TEXT("FOUND") : TEXT("NULL"));
    UE_LOG(LogTemp, Display, TEXT("🔍 PreparationText: %s"), PreparationText ? TEXT("FOUND") : TEXT("NULL"));
    UE_LOG(LogTemp, Display, TEXT("🔍 IngredientNameText: %s"), IngredientNameText ? TEXT("FOUND") : TEXT("NULL"));
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
