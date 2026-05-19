#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Interfaces/PUCustomizationStageModuleInterface.h"
#include "PUPipelineStageMinigameModuleWidget.generated.h"

struct FKey;

class UWidgetAnimation;
class UPUDishCustomizationWidget;
class UPUDishCustomizationComponent;
class UPUIngredientSlot;
class UPUStripMinigameBehavior;
class UPUStripMinigameProgressBarWidget;

/**
 * Shared base for pipeline-mounted stage vignettes that run an interactive strip-minigame (chop, marinate, etc.).
 * Assign a StripMinigameBehavior (class or instanced) per widget — Blueprints stay parented here; no per-minigame reparent.
 *
 * BindWidgetOptional: StageMinigameUIPanel
 * Progress bar: nest WBP_ProgressBar (parent: Strip Minigame Progress Bar) under StageMinigameUIPanel — any instance name; C++ discovers it at runtime.
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Pipeline Stage Minigame Module"))
class PROJECTUMEOWMI_API UPUPipelineStageMinigameModuleWidget : public UUserWidget, public IPUCustomizationStageModuleInterface
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Stage Minigame|Shell")
    TObjectPtr<UPUDishCustomizationWidget> OwnerShell;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Minigame|Shell")
    TObjectPtr<UPUDishCustomizationComponent> CustomizationComponent;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame")
    TObjectPtr<UWidget> StageMinigameUIPanel;

    /** Runtime reference to WBP_ProgressBar (or any Strip Minigame Progress Bar child under StageMinigameUIPanel). Not a BindWidget — do not rename the instance to StripMinigameProgressBar. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Stage Minigame|Progress Bar")
    TObjectPtr<UPUStripMinigameProgressBarWidget> StripMinigameProgressBar;

    /** Optional: find by this name before searching StageMinigameUIPanel descendants. Leave None for auto-discovery. */
    UPROPERTY(EditDefaultsOnly, Category = "Stage Minigame|Progress Bar", AdvancedDisplay)
    FName StripMinigameProgressBarWidgetName;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Minigame")
    bool bStripMinigameActive = false;

    /** Optional inline behavior (wins over class + stage map). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Minigame|Behavior", meta = (DisplayName = "Strip Minigame Behavior (Instanced)"))
    TObjectPtr<UPUStripMinigameBehavior> StripMinigameBehavior;

    /** Spawned when no instanced behavior is set. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Minigame|Behavior")
    TSubclassOf<UPUStripMinigameBehavior> StripMinigameBehaviorClass;

    /** Fallback when class is unset — e.g. Stage.Chopping → chop behavior. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Minigame|Behavior", meta = (Categories = "Stage"))
    TMap<FGameplayTag, TSubclassOf<UPUStripMinigameBehavior>> StripMinigameBehaviorByStageId;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Minigame|Behavior")
    TObjectPtr<UPUStripMinigameBehavior> ActiveStripMinigameBehavior;

    UFUNCTION(BlueprintPure, Category = "Stage Minigame")
    bool IsStripMinigameActive() const { return bStripMinigameActive; }

    /** Strip slot this minigame session was opened on (preview + rail lock). */
    UFUNCTION(BlueprintPure, Category = "Stage Minigame")
    UPUIngredientSlot* GetStripMinigameContextStripSlot() const { return StripMinigameContextStripSlot; }

    UFUNCTION(BlueprintPure, Category = "Stage Minigame|Behavior")
    UPUStripMinigameBehavior* GetActiveStripMinigameBehavior() const { return ActiveStripMinigameBehavior; }

    UFUNCTION(BlueprintCallable, Category = "Stage Minigame")
    virtual void SetStripMinigameActive(bool bActive, UPUIngredientSlot* ContextStripSlot);

    virtual bool TryConsumeStripMinigameKey(const FKey& Key);
    virtual bool TryReleaseStripMinigameKey(const FKey& Key);

    virtual void InitializeStageModule_Implementation(
        UPUDishCustomizationWidget* InOwnerShell,
        UPUDishCustomizationComponent* InCustomizationComponent,
        const FPUDishCustomizationStageDescriptor& StageDescriptor) override;

    virtual void ShutdownStageModule_Implementation() override;

    virtual void OnIngredientStripSlotFocusChanged_Implementation(UPUIngredientSlot* StripSlot) override;

    virtual bool ToggleStageMinigameFromIngredientStripSlot_Implementation(UPUIngredientSlot* StripSlot) override;

    /** P / gamepad A pressed: play chop animation, then ReceiveStripMinigameChopPressed (set held brush here). */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Presentation")
    void NotifyStripMinigameChopPressed();

    /** P / gamepad A released: ReceiveStripMinigameChopReleased (restore brush here — no reverse animation). */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Presentation")
    void NotifyStripMinigameChopReleased();

    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Presentation", meta = (DeprecatedFunction, DeprecationMessage = "Use NotifyStripMinigameChopPressed"))
    void NotifyStripMinigameChopPlayed() { NotifyStripMinigameChopPressed(); }

    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Progress Bar")
    void SyncStripMinigameProgressBarBinding();

    virtual void NativeConstruct() override;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Presentation", meta = (DisplayName = "On Strip Minigame Chop Pressed"))
    void ReceiveStripMinigameChopPressed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Presentation", meta = (DisplayName = "On Strip Minigame Chop Released"))
    void ReceiveStripMinigameChopReleased();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Presentation", meta = (DisplayName = "On Strip Minigame Chop Played", DeprecatedFunction, DeprecationMessage = "Use On Strip Minigame Chop Pressed"))
    void ReceiveStripMinigameChopPlayed();

    /** Single chop animation (e.g. knife down). BindWidgetAnim name must match the animation asset name in the UMG designer. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> ChopStrikeAnimation;

    /** Optional A/B chop animations (alternate each press when both are set). */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> ChopStrikeAnimationA;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> ChopStrikeAnimationB;

    UFUNCTION(BlueprintNativeEvent, Category = "Stage Minigame|Presentation", meta = (DisplayName = "Play Strip Minigame Chop Animation"))
    void PlayStripMinigameChopAnimation();
    virtual void PlayStripMinigameChopAnimation_Implementation();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame", meta = (DisplayName = "On Strip Minigame Presentation Changed"))
    void ReceiveStripMinigamePresentationChanged(bool bActive, UPUIngredientSlot* StripSlot);

    UFUNCTION(BlueprintNativeEvent, Category = "Stage Minigame", meta = (DisplayName = "Handle Ingredient Strip Slot Focus Changed"))
    void HandleIngredientStripSlotFocusChanged(UPUIngredientSlot* StripSlot);

    UFUNCTION(BlueprintNativeEvent, Category = "Stage Minigame|Behavior")
    TSubclassOf<UPUStripMinigameBehavior> ResolveStripMinigameBehaviorClass(
        const FPUDishCustomizationStageDescriptor& StageDescriptor) const;

private:
    void ResolveStripMinigameProgressBarWidget();
    void ApplyStageMinigameUIPanelVisibility();
    void SetupStripMinigameBehavior(const FPUDishCustomizationStageDescriptor& StageDescriptor);
    void TeardownStripMinigameBehavior();

    FPUDishCustomizationStageDescriptor CachedStageDescriptor;
    bool bHasCachedStageDescriptor = false;

    UPROPERTY(Transient)
    TObjectPtr<UPUIngredientSlot> StripMinigameContextStripSlot;

    bool bNextChopStrikeUsesAnimationA = true;
};
