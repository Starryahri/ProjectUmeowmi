#include "PUMarinateStripMinigameBehavior.h"

#include "PUPipelineStageMinigameModuleWidget.h"
#include "PUIngredientSlot.h"
#include "InputCoreTypes.h"

namespace
{
    constexpr bool bPU_LogMarinateMinigameProgress = true;
}

float UPUMarinateStripMinigameBehavior::GetOverallMarinateProgressNormalized() const
{
    const int32 Required = FMath::Max(1, MixStrokesRequired);
    return FMath::Clamp(
        static_cast<float>(MixStrokesCompleted) / static_cast<float>(Required),
        0.f,
        1.f);
}

bool UPUMarinateStripMinigameBehavior::IsMarinationComplete() const
{
    return MixStrokesCompleted >= FMath::Max(1, MixStrokesRequired);
}

void UPUMarinateStripMinigameBehavior::HandleStageModuleInitialized_Implementation(
    const FPUDishCustomizationStageDescriptor& StageDescriptor)
{
    (void)StageDescriptor;
    EnsureDefaultPreparationTag();
}

void UPUMarinateStripMinigameBehavior::HandleStripMinigameSessionChanged_Implementation(
    bool bActive,
    UPUIngredientSlot* StripSlot)
{
    if (bActive)
    {
        ActiveMarinateStripSlot = StripSlot;
        ResetMarinateSession();
    }
    else
    {
        ActiveMarinateStripSlot = nullptr;
    }
}

bool UPUMarinateStripMinigameBehavior::TryConsumeMinigameKey_Implementation(FKey Key)
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return false;
    }

    if (IsMixKey(Key))
    {
        RegisterMixInput();
        return true;
    }

    return false;
}

bool UPUMarinateStripMinigameBehavior::CanStartStripMinigameForSlot_Implementation(const UPUIngredientSlot* StripSlot) const
{
    if (!Super::CanStartStripMinigameForSlot_Implementation(StripSlot))
    {
        return false;
    }

    if (MarinationBowlIngredients.Num() <= 0)
    {
        return false;
    }

    if (MarinationRailBindings.Num() <= 0)
    {
        return true;
    }

    for (const FPUMarinationBowlRailBinding& Binding : MarinationRailBindings)
    {
        if (const UPUIngredientSlot* RailSlot = Binding.StripSlot.Get())
        {
            if (!IngredientHasMarinatedPreparation(RailSlot->GetIngredientInstance()))
            {
                return true;
            }
        }
    }

    return false;
}

void UPUMarinateStripMinigameBehavior::RegisterMixInput()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return;
    }

    if (IsMarinationComplete())
    {
        BroadcastMarinateProgress();
        return;
    }

    OwnerModule->NotifyStripMinigameChopPressed();
    ++MixStrokesCompleted;
    BroadcastMarinateProgress();

    if (IsMarinationComplete())
    {
        FinishMarinationAndApply();
    }
}

bool UPUMarinateStripMinigameBehavior::FinishMarinationAndApply()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return false;
    }

    const bool bApplied = ApplyMarinatedPreparationToBowlRailSlots();

    if (bPU_LogMarinateMinigameProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("[MarinateMinigame] Finish — applied Prep.Marinate (success=%d)"), bApplied ? 1 : 0);
    }

    OnMarinateCommitFinished.Broadcast(bApplied);
    RequestEndStripMinigameSession();
    return bApplied;
}

void UPUMarinateStripMinigameBehavior::ResetMarinateSession()
{
    MixStrokesCompleted = 0;
    BroadcastMarinateProgress();
}

void UPUMarinateStripMinigameBehavior::BroadcastMarinateProgress()
{
    const int32 Required = FMath::Max(1, MixStrokesRequired);

    if (bPU_LogMarinateMinigameProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("[MarinateMinigame] Mix progress %d/%d"), MixStrokesCompleted, Required);
    }

    OnMarinateProgressUpdated.Broadcast(MixStrokesCompleted, Required);
}

bool UPUMarinateStripMinigameBehavior::ApplyMarinatedPreparationToBowlRailSlots()
{
    EnsureDefaultPreparationTag();
    if (!MarinatedPreparationTag.IsValid())
    {
        return false;
    }

    bool bAppliedAny = false;
    for (const FPUMarinationBowlRailBinding& Binding : MarinationRailBindings)
    {
        UPUIngredientSlot* RailSlot = Binding.StripSlot.Get();
        if (!IsValid(RailSlot) || RailSlot->IsEmpty())
        {
            continue;
        }

        if (RailSlot->ApplyPreparationFromStageModule(MarinatedPreparationTag))
        {
            bAppliedAny = true;
        }
    }

    return bAppliedAny;
}

bool UPUMarinateStripMinigameBehavior::IngredientHasMarinatedPreparation(const FIngredientInstance& IngredientInstance) const
{
    FGameplayTag MarinateTag = MarinatedPreparationTag;
    if (!MarinateTag.IsValid())
    {
        MarinateTag = FGameplayTag::RequestGameplayTag(FName("Prep.Marinate"), false);
    }

    if (!MarinateTag.IsValid())
    {
        return false;
    }

    return IngredientInstance.Preparations.HasTag(MarinateTag)
        || IngredientInstance.IngredientData.ActivePreparations.HasTag(MarinateTag);
}

bool UPUMarinateStripMinigameBehavior::IsMixKey(FKey Key)
{
    return Key == EKeys::P || Key == EKeys::Gamepad_FaceButton_Bottom;
}

void UPUMarinateStripMinigameBehavior::DisconnectMarinateDelegates(UObject* ProgressBarSubscriber)
{
    if (ProgressBarSubscriber)
    {
        OnMarinateProgressUpdated.RemoveAll(ProgressBarSubscriber);
    }
}

void UPUMarinateStripMinigameBehavior::HandleIngredientAddedToStripSlot_Implementation(
    UPUIngredientSlot* StripSlot,
    const FIngredientInstance& IngredientInstance)
{
    int32 BowlIngredientIndex = FindBowlIngredientIndexForStripSlot(StripSlot);
    if (BowlIngredientIndex == INDEX_NONE)
    {
        MarinationBowlIngredients.Add(IngredientInstance);
        BowlIngredientIndex = MarinationBowlIngredients.Num() - 1;
        RegisterMarinationRailStripSlot(StripSlot, BowlIngredientIndex);
    }
    else if (MarinationBowlIngredients.IsValidIndex(BowlIngredientIndex))
    {
        MarinationBowlIngredients[BowlIngredientIndex] = IngredientInstance;
    }

    if (IsValid(OwnerModule))
    {
        OwnerModule->ApplyMarinationBowlVisualsForIngredient(IngredientInstance, StripSlot, BowlIngredientIndex);
    }

    OnMarinationBowlIngredientAdded.Broadcast(StripSlot, IngredientInstance);
}

void UPUMarinateStripMinigameBehavior::DisconnectFromOwner(
    UPUPipelineStageMinigameModuleWidget* OwnerWidget,
    UObject* ProgressBarSubscriber)
{
    DisconnectMarinateDelegates(ProgressBarSubscriber);
    UnregisterAllMarinationRailStripSlots();
    MarinationBowlIngredients.Empty();
    OnMarinationBowlIngredientAdded.Clear();
    OnMarinateProgressUpdated.Clear();
    OnMarinateCommitFinished.Clear();
    ActiveMarinateStripSlot = nullptr;
    MixStrokesCompleted = 0;

    if (IsValid(OwnerWidget))
    {
        OwnerWidget->ClearMarinationBowlVisuals();
    }

    Super::DisconnectFromOwner(OwnerWidget, ProgressBarSubscriber);
}

void UPUMarinateStripMinigameBehavior::HandleMarinationRailStripSlotIngredientChanged(
    const FIngredientInstance& IngredientInstance)
{
    (void)IngredientInstance;
    SyncMarinationBowlVisualsFromRailBindings();
}

void UPUMarinateStripMinigameBehavior::EnsureDefaultPreparationTag()
{
    if (!MarinatedPreparationTag.IsValid())
    {
        MarinatedPreparationTag = FGameplayTag::RequestGameplayTag(FName("Prep.Marinate"), false);
    }
}

void UPUMarinateStripMinigameBehavior::RegisterMarinationRailStripSlot(
    UPUIngredientSlot* StripSlot,
    int32 BowlIngredientIndex)
{
    if (!IsValid(StripSlot) || BowlIngredientIndex == INDEX_NONE)
    {
        return;
    }

    for (FPUMarinationBowlRailBinding& Binding : MarinationRailBindings)
    {
        if (Binding.StripSlot.Get() == StripSlot)
        {
            Binding.BowlIngredientIndex = BowlIngredientIndex;
            StripSlot->OnSlotIngredientChanged.AddUniqueDynamic(
                this,
                &UPUMarinateStripMinigameBehavior::HandleMarinationRailStripSlotIngredientChanged);
            return;
        }
    }

    FPUMarinationBowlRailBinding NewBinding;
    NewBinding.StripSlot = StripSlot;
    NewBinding.BowlIngredientIndex = BowlIngredientIndex;
    MarinationRailBindings.Add(NewBinding);

    StripSlot->OnSlotIngredientChanged.AddUniqueDynamic(
        this,
        &UPUMarinateStripMinigameBehavior::HandleMarinationRailStripSlotIngredientChanged);
}

void UPUMarinateStripMinigameBehavior::UnregisterAllMarinationRailStripSlots()
{
    for (const FPUMarinationBowlRailBinding& Binding : MarinationRailBindings)
    {
        if (UPUIngredientSlot* StripSlot = Binding.StripSlot.Get())
        {
            StripSlot->OnSlotIngredientChanged.RemoveDynamic(
                this,
                &UPUMarinateStripMinigameBehavior::HandleMarinationRailStripSlotIngredientChanged);
        }
    }
    MarinationRailBindings.Empty();
}

void UPUMarinateStripMinigameBehavior::SyncMarinationBowlVisualsFromRailBindings()
{
    if (!IsValid(OwnerModule))
    {
        return;
    }

    for (const FPUMarinationBowlRailBinding& Binding : MarinationRailBindings)
    {
        UPUIngredientSlot* StripSlot = Binding.StripSlot.Get();
        const int32 BowlIngredientIndex = Binding.BowlIngredientIndex;
        if (BowlIngredientIndex == INDEX_NONE)
        {
            continue;
        }

        if (!IsValid(StripSlot) || StripSlot->IsEmpty())
        {
            if (MarinationBowlIngredients.IsValidIndex(BowlIngredientIndex))
            {
                MarinationBowlIngredients[BowlIngredientIndex] = FIngredientInstance();
            }
            OwnerModule->ClearMarinationBowlLayerVisuals(BowlIngredientIndex);
            continue;
        }

        const FIngredientInstance& Instance = StripSlot->GetIngredientInstance();
        if (MarinationBowlIngredients.IsValidIndex(BowlIngredientIndex))
        {
            MarinationBowlIngredients[BowlIngredientIndex] = Instance;
        }
        OwnerModule->ApplyMarinationBowlVisualsForIngredient(Instance, StripSlot, BowlIngredientIndex);
    }
}

int32 UPUMarinateStripMinigameBehavior::FindBowlIngredientIndexForStripSlot(const UPUIngredientSlot* StripSlot) const
{
    if (!IsValid(StripSlot))
    {
        return INDEX_NONE;
    }

    for (const FPUMarinationBowlRailBinding& Binding : MarinationRailBindings)
    {
        if (Binding.StripSlot.Get() == StripSlot)
        {
            return Binding.BowlIngredientIndex;
        }
    }

    return INDEX_NONE;
}
