#include "PUMarinateStripMinigameBehavior.h"

#include "PUPipelineStageMinigameModuleWidget.h"
#include "PUIngredientSlot.h"

void UPUMarinateStripMinigameBehavior::HandleStageModuleInitialized_Implementation(
    const FPUDishCustomizationStageDescriptor& StageDescriptor)
{
    (void)StageDescriptor;
    EnsureDefaultPreparationTag();
}

void UPUMarinateStripMinigameBehavior::HandleIngredientAddedToStripSlot_Implementation(
    UPUIngredientSlot* StripSlot,
    const FIngredientInstance& IngredientInstance)
{
    MarinationBowlIngredients.Add(IngredientInstance);

    if (IsValid(OwnerModule))
    {
        OwnerModule->ApplyMarinationBowlVisualsForIngredient(IngredientInstance, StripSlot);
    }

    OnMarinationBowlIngredientAdded.Broadcast(StripSlot, IngredientInstance);
}

void UPUMarinateStripMinigameBehavior::DisconnectFromOwner(
    UPUPipelineStageMinigameModuleWidget* OwnerWidget,
    UObject* ProgressBarSubscriber)
{
    MarinationBowlIngredients.Empty();
    OnMarinationBowlIngredientAdded.Clear();

    if (IsValid(OwnerWidget))
    {
        OwnerWidget->ClearMarinationBowlVisuals();
    }

    Super::DisconnectFromOwner(OwnerWidget, ProgressBarSubscriber);
}

void UPUMarinateStripMinigameBehavior::EnsureDefaultPreparationTag()
{
    if (!MarinatedPreparationTag.IsValid())
    {
        MarinatedPreparationTag = FGameplayTag::RequestGameplayTag(FName("Prep.Marinate"), false);
    }
}
