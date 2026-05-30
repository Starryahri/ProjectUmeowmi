#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Interfaces/PUCustomizationStageModuleInterface.h"
#include "PUCookingStripMinigameBehavior.h"
#include "PUPipelineStageMinigameModuleWidget.generated.h"

struct FKey;

class UWidgetAnimation;
class UPUDishCustomizationWidget;
class UPUDishCustomizationComponent;
class UPUIngredientSlot;
class UImage;
class UPanelWidget;
class UTexture2D;
class UPUIngredientDragDropOperation;
class UCanvasPanel;
class UPUStripMinigameBehavior;
class UPUStripMinigameProgressBarWidget;
class UPUPlatingStripMinigameBehavior;

/** Runtime-spawned bowl images for one ingredient layer (2nd rail ingredient = layer 0, etc.). */
USTRUCT()
struct FPUMarinationBowlSpawnedLayer
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TArray<TObjectPtr<UImage>> SlotImages;
};

/**
 * Shared base for pipeline-mounted stage vignettes that run an interactive strip-minigame (chop, marinate, etc.).
 * Assign a StripMinigameBehavior (class or instanced) per widget — Blueprints stay parented here; no per-minigame reparent.
 *
 * BindWidgetOptional: StageMinigameUIPanel, FoodToBeChopped (chop food image — required name in UMG)
 * Marination bowl: place anchor images MarinationBowlSlot0 … MarinationBowlSlot4 (or nest under MarinationBowlSlotPanel).
 *   1st rail ingredient fills those anchors; each later ingredient spawns UImage children at every anchor at runtime.
 *   Place BowlFront (foreground rim) in the same panel — C++ keeps it above all ingredient images.
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

    /** Primary chop food image — UMG widget name must be FoodToBeChopped. */
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Food")
    TObjectPtr<UImage> FoodToBeChopped;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Food")
    TObjectPtr<UImage> StripMinigameFoodImage;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Stage Minigame|Food")
    TObjectPtr<UImage> ResolvedStripMinigameFoodImage;

    UPROPERTY(EditDefaultsOnly, Category = "Stage Minigame|Food", AdvancedDisplay)
    FName StripMinigameFoodImageWidgetName;

    static constexpr int32 DefaultMarinationBowlVisualCount = 5;
    static constexpr float MarinationBowlImageDrawSize = 64.f;

    /** How many bowl slot anchors exist (MarinationBowlSlot0 … N-1). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl", meta = (ClampMin = "1", ClampMax = "5"))
    int32 MarinationBowlVisualCount = DefaultMarinationBowlVisualCount;

    /** Optional panel containing MarinationBowlSlot0 … N-1 Image children. */
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TObjectPtr<UPanelWidget> MarinationBowlSlotPanel;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TObjectPtr<UImage> MarinationBowlSlot0;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TObjectPtr<UImage> MarinationBowlSlot1;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TObjectPtr<UImage> MarinationBowlSlot2;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TObjectPtr<UImage> MarinationBowlSlot3;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TObjectPtr<UImage> MarinationBowlSlot4;

    /** Foreground bowl rim/frame — always kept above ingredient slot images. */
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TObjectPtr<UImage> BowlFront;

    /** Canvas for free-form plated ingredient slots (Stage.Plating / garnish). */
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Plating")
    TObjectPtr<UCanvasPanel> PlatingDishArea;

    /** Optional full-area drop target over PlatingDishArea — name must be PlatingDishDropTarget. */
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Stage Minigame|Plating")
    TObjectPtr<UPUIngredientSlot> PlatingDishDropTarget;

    /** Draw size for runtime-spawned plated ingredient slots on the dish canvas. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Minigame|Plating", meta = (ClampMin = "16"))
    float PlatingDishSlotDrawSize = 64.f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TArray<TObjectPtr<UImage>> ResolvedMarinationBowlSlotImages;

    /** Bowl targets that are ingredient-slot widgets (MarinationBowlSlot0 … 4 by name). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TArray<TObjectPtr<UPUIngredientSlot>> ResolvedMarinationBowlIngredientSlots;

    /** Runtime UImages spawned for 2nd+ rail ingredients (one image per anchor slot). */
    UPROPERTY(Transient)
    TArray<FPUMarinationBowlSpawnedLayer> SpawnedMarinationBowlLayerImages;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Stage Minigame|Marination Bowl")
    TObjectPtr<UImage> ResolvedMarinationBowlFrontImage;

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

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Stage Minigame|Behavior")
    TObjectPtr<UPUStripMinigameBehavior> ActiveStripMinigameBehavior;

    UFUNCTION(BlueprintPure, Category = "Stage Minigame")
    bool IsStripMinigameActive() const { return bStripMinigameActive; }

    /** Strip slot this minigame session was opened on (preview + rail lock). */
    UFUNCTION(BlueprintPure, Category = "Stage Minigame")
    UPUIngredientSlot* GetStripMinigameContextStripSlot() const { return StripMinigameContextStripSlot; }

    UFUNCTION(BlueprintPure, Category = "Stage Minigame|Behavior")
    UPUStripMinigameBehavior* GetActiveStripMinigameBehavior() const { return ActiveStripMinigameBehavior; }

    /** Cast helper when Stage.Cooking (or a cooking behavior subclass) is active. */
    UFUNCTION(BlueprintPure, Category = "Stage Minigame|Cooking")
    UPUCookingStripMinigameBehavior* GetActiveCookingStripMinigameBehavior() const;

    /** Cast helper when Stage.Plating / Stage.Garnish is active. */
    UFUNCTION(BlueprintPure, Category = "Stage Minigame|Plating")
    UPUPlatingStripMinigameBehavior* GetActivePlatingStripMinigameBehavior() const;

    /** Writes 2D dish-area layout from spawned slots into OwnerShell / CustomizationComponent dish data. */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Plating")
    void SyncPlatingDishAreaToDishData();

    /** Removes runtime plated slots from the dish canvas. */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Plating")
    void ClearPlatingDishAreaVisuals();

    /** Drop from rail or rearrange an existing plated slot onto the dish canvas. Returns false if unhandled. */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Plating")
    bool TryHandlePlatingDropOnDishArea(
        UPUIngredientSlot* DropTargetSlot,
        UPUIngredientDragDropOperation* DragOperation,
        const FVector2D& LocalPositionInDropTarget);

    /** Forwards to the active cooking behavior when present. */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Cooking")
    void AdvanceCookingStep();

    /** Forwards to the active cooking behavior when present. */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Cooking")
    void ConfirmCookingStep();

    /** 0-based index into the dish CustomizationStages array for the active pipeline step, or INDEX_NONE. */
    UFUNCTION(BlueprintPure, Category = "Stage Minigame|Pipeline", meta = (DisplayName = "Get Current Stage Index"))
    int32 GetCurrentStageIndex() const;

    UFUNCTION(BlueprintCallable, Category = "Stage Minigame")
    virtual void SetStripMinigameActive(bool bActive, UPUIngredientSlot* ContextStripSlot);

    virtual bool TryConsumeStripMinigameKey(const FKey& Key);
    virtual bool TryReleaseStripMinigameKey(const FKey& Key);

    virtual void InitializeStageModule_Implementation(
        UPUDishCustomizationWidget* InOwnerShell,
        UPUDishCustomizationComponent* InCustomizationComponent,
        const FPUDishCustomizationStageDescriptor& StageDescriptor) override;

    virtual void ShutdownStageModule_Implementation() override;

    /** Clears dead shell/component/behavior refs (safe before GC). */
    void SanitizeStaleObjectReferences();

    static void SanitizeAllLiveStageMinigameModules();

    virtual void OnIngredientStripSlotFocusChanged_Implementation(UPUIngredientSlot* StripSlot) override;

    virtual void OnIngredientAddedToStripSlot_Implementation(
        UPUIngredientSlot* StripSlot,
        const FIngredientInstance& IngredientInstance) override;

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

    /** Chop stage: food image texture for the active strip ingredient's current cut tier. */
    UFUNCTION(BlueprintPure, Category = "Stage Minigame|Chop Presentation")
    UTexture2D* GetStripMinigameFoodTexture() const;

    /** Chop stage: multiply tint — white for whole; boosted AverageTintColor for sliced/chopped/minced grayscale art. */
    UFUNCTION(BlueprintPure, Category = "Stage Minigame|Chop Presentation")
    FLinearColor GetStripMinigameFoodTint() const;

    /** Pushes current chop food texture + tint to FoodToBeChopped (on minigame open and when a cut tier completes). */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Chop Presentation")
    void ApplyStripMinigameFoodVisual();

    /** P / gamepad A pressed during marinate minigame — plays mix animation, then ReceiveStripMinigameMixPressed. */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Marinate Presentation")
    void NotifyStripMinigameMixPressed();

    /** P / gamepad A released: ReceiveStripMinigameMixReleased (restore brush here — no reverse animation). */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Marinate Presentation")
    void NotifyStripMinigameMixReleased();

    /** Marination: 1st ingredient fills anchor images; later ones spawn images at every anchor slot. */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Marination Bowl")
    void ApplyMarinationBowlVisualsForIngredient(
        const FIngredientInstance& IngredientInstance,
        UPUIngredientSlot* SourceStripSlot = nullptr,
        int32 BowlIngredientIndex = 0);

    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Marination Bowl")
    void ClearMarinationBowlVisuals();

    /** Clears one rail-ingredient layer (0 = base MarinationBowlSlot images). */
    UFUNCTION(BlueprintCallable, Category = "Stage Minigame|Marination Bowl")
    void ClearMarinationBowlLayerVisuals(int32 BowlIngredientIndex);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void BeginDestroy() override;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Presentation", meta = (DisplayName = "On Strip Minigame Chop Pressed"))
    void ReceiveStripMinigameChopPressed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Presentation", meta = (DisplayName = "On Strip Minigame Chop Released"))
    void ReceiveStripMinigameChopReleased();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Presentation", meta = (DisplayName = "On Strip Minigame Chop Played", DeprecatedFunction, DeprecationMessage = "Use On Strip Minigame Chop Pressed"))
    void ReceiveStripMinigameChopPlayed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Marinate Presentation", meta = (DisplayName = "On Strip Minigame Mix Pressed"))
    void ReceiveStripMinigameMixPressed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Marinate Presentation", meta = (DisplayName = "On Strip Minigame Mix Released"))
    void ReceiveStripMinigameMixReleased();

    /** Fired when the active cooking behavior enters a new step (bind on your cooking stage Blueprint). */
    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Cooking", meta = (DisplayName = "On Cooking Step Changed"))
    void ReceiveCookingStepChanged(int32 StepIndex, FPUCookingMinigameStepDescriptor StepDescriptor);

    /** Fired while the active cooking step advances (bar listens internally; use for extra VFX). */
    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Cooking", meta = (DisplayName = "On Cooking Progress Updated"))
    void ReceiveCookingProgressUpdated(
        int32 StepIndex,
        int32 StepProgressCompleted,
        int32 StepProgressRequired,
        float OverallProgressNormalized,
        int32 TotalStepCount);

    /** Fired when the cooking session commits Prep.Cook and closes. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame|Cooking", meta = (DisplayName = "On Cooking Commit Finished"))
    void ReceiveCookingCommitFinished(bool bAppliedPreparation);

    /** Single chop animation (e.g. knife down). BindWidgetAnim name must match the animation asset name in the UMG designer. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> ChopStrikeAnimation;

    /** Optional A/B chop animations (alternate each press when both are set). */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> ChopStrikeAnimationA;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> ChopStrikeAnimationB;

    /** Single mix animation for marinate minigame (e.g. spoon stir). */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> MixStrikeAnimation;

    /** Optional A/B mix animations (alternate each press when both are set). */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> MixStrikeAnimationA;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> MixStrikeAnimationB;

    UFUNCTION(BlueprintNativeEvent, Category = "Stage Minigame|Presentation", meta = (DisplayName = "Play Strip Minigame Chop Animation"))
    void PlayStripMinigameChopAnimation();
    virtual void PlayStripMinigameChopAnimation_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = "Stage Minigame|Marinate Presentation", meta = (DisplayName = "Play Strip Minigame Mix Animation"))
    void PlayStripMinigameMixAnimation();
    virtual void PlayStripMinigameMixAnimation_Implementation();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stage Minigame", meta = (DisplayName = "On Strip Minigame Presentation Changed"))
    void ReceiveStripMinigamePresentationChanged(bool bActive, UPUIngredientSlot* StripSlot);

    UFUNCTION(BlueprintNativeEvent, Category = "Stage Minigame", meta = (DisplayName = "Handle Ingredient Strip Slot Focus Changed"))
    void HandleIngredientStripSlotFocusChanged(UPUIngredientSlot* StripSlot);

    UFUNCTION(BlueprintNativeEvent, Category = "Stage Minigame|Behavior")
    TSubclassOf<UPUStripMinigameBehavior> ResolveStripMinigameBehaviorClass(
        const FPUDishCustomizationStageDescriptor& StageDescriptor) const;

private:
    void ResolveStripMinigameProgressBarWidget();
    void ResolveStripMinigameFoodImageWidget();
    void ResolveMarinationBowlSlotTargets();
    void ResolveMarinationBowlFrontImage();
    void EnsureMarinationBowlFrontOnTop();
    static bool TryGetMarinationBowlDisplayVisual(
        const FIngredientInstance& IngredientInstance,
        UTexture2D*& OutTexture,
        FLinearColor& OutTint);
    bool ResolveMarinationIngredientVisual(
        const FIngredientInstance& IngredientInstance,
        UPUIngredientSlot* SourceStripSlot,
        UTexture2D*& OutTexture,
        FLinearColor& OutTint) const;
    UWidget* GetMarinationBowlAnchorWidget(int32 SlotIndex) const;
    void ApplyMarinationBowlImageVisual(UImage* BowlImage, UTexture2D* Texture, const FLinearColor& Tint);
    void ApplyMarinationBaseBowlVisuals(UTexture2D* Texture, const FLinearColor& Tint, const FIngredientInstance& IngredientInstance);
    void SpawnMarinationBowlImageAtSlot(
        int32 LayerIndex,
        int32 SlotIndex,
        UTexture2D* Texture,
        const FLinearColor& Tint,
        UWidget* AnchorWidget);
    void ClearSpawnedMarinationBowlImages();

    void ResolvePlatingDishAreaWidgets();
    UPUIngredientSlot* SpawnPlatedIngredientOnDishArea(
        const FIngredientInstance& IngredientInstance,
        const FVector2D& LocalPositionInCanvas);
    bool TryMovePlatedIngredientOnDishArea(UPUIngredientSlot* ArrangementSlot, const FVector2D& LocalPositionInCanvas);
    static FVector2D ComputePlatingLocalPositionInCanvas(
        UCanvasPanel* Canvas,
        UWidget* DropTargetWidget,
        const FVector2D& LocalPositionInDropTarget);

    /** Browsing preview on `FoodToBeChopped` while minigame is inactive — whole art, white tint (clears stale cut-tier multiply). */
    void ApplyStripSlotFocusPreviewVisual(UPUIngredientSlot* StripSlot);

    void ApplyStageMinigameUIPanelVisibility();
    void SetupStripMinigameBehavior(const FPUDishCustomizationStageDescriptor& StageDescriptor);
    void TeardownStripMinigameBehavior();
    void BindCookingStripMinigamePresentation(UPUCookingStripMinigameBehavior* CookingBehavior);
    void UnbindCookingStripMinigamePresentation(UPUCookingStripMinigameBehavior* CookingBehavior);

    UFUNCTION()
    void HandleCookingStepChangedForwarded(int32 StepIndex, FPUCookingMinigameStepDescriptor StepDescriptor);

    UFUNCTION()
    void HandleCookingProgressUpdatedForwarded(
        int32 StepIndex,
        int32 StepProgressCompleted,
        int32 StepProgressRequired,
        float OverallProgressNormalized,
        int32 TotalStepCount);

    UFUNCTION()
    void HandleCookingCommitFinishedForwarded(bool bAppliedPreparation);

    FPUDishCustomizationStageDescriptor CachedStageDescriptor;
    bool bHasCachedStageDescriptor = false;

    UPROPERTY(Transient)
    TObjectPtr<UPUIngredientSlot> StripMinigameContextStripSlot;

    bool bNextChopStrikeUsesAnimationA = true;
    bool bNextMixStrikeUsesAnimationA = true;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UPUIngredientSlot>> SpawnedPlatingDishSlots;
};
