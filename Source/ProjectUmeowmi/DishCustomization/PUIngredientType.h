#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUIngredientType.generated.h"

class UTexture2D;

/** Metadata row for `Ingredient.Type.*` tags (icon + label for slot hints and UI). */
USTRUCT(BlueprintType)
struct FPUIngredientTypeBase : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type", meta = (Categories = "Ingredient.Type"))
    FGameplayTag TypeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type|Visual")
    TObjectPtr<UTexture2D> TypeIcon = nullptr;
};
