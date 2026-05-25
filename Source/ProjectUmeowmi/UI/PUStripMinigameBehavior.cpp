#include "PUStripMinigameBehavior.h"

#include "../DishCustomization/PUDishBase.h"
#include "PUPipelineStageMinigameModuleWidget.h"
#include "PUIngredientSlot.h"
#include "PUUObjectSafety.h"
#include "UObject/UObjectIterator.h"

void UPUStripMinigameBehavior::InitializeBehavior(UPUPipelineStageMinigameModuleWidget* InOwnerModule)
{
    OwnerModule = InOwnerModule;
}

void UPUStripMinigameBehavior::HandleStageModuleInitialized_Implementation(
    const FPUDishCustomizationStageDescriptor& StageDescriptor)
{
    (void)StageDescriptor;
}

void UPUStripMinigameBehavior::HandleStripMinigameSessionChanged_Implementation(
    bool bActive,
    UPUIngredientSlot* StripSlot)
{
    (void)bActive;
    (void)StripSlot;
}

bool UPUStripMinigameBehavior::TryConsumeMinigameKey_Implementation(FKey Key)
{
    (void)Key;
    return false;
}

bool UPUStripMinigameBehavior::TryReleaseMinigameKey_Implementation(FKey Key)
{
    (void)Key;
    return false;
}

bool UPUStripMinigameBehavior::CanStartStripMinigameForSlot_Implementation(const UPUIngredientSlot* StripSlot) const
{
    return IsValid(StripSlot) && !StripSlot->IsEmpty();
}

void UPUStripMinigameBehavior::RequestEndStripMinigameSession()
{
    if (IsValid(OwnerModule) && OwnerModule->IsStripMinigameActive())
    {
        OwnerModule->SetStripMinigameActive(false, nullptr);
    }
}

void UPUStripMinigameBehavior::DisconnectFromOwner(
    UPUPipelineStageMinigameModuleWidget* OwnerWidget,
    UObject* ProgressBarSubscriber)
{
    (void)OwnerWidget;
    (void)ProgressBarSubscriber;
    OwnerModule = nullptr;
}

void UPUStripMinigameBehavior::SanitizeStaleObjectReferences()
{
    if (OwnerModule != nullptr && !PUObjectReferenceSafety::IsLiveObject(OwnerModule))
    {
        OwnerModule = nullptr;
    }
}

void UPUStripMinigameBehavior::SanitizeAllLiveStripMinigameBehaviors()
{
    for (TObjectIterator<UPUStripMinigameBehavior> It; It; ++It)
    {
        UPUStripMinigameBehavior* Behavior = *It;
        if (PUObjectReferenceSafety::CanQueryUObject(Behavior) && !Behavior->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
        {
            Behavior->SanitizeStaleObjectReferences();
        }
    }
}
