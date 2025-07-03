#include "PUOrderComponent.h"
#include "PUOrderBlueprintLibrary.h"
#include "PUDishBlueprintLibrary.h"
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
    
    // Get a random dish tag
    FGameplayTag DishTag = UPUDishBlueprintLibrary::GetRandomDishTag();
    if (!DishTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUOrderComponent::GenerateSimpleOrder - Failed to get valid dish tag, using default"));
        DishTag = FGameplayTag::RequestGameplayTag(TEXT("Dish.Congee"));
    }
    
    // Get the base dish from the data table
    FPUDishBase BaseDish;
    bool bGotBaseDish = false;
    if (DishDataTable)
    {
        bGotBaseDish = UPUDishBlueprintLibrary::GetDishFromDataTable(DishDataTable, DishTag, BaseDish);
    }
    
    if (!bGotBaseDish)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUOrderComponent::GenerateSimpleOrder - Failed to get base dish, creating empty dish"));
        BaseDish.DishTag = DishTag;
        BaseDish.DisplayName = FText::FromString(DishTag.ToString());
    }
    
    // Create a unique order ID
    FName OrderID = FName(*FString::Printf(TEXT("Order_%d"), FMath::RandRange(1000, 9999)));
    
    // Create the dialogue text with the specific dish name
    FText DialogueText = FText::Format(
        DefaultOrderDescription,
        FText::AsNumber(DefaultMinIngredients),
        FText::FromString(DefaultTargetFlavor.ToString())
    );
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::GenerateSimpleOrder - Creating order with ID: %s for dish: %s"), 
        *OrderID.ToString(), *DishTag.ToString());
    
    // Create the order using the Blueprint Library
    CurrentOrder = UPUOrderBlueprintLibrary::CreateSimpleOrder(
        OrderID,
        FText::FromString(FString::Printf(TEXT("Simple %s order"), *DishTag.ToString())),
        DefaultMinIngredients,
        DefaultTargetFlavor,
        DefaultMinFlavorValue,
        DialogueText
    );
    
    // Set the base dish in the order
    CurrentOrder.BaseDish = BaseDish;
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderComponent::GenerateSimpleOrder - Order created successfully with base dish: %s (%d ingredients)"), 
        *BaseDish.DisplayName.ToString(), BaseDish.IngredientInstances.Num());
} 