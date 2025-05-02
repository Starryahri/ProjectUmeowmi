#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PUIngredientData.generated.h"

USTRUCT(BlueprintType)
struct FIngredientData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    FName IngredientName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    TSoftObjectPtr<UTexture2D> PreviewTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    TSoftObjectPtr<UMaterialInterface> MaterialInstance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    int32 MinQuantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    int32 MaxQuantity = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    int32 CurrentQuantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    float PricePerUnit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    TArray<FVector> PlacementPositions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    TArray<FRotator> PlacementRotations;

    // Special effects at specific quantities
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    TMap<int32, FName> QuantitySpecialEffects;
}; 