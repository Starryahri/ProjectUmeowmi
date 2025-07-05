#include "PUDishCustomizationWidget.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "PUIngredientButtonWidget.h"
#include "PUIngredientQuantityControl.h"
#include "Components/UniformGridPanel.h"
#include "Components/ScrollBox.h"

UPUDishCustomizationWidget::UPUDishCustomizationWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , CustomizationComponent(nullptr)
{
}

void UPUDishCustomizationWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget constructed: %s"), *GetName());
    
    // Note: Don't subscribe to events here - the component reference isn't set yet
    // Subscription will happen in SetCustomizationComponent()
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget setup complete"));
}

void UPUDishCustomizationWidget::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::NativeDestruct - Widget destructing"));
    
    // Unsubscribe from events
    UnsubscribeFromEvents();
    
    Super::NativeDestruct();
}

void UPUDishCustomizationWidget::OnInitialDishDataReceived(const FPUDishBase& InitialDishData)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnInitialDishDataReceived - Received initial dish data: %s with %d ingredients"), 
        *InitialDishData.DisplayName.ToString(), InitialDishData.IngredientInstances.Num());
    
    // Update current dish data
    CurrentDishData = InitialDishData;
    
    // Create ingredient buttons for all instances
    CreateIngredientButtons();
    
    // Call the Blueprint event
    OnDishDataReceived(InitialDishData);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnInitialDishDataReceived - Blueprint event OnDishDataReceived called"));
}

void UPUDishCustomizationWidget::OnDishDataUpdated(const FPUDishBase& UpdatedDishData)
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::OnDishDataUpdated - Received dish data update: %s with %d ingredients"), 
        *UpdatedDishData.DisplayName.ToString(), UpdatedDishData.IngredientInstances.Num());
    
    // Update current dish data
    CurrentDishData = UpdatedDishData;
    
    // Update ingredient buttons to reflect changes
    UpdateIngredientButtons();
    
    // Call the Blueprint event
    OnDishDataChanged(UpdatedDishData);
}

void UPUDishCustomizationWidget::OnCustomizationEnded()
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::OnCustomizationEnded - Customization ended"));
    
    // Clean up all ingredient buttons and quantity controls
    ClearIngredientButtons();
    ClearQuantityControlScrollBox();
    
    // Call the Blueprint event
    OnCustomizationModeEnded();
}

void UPUDishCustomizationWidget::SetCustomizationComponent(UPUDishCustomizationComponent* Component)
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::SetCustomizationComponent - Setting component reference"));
    
    // Unsubscribe from previous component if any
    UnsubscribeFromEvents();
    
    // Set the new component reference
    CustomizationComponent = Component;
    
    // Subscribe to the new component's events
    SubscribeToEvents();
}

void UPUDishCustomizationWidget::UpdateDishData(const FPUDishBase& NewDishData)
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::UpdateDishData - Updating dish data: %s with %d ingredients"), 
        *NewDishData.DisplayName.ToString(), NewDishData.IngredientInstances.Num());
    
    // Update local data
    CurrentDishData = NewDishData;
    
    // Sync back to the customization component
    if (CustomizationComponent)
    {
        CustomizationComponent->SyncDishDataFromUI(NewDishData);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUDishCustomizationWidget::UpdateDishData - No customization component reference"));
    }
}

void UPUDishCustomizationWidget::SubscribeToEvents()
{
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::SubscribeToEvents - Subscribing to customization component events"));
        
        // Subscribe to the component's events
        CustomizationComponent->OnInitialDishDataReceived.AddDynamic(this, &UPUDishCustomizationWidget::OnInitialDishDataReceived);
        CustomizationComponent->OnDishDataUpdated.AddDynamic(this, &UPUDishCustomizationWidget::OnDishDataUpdated);
        CustomizationComponent->OnCustomizationEnded.AddDynamic(this, &UPUDishCustomizationWidget::OnCustomizationEnded);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUDishCustomizationWidget::SubscribeToEvents - No customization component to subscribe to"));
    }
}

void UPUDishCustomizationWidget::UnsubscribeFromEvents()
{
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::UnsubscribeFromEvents - Unsubscribing from customization component events"));
        
        // Unsubscribe from the component's events
        CustomizationComponent->OnInitialDishDataReceived.RemoveDynamic(this, &UPUDishCustomizationWidget::OnInitialDishDataReceived);
        CustomizationComponent->OnDishDataUpdated.RemoveDynamic(this, &UPUDishCustomizationWidget::OnDishDataUpdated);
        CustomizationComponent->OnCustomizationEnded.RemoveDynamic(this, &UPUDishCustomizationWidget::OnCustomizationEnded);
    }
}

// Ingredient Button Management
void UPUDishCustomizationWidget::CreateIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::CreateIngredientButtons - Creating ingredient buttons for %d instances"), 
        CurrentDishData.IngredientInstances.Num());
    
    // Clear existing buttons
    ClearIngredientButtons();
    
    // Create a button for each ingredient instance
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        CreateIngredientButtonForInstance(Instance);
    }
    
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::CreateIngredientButtons - Created %d ingredient buttons"), IngredientButtons.Num());
}

void UPUDishCustomizationWidget::UpdateIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::UpdateIngredientButtons - Updating ingredient buttons"));
    
    // Update existing buttons and create new ones as needed
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        UpdateIngredientButtonForInstance(Instance);
    }
    
    // Remove buttons for instances that no longer exist
    TArray<int32> CurrentInstanceIDs;
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        CurrentInstanceIDs.Add(Instance.InstanceID);
    }
    
    for (int32 i = IngredientButtons.Num() - 1; i >= 0; --i)
    {
        if (IngredientButtons[i] && !CurrentInstanceIDs.Contains(IngredientButtons[i]->GetInstanceID()))
        {
            RemoveIngredientButtonForInstance(IngredientButtons[i]->GetInstanceID());
        }
    }
}

void UPUDishCustomizationWidget::ClearIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::ClearIngredientButtons - Clearing all ingredient buttons"));
    
    // Remove all buttons from the grid
    if (IngredientButtonGrid)
    {
        IngredientButtonGrid->ClearChildren();
    }
    
    // Clean up button references
    for (UPUIngredientButtonWidget* Button : IngredientButtons)
    {
        if (Button)
        {
            Button->RemoveFromParent();
        }
    }
    
    IngredientButtons.Empty();
}

void UPUDishCustomizationWidget::CreateIngredientButtonForInstance(const FIngredientInstance& Instance)
{
    if (!IngredientButtonGrid || !IngredientButtonWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("PUDishCustomizationWidget::CreateIngredientButtonForInstance - Missing grid or widget class"));
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::CreateIngredientButtonForInstance - Creating button for instance %d"), Instance.InstanceID);
    
    // Create the button widget
    UPUIngredientButtonWidget* ButtonWidget = CreateWidget<UPUIngredientButtonWidget>(this, IngredientButtonWidgetClass);
    if (ButtonWidget)
    {
        // Initialize the button
        ButtonWidget->InitializeWithInstance(Instance);
        ButtonWidget->SetParentDishWidget(this);
        
        // Add to the grid
        IngredientButtonGrid->AddChild(ButtonWidget);
        
        // Store reference
        IngredientButtons.Add(ButtonWidget);
        
        UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::CreateIngredientButtonForInstance - Button created successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUDishCustomizationWidget::CreateIngredientButtonForInstance - Failed to create button widget"));
    }
}

void UPUDishCustomizationWidget::UpdateIngredientButtonForInstance(const FIngredientInstance& Instance)
{
    // Find the existing button for this instance
    for (UPUIngredientButtonWidget* Button : IngredientButtons)
    {
        if (Button && Button->GetInstanceID() == Instance.InstanceID)
        {
            Button->UpdateDisplay(Instance);
            return;
        }
    }
    
    // If no button found, create a new one
    CreateIngredientButtonForInstance(Instance);
}

void UPUDishCustomizationWidget::RemoveIngredientButtonForInstance(int32 InstanceID)
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::RemoveIngredientButtonForInstance - Removing button for instance %d"), InstanceID);
    
    for (int32 i = 0; i < IngredientButtons.Num(); ++i)
    {
        if (IngredientButtons[i] && IngredientButtons[i]->GetInstanceID() == InstanceID)
        {
            // Remove from grid
            if (IngredientButtonGrid)
            {
                IngredientButtonGrid->RemoveChild(IngredientButtons[i]);
            }
            
            // Clean up
            IngredientButtons[i]->RemoveFromParent();
            IngredientButtons.RemoveAt(i);
            break;
        }
    }
}

// Quantity Control Management
void UPUDishCustomizationWidget::AddQuantityControlToScrollBox(UPUIngredientQuantityControl* QuantityControl)
{
    if (!QuantityControlScrollBox || !QuantityControl)
    {
        UE_LOG(LogTemp, Warning, TEXT("PUDishCustomizationWidget::AddQuantityControlToScrollBox - Missing scroll box or quantity control"));
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::AddQuantityControlToScrollBox - Adding quantity control to scroll box"));
    
    // Add to scroll box
    QuantityControlScrollBox->AddChild(QuantityControl);
    
    // Store reference
    ActiveQuantityControls.Add(QuantityControl);
}

void UPUDishCustomizationWidget::RemoveQuantityControlFromScrollBox(UPUIngredientQuantityControl* QuantityControl)
{
    if (!QuantityControl)
    {
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::RemoveQuantityControlFromScrollBox - Removing quantity control from scroll box"));
    
    // Remove from scroll box
    if (QuantityControlScrollBox)
    {
        QuantityControlScrollBox->RemoveChild(QuantityControl);
    }
    
    // Remove from active controls
    ActiveQuantityControls.Remove(QuantityControl);
    
    // Clean up
    QuantityControl->RemoveFromParent();
}

void UPUDishCustomizationWidget::ClearQuantityControlScrollBox()
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::ClearQuantityControlScrollBox - Clearing quantity control scroll box"));
    
    // Remove all controls from scroll box
    if (QuantityControlScrollBox)
    {
        QuantityControlScrollBox->ClearChildren();
    }
    
    // Clean up control references
    for (UPUIngredientQuantityControl* Control : ActiveQuantityControls)
    {
        if (Control)
        {
            Control->RemoveFromParent();
        }
    }
    
    ActiveQuantityControls.Empty();
}

UPUIngredientQuantityControl* UPUDishCustomizationWidget::CreateQuantityControlWidget(const FIngredientInstance& Instance)
{
    if (!QuantityControlWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("PUDishCustomizationWidget::CreateQuantityControlWidget - Missing quantity control widget class"));
        return nullptr;
    }
    
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::CreateQuantityControlWidget - Creating quantity control for instance %d"), Instance.InstanceID);
    
    // Create the quantity control widget
    UPUIngredientQuantityControl* QuantityControl = CreateWidget<UPUIngredientQuantityControl>(this, QuantityControlWidgetClass);
    if (QuantityControl)
    {
        // Initialize the control
        QuantityControl->InitializeWithInstance(Instance);
        QuantityControl->SetParentDishWidget(this);
        
        UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::CreateQuantityControlWidget - Quantity control created successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUDishCustomizationWidget::CreateQuantityControlWidget - Failed to create quantity control widget"));
    }
    
    return QuantityControl;
} 