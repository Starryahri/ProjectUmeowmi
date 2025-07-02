#include "PUOrderBlueprintLibrary.h"
#include "Engine/Engine.h"

bool UPUOrderBlueprintLibrary::ValidateDish(const FPUOrderBase& Order, const FPUDishBase& Dish)
{
    UE_LOG(LogTemp, Log, TEXT("UPUOrderBlueprintLibrary::ValidateDish - Called from Blueprint"));
    return Order.ValidateDish(Dish);
}

float UPUOrderBlueprintLibrary::GetSatisfactionScore(const FPUOrderBase& Order, const FPUDishBase& Dish)
{
    UE_LOG(LogTemp, Log, TEXT("UPUOrderBlueprintLibrary::GetSatisfactionScore - Called from Blueprint"));
    return Order.GetSatisfactionScore(Dish);
}

void UPUOrderBlueprintLibrary::LogOrderDetails(const FPUOrderBase& Order)
{
    UE_LOG(LogTemp, Log, TEXT("UPUOrderBlueprintLibrary::LogOrderDetails - Called from Blueprint"));
    Order.LogOrderDetails();
}

void UPUOrderBlueprintLibrary::LogValidationResults(const FPUOrderBase& Order, const FPUDishBase& Dish)
{
    UE_LOG(LogTemp, Log, TEXT("UPUOrderBlueprintLibrary::LogValidationResults - Called from Blueprint"));
    Order.LogValidationResults(Dish);
}

FPUOrderBase UPUOrderBlueprintLibrary::CreateSimpleOrder(FName OrderID, FText Description, int32 MinIngredients, FName TargetFlavor, float MinFlavorValue, FText DialogueText)
{
    UE_LOG(LogTemp, Log, TEXT("UPUOrderBlueprintLibrary::CreateSimpleOrder - Creating order: %s"), *OrderID.ToString());
    
    FPUOrderBase NewOrder;
    NewOrder.OrderID = OrderID;
    NewOrder.OrderDescription = Description;
    NewOrder.MinIngredientCount = MinIngredients;
    NewOrder.TargetFlavorProperty = TargetFlavor;
    NewOrder.MinFlavorValue = MinFlavorValue;
    NewOrder.OrderDialogueText = DialogueText;
    
    UE_LOG(LogTemp, Log, TEXT("UPUOrderBlueprintLibrary::CreateSimpleOrder - Order created successfully"));
    NewOrder.LogOrderDetails();
    
    return NewOrder;
}

 