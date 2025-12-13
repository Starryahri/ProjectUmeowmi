#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "GameplayTagContainer.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientDragDropOperation.generated.h"

UCLASS()
class PROJECTUMEOWMI_API UPUIngredientDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    UPUIngredientDragDropOperation();

    // The ingredient instance being dragged (contains all ingredient data)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Drag Drop", meta = (ExposeOnSpawn = "true"))
    FIngredientInstance IngredientInstance;

    // Set up the drag operation with ingredient instance
    UFUNCTION(BlueprintCallable, Category = "Ingredient Drag Drop")
    void SetupIngredientDrag(const FIngredientInstance& InIngredientInstance);

    // Set the drag visual widget (should be called from Blueprint OnDragDetected)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Drag Drop")
    void SetDragVisualWidget(UWidget* VisualWidget);
}; 