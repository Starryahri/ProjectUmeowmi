#pragma once

#include "CoreMinimal.h"
#include "PUStripMinigameBehavior.h"
#include "PUPlatingStripMinigameBehavior.generated.h"

class UPUIngredientSlot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPUOnPlatingCommitFinished);

/**
 * Plating strip minigame: fill the rail, start the session, drag ingredients onto the dish area and rearrange them.
 * B / gamepad B finishes plating and ends the entire customization session (final pipeline stage).
 */
UCLASS(Blueprintable, meta = (DisplayName = "Plating Strip Minigame Behavior"))
class PROJECTUMEOWMI_API UPUPlatingStripMinigameBehavior : public UPUStripMinigameBehavior
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Plating")
    FPUOnPlatingCommitFinished OnPlatingCommitFinished;

    UFUNCTION(BlueprintCallable, Category = "Plating")
    bool FinishPlatingAndCompleteCustomization();

    virtual void HandleStripMinigameSessionChanged_Implementation(bool bActive, UPUIngredientSlot* StripSlot) override;

    virtual bool TryConsumeMinigameKey_Implementation(FKey Key) override;

    virtual bool CanStartStripMinigameForSlot_Implementation(const UPUIngredientSlot* StripSlot) const override;

    virtual void DisconnectFromOwner(
        UPUPipelineStageMinigameModuleWidget* OwnerWidget,
        UObject* ProgressBarSubscriber) override;

protected:
    static bool IsFinishPlatingKey(FKey Key);
};
