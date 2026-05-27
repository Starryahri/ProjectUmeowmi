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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FPUOnMarinateProgressUpdated,
    int32,
    MixStrokesCompleted,
    int32,
    MixStrokesRequired);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FPUOnMarinateCommitFinished,
    bool,
    bAppliedPreparation);

/** Maps one ingredient-rail strip slot to its bowl layer index. */
USTRUCT()
struct FPUMarinationBowlRailBinding
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TWeakObjectPtr<UPUIngredientSlot> StripSlot;

    UPROPERTY(Transient)
    int32 BowlIngredientIndex = INDEX_NONE;
};

/**
 * Marinate strip minigame: press P / gamepad A to fill the progress bar (hidden tier pips).
 * At 100% applies Prep.Marinate to all bowl rail ingredients. Re-open allowed until complete.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Marinate Strip Minigame Behavior"))
class PROJECTUMEOWMI_API UPUMarinateStripMinigameBehavior : public UPUStripMinigameBehavior
{
    GENERATED_BODY()

public:
    static constexpr int32 DefaultMixStrokesRequired = 9;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Marinate", meta = (ClampMin = "1"))
    int32 MixStrokesRequired = DefaultMixStrokesRequired;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Marinate|Preparations", meta = (Categories = "Prep"))
    FGameplayTag MarinatedPreparationTag;

    UPROPERTY(BlueprintAssignable, Category = "Marinate")
    FPUOnMarinationBowlIngredientAdded OnMarinationBowlIngredientAdded;

    UPROPERTY(BlueprintAssignable, Category = "Marinate")
    FPUOnMarinateProgressUpdated OnMarinateProgressUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Marinate")
    FPUOnMarinateCommitFinished OnMarinateCommitFinished;

    /** Logical ingredients in the marination bowl (one entry per rail strip fill). */
    UPROPERTY(BlueprintReadOnly, Category = "Marinate|Bowl")
    TArray<FIngredientInstance> MarinationBowlIngredients;

    UPROPERTY(BlueprintReadOnly, Category = "Marinate")
    int32 MixStrokesCompleted = 0;

    UFUNCTION(BlueprintPure, Category = "Marinate|Progress Bar")
    float GetOverallMarinateProgressNormalized() const;

    UFUNCTION(BlueprintPure, Category = "Marinate")
    bool IsMarinationComplete() const;

    UFUNCTION(BlueprintCallable, Category = "Marinate")
    void RegisterMixInput();

    UFUNCTION(BlueprintCallable, Category = "Marinate")
    bool FinishMarinationAndApply();

    virtual void HandleStageModuleInitialized_Implementation(
        const FPUDishCustomizationStageDescriptor& StageDescriptor) override;

    virtual void HandleStripMinigameSessionChanged_Implementation(bool bActive, UPUIngredientSlot* StripSlot) override;

    virtual bool TryConsumeMinigameKey_Implementation(FKey Key) override;

    virtual bool TryReleaseMinigameKey_Implementation(FKey Key) override;

    virtual bool CanStartStripMinigameForSlot_Implementation(const UPUIngredientSlot* StripSlot) const override;

    virtual void HandleIngredientAddedToStripSlot_Implementation(
        UPUIngredientSlot* StripSlot,
        const FIngredientInstance& IngredientInstance) override;

    virtual void DisconnectFromOwner(
        UPUPipelineStageMinigameModuleWidget* OwnerWidget,
        UObject* ProgressBarSubscriber) override;

protected:
    UPROPERTY(Transient)
    TArray<FPUMarinationBowlRailBinding> MarinationRailBindings;

    UPROPERTY()
    TObjectPtr<UPUIngredientSlot> ActiveMarinateStripSlot;

    bool bMixStrikeKeyHeld = false;

    UFUNCTION()
    void HandleMarinationRailStripSlotIngredientChanged(const FIngredientInstance& IngredientInstance);

    void EnsureDefaultPreparationTag();
    void RegisterMarinationRailStripSlot(UPUIngredientSlot* StripSlot, int32 BowlIngredientIndex);
    void UnregisterAllMarinationRailStripSlots();
    void SyncMarinationBowlVisualsFromRailBindings();
    int32 FindBowlIngredientIndexForStripSlot(const UPUIngredientSlot* StripSlot) const;
    void ResetMarinateSession();
    void BroadcastMarinateProgress();
    bool ApplyMarinatedPreparationToBowlRailSlots();
    bool IngredientHasMarinatedPreparation(const FIngredientInstance& IngredientInstance) const;
    static bool IsMarinateMixKey(FKey Key);
    void DisconnectMarinateDelegates(UObject* ProgressBarSubscriber);
};
