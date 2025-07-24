#include "PUIngredientDragDropOperation.h"

UPUIngredientDragDropOperation::UPUIngredientDragDropOperation()
{
    InstanceID = 0;
    Quantity = 1;
}

void UPUIngredientDragDropOperation::SetupIngredientDrag(const FGameplayTag& InIngredientTag, const FPUIngredientBase& InIngredientData, int32 InInstanceID, int32 InQuantity)
{
    IngredientTag = InIngredientTag;
    IngredientData = InIngredientData;
    InstanceID = InInstanceID;
    Quantity = InQuantity;

    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientDragDropOperation::SetupIngredientDrag - Set up drag for ingredient %s (ID: %d, Qty: %d)"), 
        *IngredientTag.ToString(), InstanceID, Quantity);
} 