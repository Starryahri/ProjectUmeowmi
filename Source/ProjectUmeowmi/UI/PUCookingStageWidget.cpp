#include "PUCookingStageWidget.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"

UPUCookingStageWidget::UPUCookingStageWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPUCookingStageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::NativeConstruct - Cooking stage widget constructed"));
}

void UPUCookingStageWidget::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::NativeDestruct - Cooking stage widget destructing"));
    
    Super::NativeDestruct();
}

void UPUCookingStageWidget::InitializeCookingStage(const FPUDishBase& DishData)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Initializing cooking stage"));
    
    // Initialize the current dish data
    CurrentDishData = DishData;
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Cooking stage initialized for dish: %s"), 
        *CurrentDishData.DisplayName.ToString());
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Selected ingredients: %d"), 
        CurrentDishData.IngredientInstances.Num());
    
    // Create quantity controls for the selected ingredients
    CreateQuantityControlsForSelectedIngredients();
    
    // Call Blueprint event
    OnCookingStageInitialized(CurrentDishData);
}

void UPUCookingStageWidget::CreateQuantityControlsForSelectedIngredients()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Creating quantity controls"));
    
    if (!QuantityControlClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - No quantity control class set"));
        return;
    }
    
    // Create quantity controls for each ingredient instance
    for (const FIngredientInstance& IngredientInstance : CurrentDishData.IngredientInstances)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Creating control for: %s"), 
            *IngredientInstance.IngredientData.DisplayName.ToString());
        
        // Create quantity control widget
        if (UPUIngredientQuantityControl* QuantityControl = CreateWidget<UPUIngredientQuantityControl>(this, QuantityControlClass))
        {
            QuantityControl->SetIngredientInstance(IngredientInstance);
            QuantityControl->SetPreparationCheckboxClass(PreparationCheckboxClass);
            
            // Bind events
            QuantityControl->OnQuantityControlChanged.AddDynamic(this, &UPUCookingStageWidget::OnQuantityControlChanged);
            QuantityControl->OnQuantityControlRemoved.AddDynamic(this, &UPUCookingStageWidget::OnQuantityControlRemoved);
            
            // Add to viewport (Blueprint will handle the actual placement)
            QuantityControl->AddToViewport();
            
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Quantity control created for instance: %d"), 
                IngredientInstance.InstanceID);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Created %d quantity controls"), 
        CurrentDishData.IngredientInstances.Num());
}

void UPUCookingStageWidget::OnQuantityControlChanged(const FIngredientInstance& IngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlChanged - Quantity control changed for instance: %d"), 
        IngredientInstance.InstanceID);
    
    // Update the ingredient instance in the current dish data
    for (FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == IngredientInstance.InstanceID)
        {
            Instance = IngredientInstance;
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlChanged - Updated instance in dish data"));
            break;
        }
    }
}

void UPUCookingStageWidget::OnQuantityControlRemoved(int32 InstanceID, UPUIngredientQuantityControl* QuantityControlWidget)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlRemoved - Quantity control removed for instance: %d"), InstanceID);
    
    // Remove the ingredient instance from the current dish data
    CurrentDishData.IngredientInstances.RemoveAll([InstanceID](const FIngredientInstance& Instance) {
        return Instance.InstanceID == InstanceID;
    });
    
    // Remove the widget from viewport
    if (QuantityControlWidget)
    {
        QuantityControlWidget->RemoveFromParent();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlRemoved - Widget removed from viewport"));
    }
}

void UPUCookingStageWidget::FinishCookingAndStartPlating()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FinishCookingAndStartPlating - Finishing cooking and starting plating"));
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FinishCookingAndStartPlating - Final dish has %d ingredients"), 
        CurrentDishData.IngredientInstances.Num());
    
    // Call Blueprint event
    OnCookingStageCompleted(CurrentDishData);
    
    // Broadcast the cooking completed event
    OnCookingCompleted.Broadcast(CurrentDishData);
    
    // Remove this widget from viewport
    RemoveFromParent();
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FinishCookingAndStartPlating - Cooking stage completed"));
} 