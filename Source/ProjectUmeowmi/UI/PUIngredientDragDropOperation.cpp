#include "PUIngredientDragDropOperation.h"
#include "PUCookingStageWidget.h"

UPUIngredientDragDropOperation::UPUIngredientDragDropOperation()
{
    InstanceID = 0; // Will be set when SetupIngredientDrag is called
    Quantity = 1;
}

void UPUIngredientDragDropOperation::SetupIngredientDrag(const FGameplayTag& InIngredientTag, const FPUIngredientBase& InIngredientData, int32 InInstanceID, int32 InQuantity)
{
    IngredientTag = InIngredientTag;
    IngredientData = InIngredientData;
    
    // Generate a unique ID if none provided (InInstanceID == 0)
    if (InInstanceID == 0)
    {
        // Use GUID-based unique ID generation
        InstanceID = UPUCookingStageWidget::GenerateGUIDBasedInstanceID();
        UE_LOG(LogTemp, Display, TEXT("🔍 Generated GUID-based InstanceID: %d"), InstanceID);
    }
    else
    {
        InstanceID = InInstanceID;
    }
    
    Quantity = InQuantity;

    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientDragDropOperation::SetupIngredientDrag - Set up drag for ingredient %s (ID: %d, Qty: %d)"), 
        *IngredientTag.ToString(), InstanceID, Quantity);
} 