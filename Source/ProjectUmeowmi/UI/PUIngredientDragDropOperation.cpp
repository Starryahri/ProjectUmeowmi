#include "PUIngredientDragDropOperation.h"
#include "PUCookingStageWidget.h"

UPUIngredientDragDropOperation::UPUIngredientDragDropOperation()
{
    // IngredientInstance will be set when SetupIngredientDrag is called
}

void UPUIngredientDragDropOperation::SetupIngredientDrag(const FIngredientInstance& InIngredientInstance)
{
    IngredientInstance = InIngredientInstance;
    
    // Ensure IngredientTag is set from IngredientData if not already set
    if (!IngredientInstance.IngredientTag.IsValid() && IngredientInstance.IngredientData.IngredientTag.IsValid())
    {
        IngredientInstance.IngredientTag = IngredientInstance.IngredientData.IngredientTag;
    }
    
    // Generate a unique ID if none provided (InstanceID == 0)
    // This happens when dragging from pantry slots
    bool bFromPantry = (IngredientInstance.InstanceID == 0);
    if (bFromPantry)
    {
        // Use GUID-based unique ID generation
        IngredientInstance.InstanceID = UPUCookingStageWidget::GenerateGUIDBasedInstanceID();
        // Set quantity to 1 when dragging from pantry
        IngredientInstance.Quantity = 1;
        UE_LOG(LogTemp, Display, TEXT("🔍 Generated GUID-based InstanceID: %d (from pantry, quantity set to 1, tag: %s)"), 
            IngredientInstance.InstanceID, *IngredientInstance.IngredientTag.ToString());
    }

    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientDragDropOperation::SetupIngredientDrag - Set up drag for ingredient %s (ID: %d, Qty: %d, Tag: %s, FromPantry: %s)"), 
        *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID, IngredientInstance.Quantity,
        *IngredientInstance.IngredientTag.ToString(), bFromPantry ? TEXT("YES") : TEXT("NO"));
}

void UPUIngredientDragDropOperation::SetDragVisualWidget(UWidget* VisualWidget)
{
    if (VisualWidget)
    {
        DefaultDragVisual = VisualWidget;
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUIngredientDragDropOperation::SetDragVisualWidget - Set drag visual widget: %s"), 
            *VisualWidget->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUIngredientDragDropOperation::SetDragVisualWidget - VisualWidget is null"));
    }
} 