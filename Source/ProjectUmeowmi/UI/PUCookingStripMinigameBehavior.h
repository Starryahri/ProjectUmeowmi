#pragma once

#include "CoreMinimal.h"
#include "PUStripMinigameBehavior.h"
#include "GameplayTagContainer.h"
#include "PUCookingStripMinigameBehavior.generated.h"

class UPUIngredientSlot;

/** Fired when a rail ingredient is mirrored into the cooking pot (Blueprint visuals hook). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FPUOnCookingPotIngredientAdded,
    UPUIngredientSlot*,
    StripSlot,
    FIngredientInstance,
    IngredientInstance);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FPUOnCookingProgressUpdated,
    int32,
    CookStrokesCompleted,
    int32,
    CookStrokesRequired);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FPUOnCookingCommitFinished,
    bool,
    bAppliedPreparation);

/** Maps one ingredient-rail strip slot to its pot layer index. */
USTRUCT()
struct FPUCookingPotRailBinding
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TWeakObjectPtr<UPUIngredientSlot> StripSlot;

    UPROPERTY(Transient)
    int32 PotIngredientIndex = INDEX_NONE;
};

/**
 * Cooking strip minigame (stub): fill all required rail slots, then press P / gamepad A to fill the progress bar.
 * At 100% applies CookedPreparationTag to all rail ingredients. Multi-step cooking gameplay comes later.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Cooking Strip Minigame Behavior"))
class PROJECTUMEOWMI_API UPUCookingStripMinigameBehavior : public UPUStripMinigameBehavior
{
    GENERATED_BODY()

public:
    static constexpr int32 DefaultCookStrokesRequired = 9;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking", meta = (ClampMin = "1"))
    int32 CookStrokesRequired = DefaultCookStrokesRequired;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Preparations", meta = (Categories = "Prep"))
    FGameplayTag CookedPreparationTag;

    UPROPERTY(BlueprintAssignable, Category = "Cooking")
    FPUOnCookingPotIngredientAdded OnCookingPotIngredientAdded;

    UPROPERTY(BlueprintAssignable, Category = "Cooking")
    FPUOnCookingProgressUpdated OnCookingProgressUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Cooking")
    FPUOnCookingCommitFinished OnCookingCommitFinished;

    /** Logical ingredients in the cooking pot (one entry per rail strip fill). */
    UPROPERTY(BlueprintReadOnly, Category = "Cooking|Pot")
    TArray<FIngredientInstance> CookingPotIngredients;

    UPROPERTY(BlueprintReadOnly, Category = "Cooking")
    int32 CookStrokesCompleted = 0;

    UFUNCTION(BlueprintPure, Category = "Cooking|Progress Bar")
    float GetOverallCookProgressNormalized() const;

    UFUNCTION(BlueprintPure, Category = "Cooking")
    bool IsCookingComplete() const;

    /** True when every non-empty rail slot has a matching pot entry. */
    UFUNCTION(BlueprintPure, Category = "Cooking|Gather")
    bool HasAllSelectedRailIngredientsInCookingPot() const;

    UFUNCTION(BlueprintCallable, Category = "Cooking")
    void RegisterCookInput();

    UFUNCTION(BlueprintCallable, Category = "Cooking")
    bool FinishCookingAndApply();

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
    TArray<FPUCookingPotRailBinding> CookingRailBindings;

    UPROPERTY()
    TObjectPtr<UPUIngredientSlot> ActiveCookingStripSlot;

    bool bCookStrikeKeyHeld = false;

    UFUNCTION()
    void HandleCookingRailStripSlotIngredientChanged(const FIngredientInstance& IngredientInstance);

    void EnsureDefaultPreparationTag();
    void RegisterCookingRailStripSlot(UPUIngredientSlot* StripSlot, int32 PotIngredientIndex);
    void UnregisterAllCookingRailStripSlots();
    void SyncCookingPotFromRailBindings();
    int32 FindPotIngredientIndexForStripSlot(const UPUIngredientSlot* StripSlot) const;
    void ResetCookSession();
    void BroadcastCookProgress();
    bool ApplyCookedPreparationToRailSlots();
    bool IngredientHasCookedPreparation(const FIngredientInstance& IngredientInstance) const;
    static bool IsCookStrikeKey(FKey Key);
    void DisconnectCookingDelegates(UObject* ProgressBarSubscriber);
};
