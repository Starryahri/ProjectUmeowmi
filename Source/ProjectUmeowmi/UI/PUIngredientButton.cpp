#include "PUIngredientButton.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "PUIngredientDragDropOperation.h"
#include "PUCookingStageWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "GameplayTagContainer.h"

UPUIngredientButton::UPUIngredientButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPUIngredientButton::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::NativeConstruct - Widget constructed: %s"), *GetName());
    
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
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Ingredient data set successfully"));
}

void UPUIngredientButton::OnIngredientButtonClickedInternal()
{
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

FReply UPUIngredientButton::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::NativeOnMouseButtonDown - Mouse button down on ingredient: %s (Drag enabled: %s)"), 
        *IngredientData.DisplayName.ToString(), bDragEnabled ? TEXT("TRUE") : TEXT("FALSE"));
    
    // Only handle left mouse button and only if drag is enabled
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDragEnabled)
    {
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
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUIngredientButton::ResetQuantity - Quantity reset to %d"), RemainingQuantity);
    
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
        
        // Map preparation tags to icons
        if (PrepName.Contains(TEXT("Dried")))
        {
            IconString += TEXT("🌡️");
        }
        else if (PrepName.Contains(TEXT("Minced")))
        {
            IconString += TEXT("🔪");
        }
        else if (PrepName.Contains(TEXT("Boiled")))
        {
            IconString += TEXT("💧");
        }
        else if (PrepName.Contains(TEXT("Chopped")))
        {
            IconString += TEXT("✂️");
        }
        else if (PrepName.Contains(TEXT("Caramelized")))
        {
            IconString += TEXT("🔥");
        }
        else
        {
            // Default icon for unknown preparations
            IconString += TEXT("⚙️");
        }
    }
    
    return IconString;
} 