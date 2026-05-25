#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUStripMinigameBehavior.generated.h"

struct FKey;

class UPUPipelineStageMinigameModuleWidget;
class UPUIngredientSlot;

/**
 * Pluggable rules/input for a pipeline stage vignette minigame.
 * Assign on the stage widget (class or instanced) or map by StageId — no per-minigame widget reparent.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Strip Minigame Behavior"))
class PROJECTUMEOWMI_API UPUStripMinigameBehavior : public UObject
{
    GENERATED_BODY()

public:
    void InitializeBehavior(UPUPipelineStageMinigameModuleWidget* InOwnerModule);

    UFUNCTION(BlueprintPure, Category = "Strip Minigame Behavior")
    UPUPipelineStageMinigameModuleWidget* GetOwnerModule() const { return OwnerModule; }

    /** Called once when the stage module mounts. */
    UFUNCTION(BlueprintNativeEvent, Category = "Strip Minigame Behavior")
    void HandleStageModuleInitialized(const FPUDishCustomizationStageDescriptor& StageDescriptor);
    virtual void HandleStageModuleInitialized_Implementation(const FPUDishCustomizationStageDescriptor& StageDescriptor);

    /** Strip minigame session opened/closed on the owner module. */
    UFUNCTION(BlueprintNativeEvent, Category = "Strip Minigame Behavior")
    void HandleStripMinigameSessionChanged(bool bActive, UPUIngredientSlot* StripSlot);
    virtual void HandleStripMinigameSessionChanged_Implementation(bool bActive, UPUIngredientSlot* StripSlot);

    /** Return true when the key was consumed (chop, finish, etc.). */
    UFUNCTION(BlueprintNativeEvent, Category = "Strip Minigame Behavior")
    bool TryConsumeMinigameKey(FKey Key);
    virtual bool TryConsumeMinigameKey_Implementation(FKey Key);

    /** Key released while strip minigame is active (e.g. revert chop-held visuals). */
    UFUNCTION(BlueprintNativeEvent, Category = "Strip Minigame Behavior")
    bool TryReleaseMinigameKey(FKey Key);
    virtual bool TryReleaseMinigameKey_Implementation(FKey Key);

    /** False blocks Y / toggle from opening a new session (e.g. ingredient already chopped). Closing an active session is unaffected. */
    UFUNCTION(BlueprintNativeEvent, Category = "Strip Minigame Behavior")
    bool CanStartStripMinigameForSlot(const UPUIngredientSlot* StripSlot) const;
    virtual bool CanStartStripMinigameForSlot_Implementation(const UPUIngredientSlot* StripSlot) const;

    /** Clears owner/slot refs and dynamic delegate bindings before the behavior or owner widget is destroyed. */
    virtual void DisconnectFromOwner(UPUPipelineStageMinigameModuleWidget* OwnerWidget, UObject* ProgressBarSubscriber);

    /** Clears dead owner/slot refs (safe before GC). */
    virtual void SanitizeStaleObjectReferences();

    static void SanitizeAllLiveStripMinigameBehaviors();

protected:
    UFUNCTION(BlueprintCallable, Category = "Strip Minigame Behavior")
    void RequestEndStripMinigameSession();

    UPROPERTY(BlueprintReadOnly, Category = "Strip Minigame Behavior")
    TObjectPtr<UPUPipelineStageMinigameModuleWidget> OwnerModule;
};
