#include "PUIngredientDragDropOperation.h"
#include "PUCookingStageWidget.h"

UPUIngredientDragDropOperation::UPUIngredientDragDropOperation()
{
    // IngredientInstance will be set when SetupIngredientDrag is called
}

void UPUIngredientDragDropOperation::SetupIngredientDrag(const FIngredientInstance& InIngredientInstance)
{
    IngredientInstance = InIngredientInstance;
    
    // Generate a unique ID if none provided (InstanceID == 0)
    if (IngredientInstance.InstanceID == 0)
    {
        // Use GUID-based unique ID generation
        IngredientInstance.InstanceID = UPUCookingStageWidget::GenerateGUIDBasedInstanceID();
        UE_LOG(LogTemp, Display, TEXT("🔍 Generated GUID-based InstanceID: %d"), IngredientInstance.InstanceID);
    }

    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientDragDropOperation::SetupIngredientDrag - Set up drag for ingredient %s (ID: %d, Qty: %d)"), 
        *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID, IngredientInstance.Quantity);
} 