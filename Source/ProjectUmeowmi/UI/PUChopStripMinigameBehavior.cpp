#include "PUChopStripMinigameBehavior.h"

#include "PUPipelineStageMinigameModuleWidget.h"
#include "PUIngredientSlot.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "InputCoreTypes.h"

namespace
{
    constexpr bool bPU_LogChopMinigameProgress = true;
}

EPUChopCompletedCutTier UPUChopStripMinigameBehavior::GetCompletedCutTier() const
{
    return static_cast<EPUChopCompletedCutTier>(FMath::Clamp(CompletedCutTierCount, 0, MaxCompletedCutTiers));
}

EPUChopCompletedCutTier UPUChopStripMinigameBehavior::GetTargetCutTier() const
{
    if (CompletedCutTierCount >= MaxCompletedCutTiers)
    {
        return EPUChopCompletedCutTier::Minced;
    }
    return static_cast<EPUChopCompletedCutTier>(CompletedCutTierCount + 1);
}

float UPUChopStripMinigameBehavior::GetChopTierProgressNormalized() const
{
    const int32 PerTier = FMath::Max(1, ChopsPerTier);
    return static_cast<float>(ChopsTowardNextTier) / static_cast<float>(PerTier);
}

int32 UPUChopStripMinigameBehavior::GetTotalChopStrokesCompleted() const
{
    return CompletedCutTierCount * FMath::Max(1, ChopsPerTier) + ChopsTowardNextTier;
}

int32 UPUChopStripMinigameBehavior::GetTotalChopStrokesRequired() const
{
    return MaxCompletedCutTiers * FMath::Max(1, ChopsPerTier);
}

float UPUChopStripMinigameBehavior::GetOverallChopProgressNormalized() const
{
    const int32 Required = GetTotalChopStrokesRequired();
    if (Required <= 0)
    {
        return 0.f;
    }
    return FMath::Clamp(
        static_cast<float>(GetTotalChopStrokesCompleted()) / static_cast<float>(Required),
        0.f,
        1.f);
}

int32 UPUChopStripMinigameBehavior::GetProgressBarTierIconCount() const
{
    return FMath::Clamp(ProgressBarTierIconCount, 1, 4);
}

float UPUChopStripMinigameBehavior::GetChopTierAnchorPercent(int32 TierIconIndex) const
{
    const int32 Count = GetProgressBarTierIconCount();
    const int32 ClampedIndex = FMath::Clamp(TierIconIndex, 0, Count - 1);
    if (Count <= 1)
    {
        return 0.f;
    }
    return static_cast<float>(ClampedIndex) / static_cast<float>(Count - 1);
}

EPUChopTierIconState UPUChopStripMinigameBehavior::GetChopTierIconState(int32 TierIconIndex) const
{
    const int32 ClampedIndex = FMath::Clamp(TierIconIndex, 0, GetProgressBarTierIconCount() - 1);
    if (ClampedIndex == 0)
    {
        return GetTotalChopStrokesCompleted() > 0 ? EPUChopTierIconState::Completed : EPUChopTierIconState::InProgress;
    }

    if (CompletedCutTierCount >= ClampedIndex)
    {
        return EPUChopTierIconState::Completed;
    }

    const int32 TargetIndex = static_cast<int32>(GetTargetCutTier());
    if (TargetIndex == ClampedIndex)
    {
        return EPUChopTierIconState::InProgress;
    }

    return EPUChopTierIconState::Upcoming;
}

void UPUChopStripMinigameBehavior::HandleStageModuleInitialized_Implementation(
    const FPUDishCustomizationStageDescriptor& StageDescriptor)
{
    (void)StageDescriptor;
    EnsureDefaultPreparationTags();
}

namespace
{
    bool IngredientHasAnyCutPreparation(
        const FIngredientInstance& Ingredient,
        const FGameplayTag& SlicedTag,
        const FGameplayTag& ChoppedTag,
        const FGameplayTag& MincedTag)
    {
        auto HasTag = [&](const FGameplayTag& Tag) -> bool
        {
            return Tag.IsValid()
                && (Ingredient.Preparations.HasTag(Tag)
                    || Ingredient.IngredientData.ActivePreparations.HasTag(Tag));
        };

        return HasTag(SlicedTag) || HasTag(ChoppedTag) || HasTag(MincedTag);
    }
}

bool UPUChopStripMinigameBehavior::CanStartStripMinigameForSlot_Implementation(const UPUIngredientSlot* StripSlot) const
{
    if (!Super::CanStartStripMinigameForSlot_Implementation(StripSlot))
    {
        return false;
    }

    if (StripSlot->IsPreppedPantryPickerSlot())
    {
        return false;
    }

    FGameplayTag SlicedTag = SlicedPreparationTag;
    FGameplayTag ChoppedTag = ChoppedPreparationTag;
    FGameplayTag MincedTag = MincedPreparationTag;
    if (!SlicedTag.IsValid())
    {
        SlicedTag = FGameplayTag::RequestGameplayTag(FName("Prep.Slice"), false);
    }
    if (!ChoppedTag.IsValid())
    {
        ChoppedTag = FGameplayTag::RequestGameplayTag(FName("Prep.Chop"), false);
    }
    if (!MincedTag.IsValid())
    {
        MincedTag = FGameplayTag::RequestGameplayTag(FName("Prep.Mince"), false);
    }

    const FIngredientInstance& Ingredient = StripSlot->GetIngredientInstance();
    return !IngredientHasAnyCutPreparation(Ingredient, SlicedTag, ChoppedTag, MincedTag);
}

void UPUChopStripMinigameBehavior::HandleStripMinigameSessionChanged_Implementation(
    bool bActive,
    UPUIngredientSlot* StripSlot)
{
    if (bActive)
    {
        ActiveChopStripSlot = StripSlot;
        ResetChopSession();
    }
    else
    {
        if (bChopStrikeKeyHeld)
        {
            EndChopStroke();
        }
        ActiveChopStripSlot = nullptr;
    }
}

bool UPUChopStripMinigameBehavior::TryConsumeMinigameKey_Implementation(FKey Key)
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return false;
    }

    if (IsChopKey(Key))
    {
        BeginChopStroke();
        return true;
    }

    if (IsFinishChopKey(Key))
    {
        if (bChopStrikeKeyHeld)
        {
            EndChopStroke();
        }
        FinishChoppingAndApply();
        return true;
    }

    return false;
}

bool UPUChopStripMinigameBehavior::TryReleaseMinigameKey_Implementation(FKey Key)
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || !IsChopKey(Key))
    {
        return false;
    }

    if (bChopStrikeKeyHeld)
    {
        EndChopStroke();
    }
    return true;
}

void UPUChopStripMinigameBehavior::BeginChopStroke()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || bChopStrikeKeyHeld)
    {
        return;
    }

    bChopStrikeKeyHeld = true;
    RegisterChopInput();
}

void UPUChopStripMinigameBehavior::EndChopStroke()
{
    if (!bChopStrikeKeyHeld)
    {
        return;
    }

    bChopStrikeKeyHeld = false;

    if (IsValid(OwnerModule))
    {
        OwnerModule->NotifyStripMinigameChopReleased();
    }
    OnChopStrokeReleased.Broadcast();
}

void UPUChopStripMinigameBehavior::RegisterChopInput()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive())
    {
        return;
    }

    OwnerModule->NotifyStripMinigameChopPressed();
    OnChopStrokePlayed.Broadcast();

    if (CompletedCutTierCount >= MaxCompletedCutTiers)
    {
        BroadcastChopProgress();
        return;
    }

    const int32 PerTier = FMath::Max(1, ChopsPerTier);
    bool bCompletedNewCutTier = false;
    ++ChopsTowardNextTier;
    if (ChopsTowardNextTier >= PerTier)
    {
        ChopsTowardNextTier = 0;
        ++CompletedCutTierCount;
        bCompletedNewCutTier = true;
    }

    BroadcastChopProgress();

    if (bCompletedNewCutTier)
    {
        NotifyChopFoodVisualTierChanged();
    }
}

bool UPUChopStripMinigameBehavior::FinishChoppingAndApply()
{
    if (!IsValid(OwnerModule) || !OwnerModule->IsStripMinigameActive() || !IsValid(ActiveChopStripSlot))
    {
        return false;
    }

    const EPUChopCompletedCutTier AppliedTier = GetCompletedCutTier();
    const bool bApplied =
        AppliedTier != EPUChopCompletedCutTier::Whole &&
        ApplyCompletedCutTierToStripSlot(ActiveChopStripSlot, AppliedTier);

    if (bPU_LogChopMinigameProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("[ChopMinigame] Finish — applied %s (success=%d)"),
            *UEnum::GetValueAsString(AppliedTier),
            bApplied ? 1 : 0);
    }

    OnChopCommitFinished.Broadcast(AppliedTier, bApplied);
    RequestEndStripMinigameSession();
    return bApplied;
}

void UPUChopStripMinigameBehavior::ResetChopSession()
{
    if (bChopStrikeKeyHeld)
    {
        EndChopStroke();
    }
    CompletedCutTierCount = 0;
    ChopsTowardNextTier = 0;
    BroadcastChopProgress();
    NotifyChopFoodVisualTierChanged();
}

EPUIngredientCutVisualTier UPUChopStripMinigameBehavior::ChopTierToIngredientVisualTier(EPUChopCompletedCutTier Tier)
{
    return static_cast<EPUIngredientCutVisualTier>(static_cast<uint8>(Tier));
}

UTexture2D* UPUChopStripMinigameBehavior::GetMinigameIngredientDisplayTexture() const
{
    if (!IsValid(ActiveChopStripSlot))
    {
        return nullptr;
    }

    const FPUIngredientBase& IngredientData = ActiveChopStripSlot->GetIngredientInstance().IngredientData;
    const EPUChopCompletedCutTier VisualTier = GetCompletedCutTier();
    return IngredientData.GetCutVisualTexture(ChopTierToIngredientVisualTier(VisualTier));
}

FLinearColor UPUChopStripMinigameBehavior::GetMinigameIngredientDisplayTint() const
{
    if (!IsValid(ActiveChopStripSlot))
    {
        return FLinearColor::White;
    }

    return ActiveChopStripSlot->GetIngredientInstance().IngredientData.GetMinigameTintColor();
}

void UPUChopStripMinigameBehavior::NotifyChopFoodVisualTierChanged()
{
    if (!IsValid(OwnerModule))
    {
        return;
    }

    OwnerModule->ApplyStripMinigameFoodVisual();
}

void UPUChopStripMinigameBehavior::DisconnectFromOwner(
    UPUPipelineStageMinigameModuleWidget* OwnerWidget,
    UObject* ProgressBarSubscriber)
{
    DisconnectChopDelegates(OwnerWidget, ProgressBarSubscriber);
    Super::DisconnectFromOwner(OwnerWidget, ProgressBarSubscriber);
}

void UPUChopStripMinigameBehavior::DisconnectChopDelegates(
    UObject* OwnerWidget,
    UObject* ProgressBarSubscriber)
{
    (void)OwnerWidget;
    (void)ProgressBarSubscriber;

    if (bChopStrikeKeyHeld)
    {
        bChopStrikeKeyHeld = false;
    }

    ActiveChopStripSlot = nullptr;
    OnChopProgressUpdated.Clear();
    OnChopFoodVisualUpdated.Clear();
    OnChopStrokePlayed.Clear();
    OnChopStrokeReleased.Clear();
    OnChopCommitFinished.Clear();
}

void UPUChopStripMinigameBehavior::BroadcastChopProgress()
{
    if (!IsValid(OwnerModule))
    {
        return;
    }

    const EPUChopCompletedCutTier CompletedTier = GetCompletedCutTier();
    const EPUChopCompletedCutTier TargetTier = GetTargetCutTier();
    const int32 PerTier = FMath::Max(1, ChopsPerTier);

    if (bPU_LogChopMinigameProgress)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[ChopMinigame] Completed=%d (%s) Progress=%d/%d toward %s"),
            CompletedCutTierCount,
            *UEnum::GetValueAsString(CompletedTier),
            ChopsTowardNextTier,
            PerTier,
            *UEnum::GetValueAsString(TargetTier));
    }

    OnChopProgressUpdated.Broadcast(CompletedTier, ChopsTowardNextTier, PerTier, TargetTier);
}

bool UPUChopStripMinigameBehavior::ApplyCompletedCutTierToStripSlot(
    UPUIngredientSlot* StripSlot,
    EPUChopCompletedCutTier Tier)
{
    if (!IsValid(StripSlot))
    {
        return false;
    }

    const FGameplayTag PrepTag = GetPreparationTagForCompletedTier(Tier);
    if (!PrepTag.IsValid())
    {
        return false;
    }

    RemoveMutuallyExclusiveCutPreparations(StripSlot);
    return StripSlot->ApplyPreparationFromStageModule(PrepTag);
}

void UPUChopStripMinigameBehavior::RemoveMutuallyExclusiveCutPreparations(UPUIngredientSlot* StripSlot)
{
    if (!IsValid(StripSlot))
    {
        return;
    }

    const FGameplayTag CutTags[] = {SlicedPreparationTag, ChoppedPreparationTag, MincedPreparationTag};
    for (const FGameplayTag& Tag : CutTags)
    {
        if (Tag.IsValid())
        {
            StripSlot->RemovePreparationFromStageModule(Tag);
        }
    }
}

FGameplayTag UPUChopStripMinigameBehavior::GetPreparationTagForCompletedTier(EPUChopCompletedCutTier Tier) const
{
    switch (Tier)
    {
    case EPUChopCompletedCutTier::Sliced:
        return SlicedPreparationTag;
    case EPUChopCompletedCutTier::Chopped:
        return ChoppedPreparationTag;
    case EPUChopCompletedCutTier::Minced:
        return MincedPreparationTag;
    default:
        return FGameplayTag();
    }
}

bool UPUChopStripMinigameBehavior::IsChopKey(FKey Key)
{
    return Key == EKeys::P || Key == EKeys::Gamepad_FaceButton_Bottom;
}

bool UPUChopStripMinigameBehavior::IsFinishChopKey(FKey Key)
{
    return Key == EKeys::B || Key == EKeys::Gamepad_FaceButton_Right;
}

void UPUChopStripMinigameBehavior::EnsureDefaultPreparationTags()
{
    if (!SlicedPreparationTag.IsValid())
    {
        SlicedPreparationTag = FGameplayTag::RequestGameplayTag(FName("Prep.Slice"), false);
    }
    if (!ChoppedPreparationTag.IsValid())
    {
        ChoppedPreparationTag = FGameplayTag::RequestGameplayTag(FName("Prep.Chop"), false);
    }
    if (!MincedPreparationTag.IsValid())
    {
        MincedPreparationTag = FGameplayTag::RequestGameplayTag(FName("Prep.Mince"), false);
    }
}
