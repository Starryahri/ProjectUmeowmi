#include "PUCookingStripMinigameBehavior.h"

#include "PUDishCustomizationWidget.h"
#include "PUPipelineStageMinigameModuleWidget.h"
#include "PUIngredientSlot.h"
#include "InputCoreTypes.h"

namespace
{
    constexpr bool bPU_LogCookingMinigameProgress = true;
}

float UPUCookingStripMinigameBehavior::GetOverallCookProgressNormalized() const
{
    const int32 Required = FMath::Max(1, CookStrokesRequired);
    return FMath::Clamp(
        static_cast<float>(CookStrokesCompleted) / static_cast<float>(Required),
        0.f,
        1.f);
}

bool UPUCookingStripMinigameBehavior::IsCookingComplete() const
{
    return CookStrokesCompleted >= FMath::Max(1, CookStrokesRequired);
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
}

void UPUCookingStripMinigameBehavior::HandleStripMinigameSessionChanged_Implementation(
    bool bActive,
    UPUIngredientSlot* StripSlot)
{
    if (bActive)
    {
        ActiveCookingStripSlot = StripSlot;
        ResetCookSession();
    }
    else
    {
        if (bCookStrikeKeyHeld && IsValid(OwnerModule))
        {
            bCookStrikeKeyHeld = false;
            OwnerModule->NotifyStripMinigameMixReleased();
        }

        ActiveCookingStripSlot = nullptr;
    }
}

bool UPUCookingStripMinigameBehavior::TryConsumeMinigameKey_Implementation(FKey Key)
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return false;
    }

    if (IsCookStrikeKey(Key))
    {
        if (bCookStrikeKeyHeld)
        {
            return true;
        }

        bCookStrikeKeyHeld = true;
        RegisterCookInput();
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

    if (!HasAllSelectedRailIngredientsInCookingPot())
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

void UPUCookingStripMinigameBehavior::RegisterCookInput()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return;
    }

    if (IsCookingComplete())
    {
        BroadcastCookProgress();
        return;
    }

    OwnerModule->NotifyStripMinigameMixPressed();
    ++CookStrokesCompleted;
    BroadcastCookProgress();

    if (IsCookingComplete())
    {
        FinishCookingAndApply();
    }
}

bool UPUCookingStripMinigameBehavior::FinishCookingAndApply()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return false;
    }

    const bool bApplied = ApplyCookedPreparationToRailSlots();

    if (bPU_LogCookingMinigameProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("[CookingMinigame] Finish — applied cooked prep (success=%d)"), bApplied ? 1 : 0);
    }

    OnCookingCommitFinished.Broadcast(bApplied);
    RequestEndStripMinigameSession();
    return bApplied;
}

void UPUCookingStripMinigameBehavior::ResetCookSession()
{
    bCookStrikeKeyHeld = false;
    CookStrokesCompleted = 0;
    BroadcastCookProgress();
}

void UPUCookingStripMinigameBehavior::BroadcastCookProgress()
{
    const int32 Required = FMath::Max(1, CookStrokesRequired);

    if (bPU_LogCookingMinigameProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("[CookingMinigame] Cook progress %d/%d"), CookStrokesCompleted, Required);
    }

    OnCookingProgressUpdated.Broadcast(CookStrokesCompleted, Required);
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
    return Key == EKeys::P || Key == EKeys::Gamepad_FaceButton_Bottom;
}

void UPUCookingStripMinigameBehavior::DisconnectCookingDelegates(UObject* ProgressBarSubscriber)
{
    if (ProgressBarSubscriber)
    {
        OnCookingProgressUpdated.RemoveAll(ProgressBarSubscriber);
    }
}

void UPUCookingStripMinigameBehavior::HandleIngredientAddedToStripSlot_Implementation(
    UPUIngredientSlot* StripSlot,
    const FIngredientInstance& IngredientInstance)
{
    int32 PotIngredientIndex = FindPotIngredientIndexForStripSlot(StripSlot);
    if (PotIngredientIndex == INDEX_NONE)
    {
        CookingPotIngredients.Add(IngredientInstance);
        PotIngredientIndex = CookingPotIngredients.Num() - 1;
        RegisterCookingRailStripSlot(StripSlot, PotIngredientIndex);
    }
    else if (CookingPotIngredients.IsValidIndex(PotIngredientIndex))
    {
        CookingPotIngredients[PotIngredientIndex] = IngredientInstance;
    }

    OnCookingPotIngredientAdded.Broadcast(StripSlot, IngredientInstance);
}

void UPUCookingStripMinigameBehavior::DisconnectFromOwner(
    UPUPipelineStageMinigameModuleWidget* OwnerWidget,
    UObject* ProgressBarSubscriber)
{
    DisconnectCookingDelegates(ProgressBarSubscriber);
    UnregisterAllCookingRailStripSlots();
    CookingPotIngredients.Empty();
    OnCookingPotIngredientAdded.Clear();
    OnCookingProgressUpdated.Clear();
    OnCookingCommitFinished.Clear();
    ActiveCookingStripSlot = nullptr;
    CookStrokesCompleted = 0;

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
