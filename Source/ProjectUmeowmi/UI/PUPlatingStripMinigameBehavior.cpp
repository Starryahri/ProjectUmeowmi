#include "PUPlatingStripMinigameBehavior.h"

#include "PUPipelineStageMinigameModuleWidget.h"
#include "PUDishCustomizationWidget.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "PUIngredientSlot.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

namespace
{
    constexpr bool bPU_LogPlatingMinigameProgress = true;
}

void UPUPlatingStripMinigameBehavior::HandleStripMinigameSessionChanged_Implementation(
    bool bActive,
    UPUIngredientSlot* StripSlot)
{
    (void)StripSlot;

    if (!IsValid(OwnerModule) || !IsValid(OwnerModule->OwnerShell))
    {
        return;
    }

    UPUDishCustomizationWidget* Shell = OwnerModule->OwnerShell;
    if (bActive)
    {
        Shell->SetStripMinigameMainPantrySuppressed(true);
        Shell->BeginPlatingMinigameRailDragStep();
        OwnerModule->ClearPlatingDishAreaVisuals();
    }
    else
    {
        Shell->SetStripMinigameMainPantrySuppressed(false);
        Shell->EndPlatingMinigameRailDragStep();
        OwnerModule->ClearPlatingDishAreaVisuals();
    }
}

bool UPUPlatingStripMinigameBehavior::TryConsumeMinigameKey_Implementation(FKey Key)
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return false;
    }

    if (IsFinishPlatingKey(Key))
    {
        FinishPlatingAndCompleteCustomization();
        return true;
    }

    return false;
}

bool UPUPlatingStripMinigameBehavior::CanStartStripMinigameForSlot_Implementation(const UPUIngredientSlot* StripSlot) const
{
    if (!Super::CanStartStripMinigameForSlot_Implementation(StripSlot))
    {
        return false;
    }

    if (!IsValid(OwnerModule) || !IsValid(OwnerModule->OwnerShell))
    {
        return false;
    }

    return OwnerModule->OwnerShell->AreAllRequiredIngredientRailSlotsFilled();
}

bool UPUPlatingStripMinigameBehavior::FinishPlatingAndCompleteCustomization()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return false;
    }

    OwnerModule->SyncPlatingDishAreaToDishData();

    if (bPU_LogPlatingMinigameProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("[PlatingMinigame] Finish — committing dish layout and ending customization"));
    }

    OnPlatingCommitFinished.Broadcast();
    RequestEndStripMinigameSession();

    if (UPUDishCustomizationWidget* Shell = OwnerModule->OwnerShell)
    {
        if (UWorld* World = Shell->GetWorld())
        {
            TWeakObjectPtr<UPUDishCustomizationWidget> WeakShell(Shell);
            World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(Shell, [WeakShell]()
            {
                if (UPUDishCustomizationWidget* ShellPtr = WeakShell.Get())
                {
                    ShellPtr->CompleteCustomizationAfterPlatingMinigame();
                }
            }));
        }
    }

    return true;
}

void UPUPlatingStripMinigameBehavior::DisconnectFromOwner(
    UPUPipelineStageMinigameModuleWidget* OwnerWidget,
    UObject* ProgressBarSubscriber)
{
    (void)ProgressBarSubscriber;
    OnPlatingCommitFinished.Clear();

    if (IsValid(OwnerWidget))
    {
        OwnerWidget->ClearPlatingDishAreaVisuals();
    }

    Super::DisconnectFromOwner(OwnerWidget, ProgressBarSubscriber);
}

bool UPUPlatingStripMinigameBehavior::IsFinishPlatingKey(FKey Key)
{
    return Key == EKeys::B || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Escape;
}
