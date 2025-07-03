#include "PUDishCustomizationWidget.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"

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