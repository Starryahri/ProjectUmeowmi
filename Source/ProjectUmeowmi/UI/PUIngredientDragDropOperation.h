#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "GameplayTagContainer.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "PUIngredientDragDropOperation.generated.h"

UCLASS()
class PROJECTUMEOWMI_API UPUIngredientDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    UPUIngredientDragDropOperation();

    // The ingredient tag being dragged
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Drag Drop", meta = (ExposeOnSpawn = "true"))
    FGameplayTag IngredientTag;

    // The ingredient data being dragged
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Drag Drop", meta = (ExposeOnSpawn = "true"))
    FPUIngredientBase IngredientData;

    // The instance ID of the ingredient being dragged
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Drag Drop", meta = (ExposeOnSpawn = "true"))
    int32 InstanceID;

    // The quantity of the ingredient being dragged
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Drag Drop", meta = (ExposeOnSpawn = "true"))
    int32 Quantity;

    // Set up the drag operation with ingredient data
    UFUNCTION(BlueprintCallable, Category = "Ingredient Drag Drop")
    void SetupIngredientDrag(const FGameplayTag& InIngredientTag, const FPUIngredientBase& InIngredientData, int32 InInstanceID, int32 InQuantity);
}; 