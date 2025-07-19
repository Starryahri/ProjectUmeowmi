#include "PUDishCustomizationWidget.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "../DishCustomization/PUDishBlueprintLibrary.h"
#include "PUIngredientButton.h"
#include "PUIngredientQuantityControl.h"

UPUDishCustomizationWidget::UPUDishCustomizationWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , CustomizationComponent(nullptr)
{
}

void UPUDishCustomizationWidget::NativeConstruct()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - STARTING WIDGET CONSTRUCTION"));
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget name: %s"), *GetName());
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget class: %s"), *GetClass()->GetName());
    
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Super::NativeConstruct completed"));
    
    // Check if we're in the game world
    UWorld* World = GetWorld();
    if (World)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - World valid: %s"), *World->GetName());
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - World type: %s"), 
            World->IsGameWorld() ? TEXT("Game World") : TEXT("Editor World"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::NativeConstruct - No World available"));
    }
    
    // Check widget visibility
    if (IsVisible())
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget is visible"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::NativeConstruct - Widget is NOT visible"));
    }
    
    // Check if we're in viewport
    if (IsInViewport())
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget is in viewport"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::NativeConstruct - Widget is NOT in viewport"));
    }
    
    // Note: Don't subscribe to events here - the component reference isn't set yet
    // Subscription will happen in SetCustomizationComponent()
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - WIDGET CONSTRUCTION COMPLETED"));
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
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - RECEIVED INITIAL DISH DATA"));
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Widget name: %s"), *GetName());
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Received initial dish data: %s with %d ingredients"), 
        *InitialDishData.DisplayName.ToString(), InitialDishData.IngredientInstances.Num());
    
    // Log dish details
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Dish tag: %s"), *InitialDishData.DishTag.ToString());
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Dish display name: %s"), *InitialDishData.DisplayName.ToString());
    
    // Log ingredient details
    for (int32 i = 0; i < InitialDishData.IngredientInstances.Num(); i++)
    {
        const FIngredientInstance& Instance = InitialDishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Ingredient %d: %s (Qty: %d, ID: %d)"), 
            i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity, Instance.InstanceID);
    }
    
    // Update current dish data
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Updating current dish data"));
    CurrentDishData = InitialDishData;
    
    // Call the Blueprint event
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Calling Blueprint event OnDishDataReceived"));
    OnDishDataReceived(InitialDishData);
    
    UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::OnInitialDishDataReceived - INITIAL DISH DATA PROCESSED SUCCESSFULLY"));
}

void UPUDishCustomizationWidget::OnDishDataUpdated(const FPUDishBase& UpdatedDishData)
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::OnDishDataUpdated - Received dish data update: %s with %d ingredients"), 
        *UpdatedDishData.DisplayName.ToString(), UpdatedDishData.IngredientInstances.Num());
    
    // Update current dish data
    CurrentDishData = UpdatedDishData;
    
    // Call the Blueprint event
    OnDishDataChanged(UpdatedDishData);
}

void UPUDishCustomizationWidget::OnCustomizationEnded()
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::OnCustomizationEnded - Customization ended"));
    
    // Call the Blueprint event
    OnCustomizationModeEnded();
}

void UPUDishCustomizationWidget::SetCustomizationComponent(UPUDishCustomizationComponent* Component)
{
    UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - STARTING COMPONENT CONNECTION"));
    UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Widget name: %s"), *GetName());
    UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Component: %s"), 
        Component ? *Component->GetName() : TEXT("NULL"));
    
    // Unsubscribe from previous component if any
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Unsubscribing from previous component: %s"), *CustomizationComponent->GetName());
        UnsubscribeFromEvents();
    }
    
    // Set the new component reference
    CustomizationComponent = Component;
    
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::SetCustomizationComponent - Component reference set successfully"));
        
        // Subscribe to the new component's events
        UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Subscribing to component events"));
        SubscribeToEvents();
        UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::SetCustomizationComponent - Event subscription completed"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::SetCustomizationComponent - Component reference is NULL"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - COMPONENT CONNECTION COMPLETED"));
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

void UPUDishCustomizationWidget::CreateIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Creating ingredient buttons"));
    
    // Get ingredient data from the component
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Getting ingredient data from component"));
        TArray<FPUIngredientBase> IngredientData = CustomizationComponent->GetIngredientData();
        
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Retrieved %d ingredients from component"), IngredientData.Num());
        
        // Call Blueprint event for each ingredient to create buttons
        for (const FPUIngredientBase& Ingredient : IngredientData)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Creating button for ingredient: %s"), 
                *Ingredient.DisplayName.ToString());
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Ingredient tag: %s"), 
                *Ingredient.IngredientTag.ToString());
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Preview texture valid: %s"), 
                Ingredient.PreviewTexture ? TEXT("Yes") : TEXT("No"));
            if (Ingredient.PreviewTexture)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Preview texture name: %s"), 
                    *Ingredient.PreviewTexture->GetName());
            }
            
            // Call Blueprint event to create the button with ingredient data
            OnIngredientButtonCreated(nullptr, Ingredient);
        }
        
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Ingredient button creation complete"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientButtons - No customization component available"));
    }
}

void UPUDishCustomizationWidget::OnIngredientButtonClicked(const FPUIngredientBase& IngredientData)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - Ingredient button clicked: %s"), 
        *IngredientData.DisplayName.ToString());
    
    // Create a new ingredient instance
    CreateIngredientInstance(IngredientData);
}

void UPUDishCustomizationWidget::OnQuantityControlChanged(const FIngredientInstance& IngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlChanged - Quantity control changed for instance: %d"), 
        IngredientInstance.InstanceID);
    
    // Log the preparations in the received ingredient instance
    TArray<FGameplayTag> CurrentPreparations;
    IngredientInstance.Preparations.GetGameplayTagArray(CurrentPreparations);
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlChanged - Received instance %d with %d preparations:"), 
        IngredientInstance.InstanceID, CurrentPreparations.Num());
    for (const FGameplayTag& Prep : CurrentPreparations)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlChanged -   - %s"), *Prep.ToString());
    }
    
    // Update the ingredient instance in the dish data
    UpdateIngredientInstance(IngredientInstance);
}

void UPUDishCustomizationWidget::OnQuantityControlRemoved(int32 InstanceID, UPUIngredientQuantityControl* QuantityControlWidget)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlRemoved - Quantity control removed for instance: %d"), InstanceID);
    
    // Remove the ingredient instance from the dish data
    RemoveIngredientInstance(InstanceID);
    
    // Remove the widget from viewport
    if (QuantityControlWidget)
    {
        QuantityControlWidget->RemoveFromParent();
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlRemoved - Widget removed from viewport"));
    }
}

void UPUDishCustomizationWidget::CreateIngredientInstance(const FPUIngredientBase& IngredientData)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientInstance - Creating ingredient instance: %s"), 
        *IngredientData.DisplayName.ToString());
    
    // Create a new ingredient instance using the blueprint library
    FIngredientInstance NewInstance = UPUDishBlueprintLibrary::AddIngredient(CurrentDishData, IngredientData.IngredientTag);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientInstance - Created instance with ID: %d"), NewInstance.InstanceID);
    
    // Update the dish data
    UpdateDishData(CurrentDishData);
    
    // Call Blueprint event to create quantity control with ingredient instance data
    OnQuantityControlCreated(nullptr, NewInstance); // Pass the ingredient instance data
}

void UPUDishCustomizationWidget::UpdateIngredientInstance(const FIngredientInstance& IngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Updating ingredient instance: %d"), 
        IngredientInstance.InstanceID);
    
    // Log the preparations before updating
    TArray<FGameplayTag> PreparationsBefore;
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == IngredientInstance.InstanceID)
        {
            CurrentDishData.IngredientInstances[i].Preparations.GetGameplayTagArray(PreparationsBefore);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Instance %d had %d preparations before update:"), 
                IngredientInstance.InstanceID, PreparationsBefore.Num());
            for (const FGameplayTag& Prep : PreparationsBefore)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance -   - %s"), *Prep.ToString());
            }
            break;
        }
    }
    
    // Find and update the ingredient instance in the dish data
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == IngredientInstance.InstanceID)
        {
            CurrentDishData.IngredientInstances[i] = IngredientInstance;
            
            // Log the preparations after updating
            TArray<FGameplayTag> PreparationsAfter;
            IngredientInstance.Preparations.GetGameplayTagArray(PreparationsAfter);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Instance %d now has %d preparations after update:"), 
                IngredientInstance.InstanceID, PreparationsAfter.Num());
            for (const FGameplayTag& Prep : PreparationsAfter)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance -   - %s"), *Prep.ToString());
            }
            
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Instance updated successfully"));
            break;
        }
    }
    
    // Update the dish data
    UpdateDishData(CurrentDishData);
}

void UPUDishCustomizationWidget::RemoveIngredientInstance(int32 InstanceID)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstance - Removing ingredient instance: %d"), InstanceID);
    
    // Find and remove the ingredient instance from the dish data
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == InstanceID)
        {
            CurrentDishData.IngredientInstances.RemoveAt(i);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstance - Instance removed successfully"));
            break;
        }
    }
    
    // Update the dish data
    UpdateDishData(CurrentDishData);
}

void UPUDishCustomizationWidget::RefreshQuantityControls()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::RefreshQuantityControls - Refreshing quantity controls"));
    
    // This will be called when dish data is updated to refresh all quantity controls
    // Blueprint can override this to handle the UI updates
}

void UPUDishCustomizationWidget::SubscribeToEvents()
{
    UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - STARTING EVENT SUBSCRIPTION"));
    UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Widget name: %s"), *GetName());
    
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::SubscribeToEvents - Customization component valid: %s"), *CustomizationComponent->GetName());
        
        // Subscribe to the component's events
        UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Subscribing to OnInitialDishDataReceived"));
        CustomizationComponent->OnInitialDishDataReceived.AddDynamic(this, &UPUDishCustomizationWidget::OnInitialDishDataReceived);
        
        UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Subscribing to OnDishDataUpdated"));
        CustomizationComponent->OnDishDataUpdated.AddDynamic(this, &UPUDishCustomizationWidget::OnDishDataUpdated);
        
        UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Subscribing to OnCustomizationEnded"));
        CustomizationComponent->OnCustomizationEnded.AddDynamic(this, &UPUDishCustomizationWidget::OnCustomizationEnded);
        
        UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::SubscribeToEvents - All events subscribed successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::SubscribeToEvents - No customization component to subscribe to"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - EVENT SUBSCRIPTION COMPLETED"));
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

void UPUDishCustomizationWidget::EndCustomizationFromUI()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::EndCustomizationFromUI - UI button pressed to end customization"));
    
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::EndCustomizationFromUI - Calling EndCustomization on component"));
        CustomizationComponent->EndCustomization();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::EndCustomizationFromUI - No customization component available"));
    }
} 