#pragma once

#include "CoreMinimal.h"
#include "PUStripMinigameBehavior.h"
#include "GameplayTagContainer.h"
#include "PUCookingStripMinigameBehavior.generated.h"

class UPUIngredientSlot;

/** How the player completes one cooking step inside an active minigame session. */
UENUM(BlueprintType)
enum class EPUCookingStepInputMode : uint8
{
    /** P / gamepad A — discrete presses (marinate-style). */
    Strokes     UMETA(DisplayName = "Strokes"),
    /** Hold P / gamepad A until HoldSecondsRequired elapses. */
    Hold        UMETA(DisplayName = "Hold"),
    /** B / gamepad B — single confirm press (add lid, turn off heat, etc.). */
    Confirm     UMETA(DisplayName = "Confirm"),
    /** Passive timer — no input required (simmer, rest). */
    Wait        UMETA(DisplayName = "Wait"),
    /** P / gamepad A toggles on/off; completes when toggled to the target state. */
    Toggle      UMETA(DisplayName = "Toggle"),
    /**
     * Player selects one filled rail slot to commit into the cooking pot (main pantry suppressed).
     * Completes on click; that slot stays locked for the rest of the session. Fires OnCookingPotIngredientAdded.
     */
    AddIngredient UMETA(DisplayName = "Add Ingredient")
};

/** Per-step icon highlight on the shared progress bar (same semantics as chop tiers). */
UENUM(BlueprintType)
enum class EPUCookingStepIconState : uint8
{
    Upcoming    UMETA(DisplayName = "Upcoming"),
    InProgress  UMETA(DisplayName = "In Progress"),
    Completed   UMETA(DisplayName = "Completed")
};

/** One ordered step authored on the cooking behavior Blueprint (promote to data table later). */
USTRUCT(BlueprintType)
struct FPUCookingMinigameStepDescriptor
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step", meta = (Categories = "Cook.Step"))
    FGameplayTag StepId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step")
    FText StepDisplayName;

    /** Optional subtitle, hint, or controller prompt for this step. Leave empty to hide. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step")
    FText StepSecondaryDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step")
    EPUCookingStepInputMode InputMode = EPUCookingStepInputMode::Strokes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step", meta = (ClampMin = "1", EditCondition = "InputMode == EPUCookingStepInputMode::Strokes", EditConditionHides))
    int32 StrokesRequired = 9;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step", meta = (ClampMin = "0.1", EditCondition = "InputMode == EPUCookingStepInputMode::Hold", EditConditionHides))
    float HoldSecondsRequired = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step", meta = (ClampMin = "0.1", EditCondition = "InputMode == EPUCookingStepInputMode::Wait", EditConditionHides))
    float WaitSecondsRequired = 3.f;

    /** Toggle completes when StepToggleActive matches this value (default: true = one press to turn on). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step", meta = (EditCondition = "InputMode == EPUCookingStepInputMode::Toggle", EditConditionHides))
    bool bToggleTargetStateOn = true;

    /** Rail strip index (0-based) to fill. INDEX_NONE = first empty slot matching Add Ingredient Required Type. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step", meta = (EditCondition = "InputMode == EPUCookingStepInputMode::AddIngredient", EditConditionHides))
    int32 AddIngredientRailSlotIndex = INDEX_NONE;

    /** Ingredient.Type filter for the rail slot and prepped picker during Add Ingredient steps. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step", meta = (Categories = "Ingredient.Type", EditCondition = "InputMode == EPUCookingStepInputMode::AddIngredient", EditConditionHides))
    FGameplayTag AddIngredientRequiredType;

    /** When true, completing this step immediately advances to the next (or finishes the session on the last step). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Step")
    bool bAutoAdvanceOnComplete = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FPUOnCookingPotIngredientAdded,
    UPUIngredientSlot*,
    StripSlot,
    FIngredientInstance,
    IngredientInstance);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
    FPUOnCookingProgressUpdated,
    int32,
    StepIndex,
    int32,
    StepProgressCompleted,
    int32,
    StepProgressRequired,
    float,
    OverallProgressNormalized,
    int32,
    TotalStepCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FPUOnCookingStepChanged,
    int32,
    StepIndex,
    FPUCookingMinigameStepDescriptor,
    StepDescriptor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FPUOnCookingCommitFinished,
    bool,
    bAppliedPreparation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FPUOnCookingToggleStateChanged,
    bool,
    bToggleActive);

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
 * Cooking strip minigame: gather all required rail ingredients, then Y to open a multi-step session.
 * Steps are authored on the behavior Blueprint (`CookingSteps` array) — add one step at a time as you build the dish.
 * WBP_ProgressBar fill/marker default to the active step only; tier icons show which step is upcoming/active/done.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Cooking Strip Minigame Behavior"))
class PROJECTUMEOWMI_API UPUCookingStripMinigameBehavior : public UPUStripMinigameBehavior
{
    GENERATED_BODY()

public:
    static constexpr int32 DefaultCookStrokesRequired = 9;
    static constexpr int32 DefaultProgressBarStepIconCount = 4;

    /** Ordered steps for this dish's cooking session. Empty = one legacy stroke step (see DefaultLegacyStrokesRequired). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Steps")
    TArray<FPUCookingMinigameStepDescriptor> CookingSteps;

    /** Used when CookingSteps is empty so existing BPs keep working without re-authoring. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Steps", meta = (ClampMin = "1"))
    int32 DefaultLegacyStrokesRequired = DefaultCookStrokesRequired;

    /** Tier icons on WBP_ProgressBar — capped to step count. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Progress Bar", meta = (ClampMin = "1", ClampMax = "4"))
    int32 ProgressBarStepIconCount = DefaultProgressBarStepIconCount;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Preparations", meta = (Categories = "Prep"))
    FGameplayTag CookedPreparationTag;

    UPROPERTY(BlueprintAssignable, Category = "Cooking")
    FPUOnCookingPotIngredientAdded OnCookingPotIngredientAdded;

    UPROPERTY(BlueprintAssignable, Category = "Cooking")
    FPUOnCookingProgressUpdated OnCookingProgressUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Cooking")
    FPUOnCookingStepChanged OnCookingStepChanged;

    UPROPERTY(BlueprintAssignable, Category = "Cooking")
    FPUOnCookingCommitFinished OnCookingCommitFinished;

    UPROPERTY(BlueprintAssignable, Category = "Cooking")
    FPUOnCookingToggleStateChanged OnCookingToggleStateChanged;

    UPROPERTY(BlueprintReadOnly, Category = "Cooking|Pot")
    TArray<FIngredientInstance> CookingPotIngredients;

    UPROPERTY(BlueprintReadOnly, Category = "Cooking|Session")
    int32 CurrentStepIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Cooking|Session")
    int32 StepStrokesCompleted = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Cooking|Session")
    float StepHoldSecondsAccumulated = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Cooking|Session")
    float StepWaitSecondsAccumulated = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Cooking|Session")
    bool StepToggleActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Cooking|Session")
    bool bAddIngredientStepSatisfied = false;

    UFUNCTION(BlueprintPure, Category = "Cooking|Steps")
    int32 GetCookingStepCount() const;

    UFUNCTION(BlueprintPure, Category = "Cooking|Steps")
    bool TryGetCurrentStepDescriptor(FPUCookingMinigameStepDescriptor& OutStep) const;

    UFUNCTION(BlueprintPure, Category = "Cooking|Steps")
    bool IsCurrentStepComplete() const;

    UFUNCTION(BlueprintPure, Category = "Cooking|Steps")
    bool AreAllCookingStepsComplete() const;

    UFUNCTION(BlueprintPure, Category = "Cooking|Progress Bar")
    float GetCurrentStepProgressNormalized() const;

    UFUNCTION(BlueprintPure, Category = "Cooking|Progress Bar")
    float GetOverallCookProgressNormalized() const;

    UFUNCTION(BlueprintPure, Category = "Cooking|Progress Bar")
    int32 GetProgressBarStepIconCount() const;

    UFUNCTION(BlueprintPure, Category = "Cooking|Progress Bar")
    float GetCookingStepAnchorPercent(int32 StepIconIndex) const;

    UFUNCTION(BlueprintPure, Category = "Cooking|Progress Bar")
    EPUCookingStepIconState GetCookingStepIconState(int32 StepIconIndex) const;

    UFUNCTION(BlueprintPure, Category = "Cooking")
    bool HasAllSelectedRailIngredientsInCookingPot() const;

    UFUNCTION(BlueprintCallable, Category = "Cooking|Steps")
    void RegisterCookStrokeInput();

    /** Call from BP or C++ when a Confirm step should advance (also bound to B / gamepad B). */
    UFUNCTION(BlueprintCallable, Category = "Cooking|Steps")
    void ConfirmCurrentStep();

    UFUNCTION(BlueprintCallable, Category = "Cooking|Steps")
    void AdvanceCookingStep();

    UFUNCTION(BlueprintCallable, Category = "Cooking")
    bool FinishCookingAndApply();

    /** Called by the dish shell when the player commits a filled rail slot into the pot during an Add Ingredient step. */
    UFUNCTION(BlueprintCallable, Category = "Cooking|Steps")
    void NotifyAddIngredientRailSlotCommitted(UPUIngredientSlot* StripSlot);

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
    FTimerHandle HoldProgressTimerHandle;
    FTimerHandle WaitProgressTimerHandle;

    UFUNCTION()
    void HandleCookingRailStripSlotIngredientChanged(const FIngredientInstance& IngredientInstance);

    UFUNCTION()
    void TickHoldProgress();

    UFUNCTION()
    void TickWaitProgress();

    void EnsureDefaultPreparationTag();
    void EnsureDefaultCookingSteps();
    const FPUCookingMinigameStepDescriptor* GetStepDescriptor(int32 StepIndex) const;
    void RegisterCookingRailStripSlot(UPUIngredientSlot* StripSlot, int32 PotIngredientIndex);
    void UnregisterAllCookingRailStripSlots();
    void SyncCookingPotFromRailBindings();
    int32 FindPotIngredientIndexForStripSlot(const UPUIngredientSlot* StripSlot) const;
    void ResetCookSession();
    void BeginCurrentStep();
    void EndActiveStepShellEffects();
    void BeginCurrentStepShellEffects(const FPUCookingMinigameStepDescriptor& StepDescriptor);
    void CompleteCurrentStep();
    void BroadcastCookProgress();
    void RegisterCookToggleInput();
    void StopHoldProgressTimer();
    void StartHoldProgressTimer();
    void StopWaitProgressTimer();
    void StartWaitProgressTimer();
    bool DoesStripSlotSatisfyAddIngredientStep(const UPUIngredientSlot* StripSlot) const;
    bool ApplyCookedPreparationToRailSlots();
    bool IngredientHasCookedPreparation(const FIngredientInstance& IngredientInstance) const;
    static bool IsCookStrikeKey(FKey Key);
    static bool IsConfirmStepKey(FKey Key);
    void DisconnectCookingDelegates(UObject* ProgressBarSubscriber);
    UWorld* GetBehaviorWorld() const;

    /** Synthesized single step when CookingSteps is empty at edit time. */
    UPROPERTY(Transient)
    TArray<FPUCookingMinigameStepDescriptor> EffectiveCookingSteps;
};
