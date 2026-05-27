#pragma once

#include "CoreMinimal.h"
#include "PUStripMinigameBehavior.h"
#include "GameplayTagContainer.h"
#include "PUMarinateStripMinigameBehavior.generated.h"

class UPUIngredientSlot;

/** Fired when a rail ingredient is mirrored into the marination bowl visuals. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FPUOnMarinationBowlIngredientAdded,
    UPUIngredientSlot*,
    StripSlot,
    FIngredientInstance,
    IngredientInstance);

/**
 * Marinate strip minigame: rail fills mirror into bowl (5 visual copies). Y toggles session; A mashes progress (follow-up).
 * Blueprints stay parented on Pipeline Stage Minigame Module — assign this behavior class or map Stage.Marinate.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Marinate Strip Minigame Behavior"))
class PROJECTUMEOWMI_API UPUMarinateStripMinigameBehavior : public UPUStripMinigameBehavior
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Marinate|Preparations", meta = (Categories = "Prep"))
    FGameplayTag MarinatedPreparationTag;

    UPROPERTY(BlueprintAssignable, Category = "Marinate")
    FPUOnMarinationBowlIngredientAdded OnMarinationBowlIngredientAdded;

    /** Logical ingredients in the marination bowl (one entry per rail strip fill). */
    UPROPERTY(BlueprintReadOnly, Category = "Marinate|Bowl")
    TArray<FIngredientInstance> MarinationBowlIngredients;

    virtual void HandleStageModuleInitialized_Implementation(
        const FPUDishCustomizationStageDescriptor& StageDescriptor) override;

    virtual void HandleIngredientAddedToStripSlot_Implementation(
        UPUIngredientSlot* StripSlot,
        const FIngredientInstance& IngredientInstance) override;

    virtual void DisconnectFromOwner(
        UPUPipelineStageMinigameModuleWidget* OwnerWidget,
        UObject* ProgressBarSubscriber) override;

protected:
    void EnsureDefaultPreparationTag();
};
