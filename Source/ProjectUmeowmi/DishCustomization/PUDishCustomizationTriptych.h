#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUDishCustomizationTriptych.generated.h"

class UTexture2D;

/** One cover panel in a stage triptych (left rail vignette, center stage, right chrome). */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUDishCustomizationTriptychPanel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triptych")
    TSoftObjectPtr<UTexture2D> PanelImage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triptych")
    FText PanelCaption;
};

/**
 * Row in TriptychDataTable — three cover panels shown while transitioning into a pipeline stage.
 * Row name can match StageId (e.g. Stage.Chopping) or use StageDescriptor::TriptychRowName override.
 */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUDishCustomizationTriptychRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Matches FPUDishCustomizationStageDescriptor::StageId when resolving by tag instead of row name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triptych", meta = (Categories = "Stage"))
    FGameplayTag StageId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triptych")
    FPUDishCustomizationTriptychPanel LeftPanel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triptych")
    FPUDishCustomizationTriptychPanel CenterPanel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triptych")
    FPUDishCustomizationTriptychPanel RightPanel;

    /** Default cover duration when Blueprint does not drive its own animation timing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triptych|Transition", meta = (ClampMin = "0"))
    float SuggestedTransitionSeconds = 0.65f;
};
