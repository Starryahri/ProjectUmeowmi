#include "PUCookingStripMinigameBehavior.h"

#include "PUDishCustomizationWidget.h"
#include "PUPipelineStageMinigameModuleWidget.h"
#include "PUIngredientSlot.h"
#include "../DishCustomization/PUIngredientBlueprintLibrary.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

namespace
{
    constexpr bool bPU_LogCookingMinigameProgress = true;
    constexpr bool bPU_LogCookingStripAddIngredientTrace = true;
    constexpr float HoldProgressTickSeconds = 0.05f;
    constexpr float WaitProgressTickSeconds = 0.05f;
}

int32 UPUCookingStripMinigameBehavior::GetCookingStepCount() const
{
    return EffectiveCookingSteps.Num();
}

bool UPUCookingStripMinigameBehavior::TryGetCurrentStepDescriptor(FPUCookingMinigameStepDescriptor& OutStep) const
{
    if (const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex))
    {
        OutStep = *Step;
        return true;
    }
    return false;
}

bool UPUCookingStripMinigameBehavior::IsCurrentStepComplete() const
{
    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step)
    {
        return true;
    }

    switch (Step->InputMode)
    {
    case EPUCookingStepInputMode::Strokes:
        return StepStrokesCompleted >= FMath::Max(1, Step->StrokesRequired);
    case EPUCookingStepInputMode::Hold:
        return StepHoldSecondsAccumulated >= FMath::Max(0.1f, Step->HoldSecondsRequired);
    case EPUCookingStepInputMode::Confirm:
        return false;
    case EPUCookingStepInputMode::Wait:
        return StepWaitSecondsAccumulated >= FMath::Max(0.1f, Step->WaitSecondsRequired);
    case EPUCookingStepInputMode::Toggle:
        return StepToggleActive == Step->bToggleTargetStateOn;
    case EPUCookingStepInputMode::AddIngredient:
        return bAddIngredientStepSatisfied;
    default:
        return false;
    }
}

bool UPUCookingStripMinigameBehavior::AreAllCookingStepsComplete() const
{
    return GetCookingStepCount() > 0 && CurrentStepIndex >= GetCookingStepCount();
}

float UPUCookingStripMinigameBehavior::GetCurrentStepProgressNormalized() const
{
    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step)
    {
        return 1.f;
    }

    switch (Step->InputMode)
    {
    case EPUCookingStepInputMode::Strokes:
    {
        const int32 Required = FMath::Max(1, Step->StrokesRequired);
        return FMath::Clamp(static_cast<float>(StepStrokesCompleted) / static_cast<float>(Required), 0.f, 1.f);
    }
    case EPUCookingStepInputMode::Hold:
    {
        const float Required = FMath::Max(0.1f, Step->HoldSecondsRequired);
        return FMath::Clamp(StepHoldSecondsAccumulated / Required, 0.f, 1.f);
    }
    case EPUCookingStepInputMode::Wait:
    {
        const float Required = FMath::Max(0.1f, Step->WaitSecondsRequired);
        return FMath::Clamp(StepWaitSecondsAccumulated / Required, 0.f, 1.f);
    }
    case EPUCookingStepInputMode::Toggle:
        return StepToggleActive == Step->bToggleTargetStateOn ? 1.f : 0.f;
    case EPUCookingStepInputMode::Confirm:
        return 0.f;
    case EPUCookingStepInputMode::AddIngredient:
        return bAddIngredientStepSatisfied ? 1.f : 0.f;
    default:
        return 0.f;
    }
}

float UPUCookingStripMinigameBehavior::GetOverallCookProgressNormalized() const
{
    const int32 StepCount = GetCookingStepCount();
    if (StepCount <= 0)
    {
        return 0.f;
    }

    if (AreAllCookingStepsComplete())
    {
        return 1.f;
    }

    const float StepProgress = GetCurrentStepProgressNormalized();
    return FMath::Clamp((static_cast<float>(CurrentStepIndex) + StepProgress) / static_cast<float>(StepCount), 0.f, 1.f);
}

int32 UPUCookingStripMinigameBehavior::GetProgressBarStepIconCount() const
{
    return FMath::Clamp(FMath::Min(ProgressBarStepIconCount, GetCookingStepCount()), 1, 4);
}

float UPUCookingStripMinigameBehavior::GetCookingStepAnchorPercent(int32 StepIconIndex) const
{
    const int32 Count = GetProgressBarStepIconCount();
    const int32 ClampedIndex = FMath::Clamp(StepIconIndex, 0, Count - 1);
    if (Count <= 1)
    {
        return 0.f;
    }
    return static_cast<float>(ClampedIndex) / static_cast<float>(Count - 1);
}

EPUCookingStepIconState UPUCookingStripMinigameBehavior::GetCookingStepIconState(int32 StepIconIndex) const
{
    const int32 ClampedIndex = FMath::Clamp(StepIconIndex, 0, GetProgressBarStepIconCount() - 1);
    if (CurrentStepIndex > ClampedIndex || AreAllCookingStepsComplete())
    {
        return EPUCookingStepIconState::Completed;
    }
    if (CurrentStepIndex == ClampedIndex)
    {
        return EPUCookingStepIconState::InProgress;
    }
    return EPUCookingStepIconState::Upcoming;
}

bool UPUCookingStripMinigameBehavior::HasAllSelectedRailIngredientsInCookingPot() const
{
    if (!IsValid(OwnerModule) || !IsValid(OwnerModule->OwnerShell))
    {
        return false;
    }

    const int32 FilledRailSlots = OwnerModule->OwnerShell->CountFilledIngredientRailStripSlots();
    if (FilledRailSlots <= 0)
    {
        return false;
    }

    return CookingPotIngredients.Num() >= FilledRailSlots;
}

void UPUCookingStripMinigameBehavior::HandleStageModuleInitialized_Implementation(
    const FPUDishCustomizationStageDescriptor& StageDescriptor)
{
    (void)StageDescriptor;
    EnsureDefaultPreparationTag();
    EnsureDefaultCookingSteps();
}

void UPUCookingStripMinigameBehavior::HandleStripMinigameSessionChanged_Implementation(
    bool bActive,
    UPUIngredientSlot* StripSlot)
{
    if (bActive)
    {
        ActiveCookingStripSlot = StripSlot;
        if (IsValid(OwnerModule) && IsValid(OwnerModule->OwnerShell))
        {
            OwnerModule->OwnerShell->SetStripMinigameMainPantrySuppressed(true);
        }
        ResetCookSession();
    }
    else
    {
        EndActiveStepShellEffects();
        StopHoldProgressTimer();
        StopWaitProgressTimer();
        if (bCookStrikeKeyHeld && IsValid(OwnerModule))
        {
            bCookStrikeKeyHeld = false;
            OwnerModule->NotifyStripMinigameMixReleased();
        }

        if (IsValid(OwnerModule) && IsValid(OwnerModule->OwnerShell))
        {
            OwnerModule->OwnerShell->SetStripMinigameMainPantrySuppressed(false);
            OwnerModule->OwnerShell->ClearCookingAddIngredientSessionState();
        }

        ActiveCookingStripSlot = nullptr;
    }
}

bool UPUCookingStripMinigameBehavior::TryConsumeMinigameKey_Implementation(FKey Key)
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || AreAllCookingStepsComplete())
    {
        return false;
    }

    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step)
    {
        return false;
    }

    if (IsConfirmStepKey(Key) && Step->InputMode == EPUCookingStepInputMode::Confirm)
    {
        ConfirmCurrentStep();
        return true;
    }

    if (!IsCookStrikeKey(Key))
    {
        return false;
    }

    if (Step->InputMode == EPUCookingStepInputMode::Strokes)
    {
        if (bCookStrikeKeyHeld)
        {
            return true;
        }

        bCookStrikeKeyHeld = true;
        RegisterCookStrokeInput();
        return true;
    }

    if (Step->InputMode == EPUCookingStepInputMode::Hold)
    {
        if (bCookStrikeKeyHeld)
        {
            return true;
        }

        bCookStrikeKeyHeld = true;
        StartHoldProgressTimer();
        OwnerModule->NotifyStripMinigameMixPressed();
        return true;
    }

    if (Step->InputMode == EPUCookingStepInputMode::Toggle)
    {
        RegisterCookToggleInput();
        return true;
    }

    return false;
}

bool UPUCookingStripMinigameBehavior::TryReleaseMinigameKey_Implementation(FKey Key)
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || !IsCookStrikeKey(Key))
    {
        return false;
    }

    if (bCookStrikeKeyHeld)
    {
        bCookStrikeKeyHeld = false;
        StopHoldProgressTimer();
        OwnerModule->NotifyStripMinigameMixReleased();
    }

    return true;
}

bool UPUCookingStripMinigameBehavior::CanStartStripMinigameForSlot_Implementation(const UPUIngredientSlot* StripSlot) const
{
    if (!Super::CanStartStripMinigameForSlot_Implementation(StripSlot))
    {
        return false;
    }

    if (!IsValid(OwnerModule) || !IsValid(OwnerModule->OwnerShell))
    {
        return false;
    }

    UPUDishCustomizationWidget* Shell = OwnerModule->OwnerShell;
    if (!Shell->AreAllRequiredIngredientRailSlotsFilled())
    {
        return false;
    }

    if (GetCookingStepCount() <= 0)
    {
        return false;
    }

    if (CookingRailBindings.Num() <= 0)
    {
        return true;
    }

    for (const FPUCookingPotRailBinding& Binding : CookingRailBindings)
    {
        if (const UPUIngredientSlot* RailSlot = Binding.StripSlot.Get())
        {
            if (!IngredientHasCookedPreparation(RailSlot->GetIngredientInstance()))
            {
                return true;
            }
        }
    }

    return false;
}

void UPUCookingStripMinigameBehavior::RegisterCookStrokeInput()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || AreAllCookingStepsComplete())
    {
        return;
    }

    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step || Step->InputMode != EPUCookingStepInputMode::Strokes || IsCurrentStepComplete())
    {
        BroadcastCookProgress();
        return;
    }

    OwnerModule->NotifyStripMinigameMixPressed();
    ++StepStrokesCompleted;
    BroadcastCookProgress();

    if (IsCurrentStepComplete())
    {
        CompleteCurrentStep();
    }
}

void UPUCookingStripMinigameBehavior::ConfirmCurrentStep()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || AreAllCookingStepsComplete())
    {
        return;
    }

    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step || Step->InputMode != EPUCookingStepInputMode::Confirm)
    {
        return;
    }

    CompleteCurrentStep();
}

void UPUCookingStripMinigameBehavior::AdvanceCookingStep()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return;
    }

    if (AreAllCookingStepsComplete())
    {
        FinishCookingAndApply();
        return;
    }

    ++CurrentStepIndex;
    if (AreAllCookingStepsComplete())
    {
        FinishCookingAndApply();
        return;
    }

    BeginCurrentStep();
}

void UPUCookingStripMinigameBehavior::CompleteCurrentStep()
{
    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step)
    {
        return;
    }

    if (bPU_LogCookingMinigameProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("[CookingMinigame] Step %d complete (%s)"),
            CurrentStepIndex,
            Step->StepId.IsValid() ? *Step->StepId.ToString() : TEXT("(no StepId)"));
    }

    if (Step->bAutoAdvanceOnComplete)
    {
        AdvanceCookingStep();
    }
    else
    {
        EndActiveStepShellEffects();
        BroadcastCookProgress();
    }
}

bool UPUCookingStripMinigameBehavior::FinishCookingAndApply()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return false;
    }

    StopHoldProgressTimer();
    StopWaitProgressTimer();

    const bool bApplied = ApplyCookedPreparationToRailSlots();

    if (bPU_LogCookingMinigameProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("[CookingMinigame] Finish — applied cooked prep (success=%d)"), bApplied ? 1 : 0);
    }

    OnCookingCommitFinished.Broadcast(bApplied);
    RequestEndStripMinigameSession();

    if (UPUDishCustomizationWidget* Shell = IsValid(OwnerModule) ? OwnerModule->OwnerShell : nullptr)
    {
        if (UWorld* World = GetBehaviorWorld())
        {
            TWeakObjectPtr<UPUDishCustomizationWidget> WeakShell(Shell);
            World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(Shell, [WeakShell]()
            {
                if (UPUDishCustomizationWidget* ShellPtr = WeakShell.Get())
                {
                    ShellPtr->AdvanceCustomizationAfterCookingMinigameComplete();
                }
            }));
        }
    }

    return bApplied;
}

void UPUCookingStripMinigameBehavior::ResetCookSession()
{
    EndActiveStepShellEffects();
    StopHoldProgressTimer();
    StopWaitProgressTimer();
    UnregisterAllCookingRailStripSlots();
    CookingPotIngredients.Empty();
    bCookStrikeKeyHeld = false;
    CurrentStepIndex = 0;
    StepStrokesCompleted = 0;
    StepHoldSecondsAccumulated = 0.f;
    StepWaitSecondsAccumulated = 0.f;
    StepToggleActive = false;
    bAddIngredientStepSatisfied = false;
    EnsureDefaultCookingSteps();
    BeginCurrentStep();
}

void UPUCookingStripMinigameBehavior::EndActiveStepShellEffects()
{
    StopWaitProgressTimer();
    if (IsValid(OwnerModule) && IsValid(OwnerModule->OwnerShell))
    {
        OwnerModule->OwnerShell->EndCookingAddIngredientRailStep();
    }
}

void UPUCookingStripMinigameBehavior::BeginCurrentStepShellEffects(
    const FPUCookingMinigameStepDescriptor& StepDescriptor)
{
    if (!IsValid(OwnerModule) || !IsValid(OwnerModule->OwnerShell))
    {
        return;
    }

    if (StepDescriptor.InputMode == EPUCookingStepInputMode::AddIngredient)
    {
        OwnerModule->OwnerShell->BeginCookingAddIngredientRailStep(
            StepDescriptor.AddIngredientRailSlotIndex,
            StepDescriptor.AddIngredientRequiredType);
        return;
    }

    if (StepDescriptor.InputMode == EPUCookingStepInputMode::Wait)
    {
        StartWaitProgressTimer();
    }
}

void UPUCookingStripMinigameBehavior::BeginCurrentStep()
{
    EndActiveStepShellEffects();

    StepStrokesCompleted = 0;
    StepHoldSecondsAccumulated = 0.f;
    StepWaitSecondsAccumulated = 0.f;
    StepToggleActive = false;
    bAddIngredientStepSatisfied = false;

    FPUCookingMinigameStepDescriptor StepDescriptor;
    if (TryGetCurrentStepDescriptor(StepDescriptor))
    {
        if (bPU_LogCookingMinigameProgress)
        {
            UE_LOG(LogTemp, Log, TEXT("[CookingMinigame] Begin step %d/%d (%s) mode=%d"),
                CurrentStepIndex,
                GetCookingStepCount(),
                StepDescriptor.StepId.IsValid() ? *StepDescriptor.StepId.ToString() : TEXT("(no StepId)"),
                static_cast<int32>(StepDescriptor.InputMode));
        }

        BeginCurrentStepShellEffects(StepDescriptor);
        OnCookingStepChanged.Broadcast(CurrentStepIndex, StepDescriptor);
    }

    BroadcastCookProgress();
}

void UPUCookingStripMinigameBehavior::BroadcastCookProgress()
{
    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    int32 StepRequired = 1;
    int32 StepCompleted = 0;

    if (Step)
    {
        switch (Step->InputMode)
        {
        case EPUCookingStepInputMode::Strokes:
            StepRequired = FMath::Max(1, Step->StrokesRequired);
            StepCompleted = StepStrokesCompleted;
            break;
        case EPUCookingStepInputMode::Hold:
            StepRequired = FMath::Max(1, FMath::CeilToInt(Step->HoldSecondsRequired));
            StepCompleted = FMath::Clamp(FMath::FloorToInt(StepHoldSecondsAccumulated), 0, StepRequired);
            break;
        case EPUCookingStepInputMode::Confirm:
            StepRequired = 1;
            StepCompleted = 0;
            break;
        case EPUCookingStepInputMode::Wait:
            StepRequired = FMath::Max(1, FMath::CeilToInt(Step->WaitSecondsRequired));
            StepCompleted = FMath::Clamp(FMath::FloorToInt(StepWaitSecondsAccumulated), 0, StepRequired);
            break;
        case EPUCookingStepInputMode::Toggle:
            StepRequired = 1;
            StepCompleted = StepToggleActive == Step->bToggleTargetStateOn ? 1 : 0;
            break;
        case EPUCookingStepInputMode::AddIngredient:
            StepRequired = 1;
            StepCompleted = bAddIngredientStepSatisfied ? 1 : 0;
            break;
        default:
            break;
        }
    }

    const float Overall = GetOverallCookProgressNormalized();
    const int32 TotalSteps = GetCookingStepCount();

    if (bPU_LogCookingMinigameProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("[CookingMinigame] Progress step %d/%d — %d/%d (overall %.0f%%)"),
            CurrentStepIndex,
            FMath::Max(0, TotalSteps - 1),
            StepCompleted,
            StepRequired,
            Overall * 100.f);
    }

    OnCookingProgressUpdated.Broadcast(CurrentStepIndex, StepCompleted, StepRequired, Overall, TotalSteps);
}

void UPUCookingStripMinigameBehavior::TickHoldProgress()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || !bCookStrikeKeyHeld || AreAllCookingStepsComplete())
    {
        StopHoldProgressTimer();
        return;
    }

    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step || Step->InputMode != EPUCookingStepInputMode::Hold)
    {
        StopHoldProgressTimer();
        return;
    }

    StepHoldSecondsAccumulated += HoldProgressTickSeconds;
    BroadcastCookProgress();

    if (IsCurrentStepComplete())
    {
        StopHoldProgressTimer();
        if (bCookStrikeKeyHeld && IsValid(OwnerModule))
        {
            bCookStrikeKeyHeld = false;
            OwnerModule->NotifyStripMinigameMixReleased();
        }
        CompleteCurrentStep();
    }
}

void UPUCookingStripMinigameBehavior::StartHoldProgressTimer()
{
    if (UWorld* World = GetBehaviorWorld())
    {
        World->GetTimerManager().SetTimer(
            HoldProgressTimerHandle,
            this,
            &UPUCookingStripMinigameBehavior::TickHoldProgress,
            HoldProgressTickSeconds,
            true);
    }
}

void UPUCookingStripMinigameBehavior::StopHoldProgressTimer()
{
    if (UWorld* World = GetBehaviorWorld())
    {
        World->GetTimerManager().ClearTimer(HoldProgressTimerHandle);
    }
}

void UPUCookingStripMinigameBehavior::StartWaitProgressTimer()
{
    if (UWorld* World = GetBehaviorWorld())
    {
        World->GetTimerManager().SetTimer(
            WaitProgressTimerHandle,
            this,
            &UPUCookingStripMinigameBehavior::TickWaitProgress,
            WaitProgressTickSeconds,
            true);
    }
}

void UPUCookingStripMinigameBehavior::StopWaitProgressTimer()
{
    if (UWorld* World = GetBehaviorWorld())
    {
        World->GetTimerManager().ClearTimer(WaitProgressTimerHandle);
    }
}

void UPUCookingStripMinigameBehavior::TickWaitProgress()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || AreAllCookingStepsComplete())
    {
        StopWaitProgressTimer();
        return;
    }

    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step || Step->InputMode != EPUCookingStepInputMode::Wait)
    {
        StopWaitProgressTimer();
        return;
    }

    StepWaitSecondsAccumulated += WaitProgressTickSeconds;
    BroadcastCookProgress();

    if (IsCurrentStepComplete())
    {
        StopWaitProgressTimer();
        CompleteCurrentStep();
    }
}

void UPUCookingStripMinigameBehavior::RegisterCookToggleInput()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || AreAllCookingStepsComplete())
    {
        return;
    }

    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step || Step->InputMode != EPUCookingStepInputMode::Toggle)
    {
        return;
    }

    StepToggleActive = !StepToggleActive;
    OwnerModule->NotifyStripMinigameMixPressed();
    OnCookingToggleStateChanged.Broadcast(StepToggleActive);
    BroadcastCookProgress();

    if (IsCurrentStepComplete())
    {
        CompleteCurrentStep();
    }
}

bool UPUCookingStripMinigameBehavior::DoesStripSlotSatisfyAddIngredientStep(const UPUIngredientSlot* StripSlot) const
{
    if (!IsValid(StripSlot) || StripSlot->IsEmpty())
    {
        return false;
    }

    if (IsValid(OwnerModule) && IsValid(OwnerModule->OwnerShell)
        && OwnerModule->OwnerShell->IsRailSlotConsumedForCookingAddIngredient(StripSlot))
    {
        return false;
    }

    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step || Step->InputMode != EPUCookingStepInputMode::AddIngredient)
    {
        return false;
    }

    if (Step->AddIngredientRailSlotIndex != INDEX_NONE && IsValid(OwnerModule) && IsValid(OwnerModule->OwnerShell))
    {
        if (OwnerModule->OwnerShell->GetIngredientRailStripSlotByIndex(Step->AddIngredientRailSlotIndex) != StripSlot)
        {
            return false;
        }
    }

    if (Step->AddIngredientRequiredType.IsValid())
    {
        FGameplayTagContainer TypeFilter;
        TypeFilter.AddTag(Step->AddIngredientRequiredType);
        if (!UPUIngredientBlueprintLibrary::IngredientMatchesTypeFilter(
                StripSlot->GetIngredientInstance().IngredientData,
                TypeFilter))
        {
            return false;
        }
    }

    return true;
}

void UPUCookingStripMinigameBehavior::NotifyAddIngredientRailSlotCommitted(UPUIngredientSlot* StripSlot)
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || AreAllCookingStepsComplete())
    {
        if (bPU_LogCookingStripAddIngredientTrace)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CookingAddIngredient] Behavior commit ignored — minigame inactive or all steps done"));
        }
        return;
    }

    const FPUCookingMinigameStepDescriptor* Step = GetStepDescriptor(CurrentStepIndex);
    if (!Step || Step->InputMode != EPUCookingStepInputMode::AddIngredient || bAddIngredientStepSatisfied)
    {
        if (bPU_LogCookingStripAddIngredientTrace)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CookingAddIngredient] Behavior commit ignored — step %d is not an active Add Ingredient step"),
                CurrentStepIndex);
        }
        return;
    }

    if (!DoesStripSlotSatisfyAddIngredientStep(StripSlot))
    {
        if (bPU_LogCookingStripAddIngredientTrace)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CookingAddIngredient] Behavior commit rejected — slot %s failed step validation"),
                IsValid(StripSlot) ? *StripSlot->GetName() : TEXT("(invalid)"));
        }
        return;
    }

    const FIngredientInstance IngredientInstance = StripSlot->GetIngredientInstance();
    int32 PotIngredientIndex = FindPotIngredientIndexForStripSlot(StripSlot);
    const bool bNewPotEntry = PotIngredientIndex == INDEX_NONE;
    if (bNewPotEntry)
    {
        CookingPotIngredients.Add(IngredientInstance);
        PotIngredientIndex = CookingPotIngredients.Num() - 1;
        RegisterCookingRailStripSlot(StripSlot, PotIngredientIndex);
    }
    else if (CookingPotIngredients.IsValidIndex(PotIngredientIndex))
    {
        CookingPotIngredients[PotIngredientIndex] = IngredientInstance;
    }

    if (bPU_LogCookingStripAddIngredientTrace)
    {
        UE_LOG(LogTemp, Log, TEXT("[CookingAddIngredient] Added to pot — slot=%s ingredient=%s potIndex=%d potCount=%d (new=%s)"),
            *StripSlot->GetName(),
            IngredientInstance.IngredientData.IngredientTag.IsValid()
                ? *IngredientInstance.IngredientData.IngredientTag.ToString()
                : TEXT("(no tag)"),
            PotIngredientIndex,
            CookingPotIngredients.Num(),
            bNewPotEntry ? TEXT("yes") : TEXT("no"));
    }

    OnCookingPotIngredientAdded.Broadcast(StripSlot, IngredientInstance);

    bAddIngredientStepSatisfied = true;
    BroadcastCookProgress();
    CompleteCurrentStep();
}

UWorld* UPUCookingStripMinigameBehavior::GetBehaviorWorld() const
{
    return IsValid(OwnerModule) ? OwnerModule->GetWorld() : nullptr;
}

void UPUCookingStripMinigameBehavior::EnsureDefaultCookingSteps()
{
    EffectiveCookingSteps = CookingSteps;
    if (EffectiveCookingSteps.Num() > 0)
    {
        return;
    }

    FPUCookingMinigameStepDescriptor LegacyStep;
    LegacyStep.StepDisplayName = NSLOCTEXT("CookingMinigame", "LegacySimmerStep", "Simmer");
    LegacyStep.InputMode = EPUCookingStepInputMode::Strokes;
    LegacyStep.StrokesRequired = FMath::Max(1, DefaultLegacyStrokesRequired);
    LegacyStep.bAutoAdvanceOnComplete = true;
    EffectiveCookingSteps.Add(LegacyStep);
}

const FPUCookingMinigameStepDescriptor* UPUCookingStripMinigameBehavior::GetStepDescriptor(int32 StepIndex) const
{
    return EffectiveCookingSteps.IsValidIndex(StepIndex) ? &EffectiveCookingSteps[StepIndex] : nullptr;
}

bool UPUCookingStripMinigameBehavior::ApplyCookedPreparationToRailSlots()
{
    EnsureDefaultPreparationTag();
    if (!CookedPreparationTag.IsValid())
    {
        return false;
    }

    bool bAppliedAny = false;
    for (const FPUCookingPotRailBinding& Binding : CookingRailBindings)
    {
        UPUIngredientSlot* RailSlot = Binding.StripSlot.Get();
        if (!IsValid(RailSlot) || RailSlot->IsEmpty())
        {
            continue;
        }

        if (RailSlot->ApplyPreparationFromStageModule(CookedPreparationTag))
        {
            bAppliedAny = true;
        }
    }

    return bAppliedAny;
}

bool UPUCookingStripMinigameBehavior::IngredientHasCookedPreparation(const FIngredientInstance& IngredientInstance) const
{
    FGameplayTag CookedTag = CookedPreparationTag;
    if (!CookedTag.IsValid())
    {
        CookedTag = FGameplayTag::RequestGameplayTag(FName("Prep.Cook"), false);
    }

    if (!CookedTag.IsValid())
    {
        return false;
    }

    return IngredientInstance.Preparations.HasTag(CookedTag)
        || IngredientInstance.IngredientData.ActivePreparations.HasTag(CookedTag);
}

bool UPUCookingStripMinigameBehavior::IsCookStrikeKey(FKey Key)
{
    return Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Bottom;
}

bool UPUCookingStripMinigameBehavior::IsConfirmStepKey(FKey Key)
{
    return Key == EKeys::B || Key == EKeys::Gamepad_FaceButton_Right;
}

void UPUCookingStripMinigameBehavior::DisconnectCookingDelegates(UObject* ProgressBarSubscriber)
{
    if (ProgressBarSubscriber)
    {
        OnCookingProgressUpdated.RemoveAll(ProgressBarSubscriber);
        OnCookingStepChanged.RemoveAll(ProgressBarSubscriber);
    }
}

void UPUCookingStripMinigameBehavior::HandleIngredientAddedToStripSlot_Implementation(
    UPUIngredientSlot* StripSlot,
    const FIngredientInstance& IngredientInstance)
{
    // Gather-phase rail fills stay on the rail until an Add Ingredient step commits them to the pot.
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return;
    }

    (void)StripSlot;
    (void)IngredientInstance;
}

void UPUCookingStripMinigameBehavior::DisconnectFromOwner(
    UPUPipelineStageMinigameModuleWidget* OwnerWidget,
    UObject* ProgressBarSubscriber)
{
    EndActiveStepShellEffects();
    StopHoldProgressTimer();
    StopWaitProgressTimer();
    DisconnectCookingDelegates(ProgressBarSubscriber);
    UnregisterAllCookingRailStripSlots();
    CookingPotIngredients.Empty();
    EffectiveCookingSteps.Empty();
    OnCookingPotIngredientAdded.Clear();
    OnCookingProgressUpdated.Clear();
    OnCookingStepChanged.Clear();
    OnCookingCommitFinished.Clear();
    OnCookingToggleStateChanged.Clear();
    ActiveCookingStripSlot = nullptr;
    CurrentStepIndex = 0;
    StepStrokesCompleted = 0;
    StepHoldSecondsAccumulated = 0.f;
    StepWaitSecondsAccumulated = 0.f;
    StepToggleActive = false;
    bAddIngredientStepSatisfied = false;

    Super::DisconnectFromOwner(OwnerWidget, ProgressBarSubscriber);
}

void UPUCookingStripMinigameBehavior::HandleCookingRailStripSlotIngredientChanged(
    const FIngredientInstance& IngredientInstance)
{
    (void)IngredientInstance;
    SyncCookingPotFromRailBindings();
}

void UPUCookingStripMinigameBehavior::EnsureDefaultPreparationTag()
{
    if (!CookedPreparationTag.IsValid())
    {
        CookedPreparationTag = FGameplayTag::RequestGameplayTag(FName("Prep.Cook"), false);
    }
}

void UPUCookingStripMinigameBehavior::RegisterCookingRailStripSlot(
    UPUIngredientSlot* StripSlot,
    int32 PotIngredientIndex)
{
    if (!IsValid(StripSlot) || PotIngredientIndex == INDEX_NONE)
    {
        return;
    }

    for (FPUCookingPotRailBinding& Binding : CookingRailBindings)
    {
        if (Binding.StripSlot.Get() == StripSlot)
        {
            Binding.PotIngredientIndex = PotIngredientIndex;
            StripSlot->OnSlotIngredientChanged.AddUniqueDynamic(
                this,
                &UPUCookingStripMinigameBehavior::HandleCookingRailStripSlotIngredientChanged);
            return;
        }
    }

    FPUCookingPotRailBinding NewBinding;
    NewBinding.StripSlot = StripSlot;
    NewBinding.PotIngredientIndex = PotIngredientIndex;
    CookingRailBindings.Add(NewBinding);

    StripSlot->OnSlotIngredientChanged.AddUniqueDynamic(
        this,
        &UPUCookingStripMinigameBehavior::HandleCookingRailStripSlotIngredientChanged);
}

void UPUCookingStripMinigameBehavior::UnregisterAllCookingRailStripSlots()
{
    for (const FPUCookingPotRailBinding& Binding : CookingRailBindings)
    {
        if (UPUIngredientSlot* StripSlot = Binding.StripSlot.Get())
        {
            StripSlot->OnSlotIngredientChanged.RemoveDynamic(
                this,
                &UPUCookingStripMinigameBehavior::HandleCookingRailStripSlotIngredientChanged);
        }
    }
    CookingRailBindings.Empty();
}

void UPUCookingStripMinigameBehavior::SyncCookingPotFromRailBindings()
{
    for (const FPUCookingPotRailBinding& Binding : CookingRailBindings)
    {
        UPUIngredientSlot* StripSlot = Binding.StripSlot.Get();
        const int32 PotIngredientIndex = Binding.PotIngredientIndex;
        if (PotIngredientIndex == INDEX_NONE)
        {
            continue;
        }

        if (!IsValid(StripSlot) || StripSlot->IsEmpty())
        {
            if (CookingPotIngredients.IsValidIndex(PotIngredientIndex))
            {
                CookingPotIngredients[PotIngredientIndex] = FIngredientInstance();
            }
            continue;
        }

        const FIngredientInstance& Instance = StripSlot->GetIngredientInstance();
        if (CookingPotIngredients.IsValidIndex(PotIngredientIndex))
        {
            CookingPotIngredients[PotIngredientIndex] = Instance;
        }
    }
}

int32 UPUCookingStripMinigameBehavior::FindPotIngredientIndexForStripSlot(const UPUIngredientSlot* StripSlot) const
{
    if (!IsValid(StripSlot))
    {
        return INDEX_NONE;
    }

    for (const FPUCookingPotRailBinding& Binding : CookingRailBindings)
    {
        if (Binding.StripSlot.Get() == StripSlot)
        {
            return Binding.PotIngredientIndex;
        }
    }

    return INDEX_NONE;
}
