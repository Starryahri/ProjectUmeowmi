#include "PUOrderComponent.h"
#include "PUOrderBlueprintLibrary.h"
#include "Engine/Engine.h"

UPUOrderComponent::UPUOrderComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // Set default values
    DefaultMinIngredients = 3;
    DefaultTargetFlavor = FName(TEXT("Saltiness"));
    DefaultMinFlavorValue = 5.0f;
    DefaultOrderDescription = FText::FromString(TEXT("Make me congee with {0} ingredients. Make it {1}."));
    
    bHasActiveOrder = false;
}

void UPUOrderComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::BeginPlay - Order component initialized"));
}

void UPUOrderComponent::GenerateNewOrder()
{
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::GenerateNewOrder - Starting order generation"));
    
    // Clear any existing order
    if (bHasActiveOrder)
    {
        UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::GenerateNewOrder - Clearing existing order"));
        ClearCurrentOrder();
    }
    
    // Generate a simple order
    GenerateSimpleOrder();
    
    // Set active flag
    bHasActiveOrder = true;
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::GenerateNewOrder - Order generated successfully"));
    CurrentOrder.LogOrderDetails();
    
    // Broadcast the event
    OnOrderGenerated.Broadcast(CurrentOrder);
}

void UPUOrderComponent::ClearCurrentOrder()
{
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::ClearCurrentOrder - Clearing current order"));
    
    // Reset order data
    CurrentOrder = FPUOrderBase();
    bHasActiveOrder = false;
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::ClearCurrentOrder - Order cleared"));
}

bool UPUOrderComponent::ValidateDish(const FPUDishBase& Dish) const
{
    if (!bHasActiveOrder)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUOrderComponent::ValidateDish - No active order to validate against"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::ValidateDish - Validating dish against current order"));
    return UPUOrderBlueprintLibrary::ValidateDish(CurrentOrder, Dish);
}

float UPUOrderComponent::GetSatisfactionScore(const FPUDishBase& Dish) const
{
    if (!bHasActiveOrder)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUOrderComponent::GetSatisfactionScore - No active order to score against"));
        return 0.0f;
    }
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::GetSatisfactionScore - Calculating satisfaction score"));
    return UPUOrderBlueprintLibrary::GetSatisfactionScore(CurrentOrder, Dish);
}

void UPUOrderComponent::GenerateSimpleOrder()
{
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::GenerateSimpleOrder - Generating simple order"));
    
    // Create a unique order ID
    FName OrderID = FName(*FString::Printf(TEXT("Order_%d"), FMath::RandRange(1000, 9999)));
    
    // Create the dialogue text
    FText DialogueText = FText::Format(
        DefaultOrderDescription,
        FText::AsNumber(DefaultMinIngredients),
        FText::FromString(DefaultTargetFlavor.ToString())
    );
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::GenerateSimpleOrder - Creating order with ID: %s"), *OrderID.ToString());
    
    // Create the order using the Blueprint Library
    CurrentOrder = UPUOrderBlueprintLibrary::CreateSimpleOrder(
        OrderID,
        FText::FromString(TEXT("Simple congee order")),
        DefaultMinIngredients,
        DefaultTargetFlavor,
        DefaultMinFlavorValue,
        DialogueText
    );
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::GenerateSimpleOrder - Order created successfully"));
} 