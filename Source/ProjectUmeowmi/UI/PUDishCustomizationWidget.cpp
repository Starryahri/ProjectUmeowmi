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

void UPUDishCustomizationWidget::CreateIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Creating ingredient buttons"));
    
    if (!CurrentDishData.IngredientDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("PUDishCustomizationWidget::CreateIngredientButtons - No ingredient data table available"));
        return;
    }
    
    // Get all rows from the ingredient data table
    TArray<FName> RowNames = CurrentDishData.IngredientDataTable->GetRowNames();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Found %d ingredients in data table"), RowNames.Num());
    
    for (const FName& RowName : RowNames)
    {
        if (FPUIngredientBase* IngredientData = CurrentDishData.IngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("CreateIngredientButtons")))
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Creating button for ingredient: %s"), 
                *IngredientData->DisplayName.ToString());
            
            // Call Blueprint event to create the button with ingredient data
            OnIngredientButtonCreated(nullptr, *IngredientData); // Pass the ingredient data
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Ingredient button creation complete"));
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
        QuantityControlWidget->RemoveFromViewport();
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