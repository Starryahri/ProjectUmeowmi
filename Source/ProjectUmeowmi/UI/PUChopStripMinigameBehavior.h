#pragma once

#include "CoreMinimal.h"
#include "PUStripMinigameBehavior.h"
#include "GameplayTagContainer.h"
#include "PUChopStripMinigameBehavior.generated.h"

class UPUIngredientSlot;

/** Last fully completed cut tier when finishing (whole = no prep applied). */
UENUM(BlueprintType)
enum class EPUChopCompletedCutTier : uint8
{
    Whole   UMETA(DisplayName = "Whole"),
    Sliced  UMETA(DisplayName = "Sliced"),
    Chopped UMETA(DisplayName = "Chopped"),
    Minced  UMETA(DisplayName = "Minced")
};

/** Visual state for one of the four tier icons on the chop progress bar (whole → minced). */
UENUM(BlueprintType)
enum class EPUChopTierIconState : uint8
{
    Upcoming    UMETA(DisplayName = "Upcoming"),
    InProgress  UMETA(DisplayName = "In Progress"),
    Completed   UMETA(DisplayName = "Completed")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FPUOnChopProgressUpdated,
    EPUChopCompletedCutTier,
    CompletedTier,
    int32,
    ChopsTowardNextTier,
    int32,
    ChopsPerTier,
    EPUChopCompletedCutTier,
    TargetTier);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FPUOnChopCommitFinished,
    EPUChopCompletedCutTier,
    AppliedTier,
    bool,
    bAppliedPreparation);

/** Fired on every chop key press (before progress tier logic). Use for extra VFX/SFX in Blueprint. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPUOnChopStrokePlayed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPUOnChopStrokeReleased);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FPUOnChopFoodVisualUpdated,
    UTexture2D*,
    FoodTexture,
    FLinearColor,
    FoodTint);

/**
 * Chop strip minigame: 3 chops per tier (whole → sliced → chopped → minced).
 * Chop: gamepad A / P. Finish: gamepad B / B — applies last completed tier only.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Chop Strip Minigame Behavior"))
class PROJECTUMEOWMI_API UPUChopStripMinigameBehavior : public UPUStripMinigameBehavior
{
    GENERATED_BODY()

public:
    static constexpr int32 DefaultChopsPerTier = 3;
    static constexpr int32 MaxCompletedCutTiers = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chop", meta = (ClampMin = "1"))
    int32 ChopsPerTier = DefaultChopsPerTier;

    /** Icons on WBP_ProgressBar (whole + prep tiers). Anchors are spaced evenly: 0 .. 1. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chop|Progress Bar", meta = (ClampMin = "1", ClampMax = "4"))
    int32 ProgressBarTierIconCount = 4;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chop|Preparations", meta = (Categories = "Prep"))
    FGameplayTag SlicedPreparationTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chop|Preparations", meta = (Categories = "Prep"))
    FGameplayTag ChoppedPreparationTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chop|Preparations", meta = (Categories = "Prep"))
    FGameplayTag MincedPreparationTag;

    UPROPERTY(BlueprintAssignable, Category = "Chop")
    FPUOnChopProgressUpdated OnChopProgressUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Chop")
    FPUOnChopCommitFinished OnChopCommitFinished;

    UPROPERTY(BlueprintAssignable, Category = "Chop")
    FPUOnChopStrokePlayed OnChopStrokePlayed;

    UPROPERTY(BlueprintAssignable, Category = "Chop")
    FPUOnChopStrokeReleased OnChopStrokeReleased;

    /** Fired when a cut tier completes (not every chop stroke). Optional BP hook; C++ updates FoodToBeChopped directly. */
    UPROPERTY(BlueprintAssignable, Category = "Chop|Presentation")
    FPUOnChopFoodVisualUpdated OnChopFoodVisualUpdated;

    UFUNCTION(BlueprintPure, Category = "Chop")
    bool IsChopStrikeKeyHeld() const { return bChopStrikeKeyHeld; }

    /** Cut art for the current completed tier (whole → prepped/preview; then sliced / chopped / minced). */
    UFUNCTION(BlueprintPure, Category = "Chop|Presentation")
    UTexture2D* GetMinigameIngredientDisplayTexture() const;

    /** Multiply tint for cut-minigame food image: white when whole; boosted AverageTintColor for cut tiers. */
    UFUNCTION(BlueprintPure, Category = "Chop|Presentation")
    FLinearColor GetMinigameIngredientDisplayTint() const;

    UPROPERTY(BlueprintReadOnly, Category = "Chop")
    int32 CompletedCutTierCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Chop")
    int32 ChopsTowardNextTier = 0;

    UFUNCTION(BlueprintPure, Category = "Chop")
    EPUChopCompletedCutTier GetCompletedCutTier() const;

    UFUNCTION(BlueprintPure, Category = "Chop")
    EPUChopCompletedCutTier GetTargetCutTier() const;

    UFUNCTION(BlueprintPure, Category = "Chop")
    float GetChopTierProgressNormalized() const;

    /** Total chops recorded this session toward minced (0 .. MaxCompletedCutTiers * ChopsPerTier). */
    UFUNCTION(BlueprintPure, Category = "Chop|Progress Bar")
    int32 GetTotalChopStrokesCompleted() const;

    UFUNCTION(BlueprintPure, Category = "Chop|Progress Bar")
    int32 GetTotalChopStrokesRequired() const;

    /**
     * 0–1 along the bar for gradient fill width and yellow marker X (9 chops = full bar).
     * Matches mockup: fill + triangle share this value.
     */
    UFUNCTION(BlueprintPure, Category = "Chop|Progress Bar")
    float GetOverallChopProgressNormalized() const;

    /**
     * Evenly spaced anchor for tier icons (0=whole … 3=minced). Default maps to 0, 1/3, 2/3, 1 on the bar track.
     */
    UFUNCTION(BlueprintPure, Category = "Chop|Progress Bar")
    int32 GetProgressBarTierIconCount() const;

    UFUNCTION(BlueprintPure, Category = "Chop|Progress Bar")
    float GetChopTierAnchorPercent(int32 TierIconIndex) const;

    /** Per-icon highlight for the four circles on the bar. TierIconIndex 0=whole … 3=minced. */
    UFUNCTION(BlueprintPure, Category = "Chop|Progress Bar")
    EPUChopTierIconState GetChopTierIconState(int32 TierIconIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Chop")
    void RegisterChopInput();

    UFUNCTION(BlueprintCallable, Category = "Chop")
    bool FinishChoppingAndApply();

    virtual void HandleStageModuleInitialized_Implementation(
        const FPUDishCustomizationStageDescriptor& StageDescriptor) override;

    virtual void HandleStripMinigameSessionChanged_Implementation(bool bActive, UPUIngredientSlot* StripSlot) override;

    virtual bool TryConsumeMinigameKey_Implementation(FKey Key) override;
    virtual bool TryReleaseMinigameKey_Implementation(FKey Key) override;

    virtual bool CanStartStripMinigameForSlot_Implementation(const UPUIngredientSlot* StripSlot) const override;

    virtual void DisconnectFromOwner(
        UPUPipelineStageMinigameModuleWidget* OwnerWidget,
        UObject* ProgressBarSubscriber) override;

    virtual void SanitizeStaleObjectReferences() override;

protected:
    void BeginChopStroke();
    void EndChopStroke();

    void ResetChopSession();
    void BroadcastChopProgress();
    /** Updates FoodToBeChopped on the owner module — call only when CompletedCutTierCount changes or session resets. */
    void NotifyChopFoodVisualTierChanged();
    void DisconnectChopDelegates(UObject* OwnerWidget, UObject* ProgressBarSubscriber);
    static EPUIngredientCutVisualTier ChopTierToIngredientVisualTier(EPUChopCompletedCutTier Tier);
    bool ApplyCompletedCutTierToStripSlot(UPUIngredientSlot* StripSlot, EPUChopCompletedCutTier Tier);
    void RemoveMutuallyExclusiveCutPreparations(UPUIngredientSlot* StripSlot);
    FGameplayTag GetPreparationTagForCompletedTier(EPUChopCompletedCutTier Tier) const;
    static bool IsChopKey(FKey Key);
    static bool IsFinishChopKey(FKey Key);
    void EnsureDefaultPreparationTags();

    UPROPERTY()
    TObjectPtr<UPUIngredientSlot> ActiveChopStripSlot;

    bool bChopStrikeKeyHeld = false;
};
