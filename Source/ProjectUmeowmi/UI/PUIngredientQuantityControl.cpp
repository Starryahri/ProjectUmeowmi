#include "PUIngredientQuantityControl.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/CheckBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "PUIngredientButtonWidget.h"
#include "PUDishCustomizationWidget.h"
#include "Engine/Engine.h"

UPUIngredientQuantityControl::UPUIngredientQuantityControl(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ParentButtonWidget(nullptr)
    , ParentDishWidget(nullptr)
    , PendingQuantity(1)
{
}

void UPUIngredientQuantityControl::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::NativeConstruct - Widget constructed: %s"), *GetName());
    
    // Bind button click events
    if (DecreaseQuantityButton)
    {
        DecreaseQuantityButton->OnClicked.AddDynamic(this, &UPUIngredientQuantityControl::OnDecreaseQuantityClicked);
    }
    
    if (IncreaseQuantityButton)
    {
        IncreaseQuantityButton->OnClicked.AddDynamic(this, &UPUIngredientQuantityControl::OnIncreaseQuantityClicked);
    }
    
    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UPUIngredientQuantityControl::OnCloseButtonClicked);
    }
    
    // Initialize pending data
    PendingQuantity = IngredientInstance.Quantity;
    PendingPreparations = GetCurrentPreparations();
    
    // Create preparation checkboxes
    CreatePreparationCheckboxes();
    
    // Update display
    UpdateDisplayText();
    UpdateQuantityButtonStates();
    UpdateIngredientImage();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::NativeConstruct - Widget setup complete"));
}

void UPUIngredientQuantityControl::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::NativeDestruct - Widget destructing"));
    
    // Unbind button click events
    if (DecreaseQuantityButton)
    {
        DecreaseQuantityButton->OnClicked.RemoveDynamic(this, &UPUIngredientQuantityControl::OnDecreaseQuantityClicked);
    }
    
    if (IncreaseQuantityButton)
    {
        IncreaseQuantityButton->OnClicked.RemoveDynamic(this, &UPUIngredientQuantityControl::OnIncreaseQuantityClicked);
    }
    
    if (CloseButton)
    {
        CloseButton->OnClicked.RemoveDynamic(this, &UPUIngredientQuantityControl::OnCloseButtonClicked);
    }
    
    // Clean up preparation checkboxes
    PreparationCheckboxes.Empty();
    
    Super::NativeDestruct();
}

void UPUIngredientQuantityControl::InitializeWithInstance(const FIngredientInstance& Instance)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::InitializeWithInstance - Initializing with instance ID: %d, ingredient: %s"), 
        Instance.InstanceID, *Instance.IngredientData.DisplayName.ToString());
    
    IngredientInstance = Instance;
    PendingQuantity = Instance.Quantity;
    PendingPreparations = GetCurrentPreparations();
    
    UpdateDisplayText();
    UpdateQuantityButtonStates();
    UpdateIngredientImage();
    CreatePreparationCheckboxes();
}

void UPUIngredientQuantityControl::UpdateDisplay(const FIngredientInstance& Instance)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::UpdateDisplay - Updating display for instance ID: %d"), Instance.InstanceID);
    
    IngredientInstance = Instance;
    PendingQuantity = Instance.Quantity;
    PendingPreparations = GetCurrentPreparations();
    
    UpdateDisplayText();
    UpdateQuantityButtonStates();
    UpdatePreparationCheckboxes();
}

void UPUIngredientQuantityControl::SetParentButtonWidget(UPUIngredientButtonWidget* ButtonWidget)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::SetParentButtonWidget - Setting parent button widget"));
    
    ParentButtonWidget = ButtonWidget;
}

void UPUIngredientQuantityControl::SetParentDishWidget(UPUDishCustomizationWidget* DishWidget)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::SetParentDishWidget - Setting parent dish widget"));
    
    ParentDishWidget = DishWidget;
}

FGameplayTagContainer UPUIngredientQuantityControl::GetCurrentPreparations() const
{
    // Use convenient field if available, fallback to data field
    return IngredientInstance.Preparations.Num() > 0 ? 
        IngredientInstance.Preparations : IngredientInstance.IngredientData.ActivePreparations;
}

void UPUIngredientQuantityControl::OnDecreaseQuantityClicked()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::OnDecreaseQuantityClicked - Decreasing quantity from %d"), PendingQuantity);
    
    if (PendingQuantity > IngredientInstance.IngredientData.MinQuantity)
    {
        PendingQuantity--;
        UpdateDisplayText();
        UpdateQuantityButtonStates();
        ApplyChangesToDishData();
    }
}

void UPUIngredientQuantityControl::OnIncreaseQuantityClicked()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::OnIncreaseQuantityClicked - Increasing quantity from %d"), PendingQuantity);
    
    if (PendingQuantity < IngredientInstance.IngredientData.MaxQuantity)
    {
        PendingQuantity++;
        UpdateDisplayText();
        UpdateQuantityButtonStates();
        ApplyChangesToDishData();
    }
}

void UPUIngredientQuantityControl::OnCloseButtonClicked()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::OnCloseButtonClicked - Close button clicked"));
    
    // Close this widget
    if (ParentButtonWidget)
    {
        ParentButtonWidget->CloseQuantityWidget();
    }
    else
    {
        RemoveFromParent();
    }
}

void UPUIngredientQuantityControl::OnPreparationCheckboxChanged(bool bIsChecked, FGameplayTag PreparationTag)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::OnPreparationCheckboxChanged - Preparation %s %s"), 
        *PreparationTag.ToString(), bIsChecked ? TEXT("checked") : TEXT("unchecked"));
    
    if (bIsChecked)
    {
        AddPreparation(PreparationTag);
    }
    else
    {
        RemovePreparation(PreparationTag);
    }
    
    ApplyChangesToDishData();
}

void UPUIngredientQuantityControl::UpdateDisplayText()
{
    // Update ingredient name
    if (IngredientNameText)
    {
        FText DisplayName = IngredientInstance.IngredientData.DisplayName;
        if (DisplayName.IsEmpty())
        {
            // Fallback to ingredient tag if display name is empty
            DisplayName = FText::FromString(IngredientInstance.IngredientData.IngredientTag.ToString());
        }
        IngredientNameText->SetText(DisplayName);
    }
    
    // Update quantity display
    if (QuantityDisplayText)
    {
        FText QuantityDisplay = FText::Format(FText::FromString(TEXT("Quantity: {0}")), FText::AsNumber(PendingQuantity));
        QuantityDisplayText->SetText(QuantityDisplay);
    }
    
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::UpdateDisplayText - Updated display for: %s (Qty: %d)"), 
        *IngredientInstance.IngredientData.DisplayName.ToString(), PendingQuantity);
}

void UPUIngredientQuantityControl::UpdatePreparationCheckboxes()
{
    // Update the state of all preparation checkboxes
    for (const auto& CheckboxPair : PreparationCheckboxes)
    {
        const FGameplayTag& PreparationTag = CheckboxPair.Key;
        UCheckBox* Checkbox = CheckboxPair.Value;
        
        if (Checkbox)
        {
            bool bIsApplied = IsPreparationApplied(PreparationTag);
            Checkbox->SetIsChecked(bIsApplied);
        }
    }
}

void UPUIngredientQuantityControl::UpdateIngredientImage()
{
    if (IngredientImage && IngredientInstance.IngredientData.PreviewTexture)
    {
        IngredientImage->SetBrushFromTexture(IngredientInstance.IngredientData.PreviewTexture);
    }
}

void UPUIngredientQuantityControl::UpdateQuantityButtonStates()
{
    if (DecreaseQuantityButton)
    {
        DecreaseQuantityButton->SetIsEnabled(PendingQuantity > IngredientInstance.IngredientData.MinQuantity);
    }
    
    if (IncreaseQuantityButton)
    {
        IncreaseQuantityButton->SetIsEnabled(PendingQuantity < IngredientInstance.IngredientData.MaxQuantity);
    }
}

void UPUIngredientQuantityControl::ApplyChangesToDishData()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::ApplyChangesToDishData - Applying changes to dish data"));
    
    // Update the ingredient instance data
    IngredientInstance.Quantity = PendingQuantity;
    
    // Update preparations in both convenient and data fields
    IngredientInstance.Preparations = PendingPreparations;
    IngredientInstance.IngredientData.ActivePreparations = PendingPreparations;
    
    // Apply changes to the dish data through the parent dish widget
    if (ParentDishWidget)
    {
        // Get current dish data
        FPUDishBase CurrentDishData = ParentDishWidget->GetCurrentDishData();
        
        // Find and update the specific instance
        for (FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
        {
            if (Instance.InstanceID == IngredientInstance.InstanceID)
            {
                Instance = IngredientInstance;
                break;
            }
        }
        
        // Update the dish data
        ParentDishWidget->UpdateDishData(CurrentDishData);
        
        UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::ApplyChangesToDishData - Dish data updated successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUIngredientQuantityControl::ApplyChangesToDishData - No parent dish widget available"));
    }
    
    // Update the parent button widget display
    if (ParentButtonWidget)
    {
        ParentButtonWidget->UpdateDisplay(IngredientInstance);
    }
}

void UPUIngredientQuantityControl::CreatePreparationCheckboxes()
{
    if (!PreparationContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("PUIngredientQuantityControl::CreatePreparationCheckboxes - No preparation container found"));
        return;
    }
    
    // Clear existing checkboxes
    PreparationContainer->ClearChildren();
    PreparationCheckboxes.Empty();
    
    // Get available preparations for this ingredient
    TArray<FGameplayTag> AvailablePreparations = GetAvailablePreparations();
    
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::CreatePreparationCheckboxes - Creating %d preparation checkboxes"), 
        AvailablePreparations.Num());
    
    for (const FGameplayTag& PreparationTag : AvailablePreparations)
    {
        // Create checkbox widget
        UCheckBox* Checkbox = NewObject<UCheckBox>(this);
        if (Checkbox)
        {
            // Set checkbox properties
            Checkbox->SetIsChecked(IsPreparationApplied(PreparationTag));
            
            // Create text for the checkbox
            FString PrepName = PreparationTag.ToString();
            // Extract just the preparation name (after the last period)
            int32 LastPeriodIndex;
            if (PrepName.FindLastChar('.', LastPeriodIndex))
            {
                PrepName = PrepName.RightChop(LastPeriodIndex + 1);
            }
            
            // TODO: Set checkbox text (this would require a custom checkbox widget or text block)
            // For now, we'll just store the checkbox reference
            
            // Add to container
            PreparationContainer->AddChild(Checkbox);
            
            // Store reference
            PreparationCheckboxes.Add(PreparationTag, Checkbox);
            
            UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::CreatePreparationCheckboxes - Created checkbox for: %s"), *PrepName);
        }
    }
}

TArray<FGameplayTag> UPUIngredientQuantityControl::GetAvailablePreparations() const
{
    TArray<FGameplayTag> AvailablePreparations;
    
    // TODO: Get available preparations from the ingredient's preparation data table
    // For now, return a placeholder list
    // This should be populated from the ingredient's preparation data table
    
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::GetAvailablePreparations - Returning placeholder preparations"));
    
    return AvailablePreparations;
}

bool UPUIngredientQuantityControl::IsPreparationApplied(const FGameplayTag& PreparationTag) const
{
    return PendingPreparations.HasTag(PreparationTag);
}

void UPUIngredientQuantityControl::AddPreparation(const FGameplayTag& PreparationTag)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::AddPreparation - Adding preparation: %s"), *PreparationTag.ToString());
    
    PendingPreparations.AddTag(PreparationTag);
}

void UPUIngredientQuantityControl::RemovePreparation(const FGameplayTag& PreparationTag)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::RemovePreparation - Removing preparation: %s"), *PreparationTag.ToString());
    
    PendingPreparations.RemoveTag(PreparationTag);
} 