#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interfaces/PUInteractableInterface.h"
#include "PUInteractableBase.generated.h"

UCLASS()
class PROJECTUMEOWMI_API APUInteractableBase : public AActor, public IPUInteractableInterface
{
    GENERATED_BODY()

public:
    APUInteractableBase();

    // IPUInteractableInterface implementation
    virtual bool CanInteract() const override;
    virtual void StartInteraction() override;
    virtual void EndInteraction() override;
    virtual FText GetInteractionText() const override;
    virtual FText GetInteractionDescription() const override;
    virtual float GetInteractionRange() const override;
    virtual bool IsInteractable() const override;

    // Delegate getters
    virtual FOnInteractionStarted& OnInteractionStarted() override { return OnInteractionStartedDelegate; }
    virtual FOnInteractionEnded& OnInteractionEnded() override { return OnInteractionEndedDelegate; }
    virtual FOnInteractionRangeEntered& OnInteractionRangeEntered() override { return OnInteractionRangeEnteredDelegate; }
    virtual FOnInteractionRangeExited& OnInteractionRangeExited() override { return OnInteractionRangeExitedDelegate; }
    virtual FOnInteractionStateChanged& OnInteractionStateChanged() override { return OnInteractionStateChangedDelegate; }
    virtual FOnInteractionFailed& OnInteractionFailed() override { return OnInteractionFailedDelegate; }

protected:
    // Interaction properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText InteractionText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText InteractionDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float InteractionRange = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bIsInteractable = true;

    // Delegates
    FOnInteractionStarted OnInteractionStartedDelegate;
    FOnInteractionEnded OnInteractionEndedDelegate;
    FOnInteractionRangeEntered OnInteractionRangeEnteredDelegate;
    FOnInteractionRangeExited OnInteractionRangeExitedDelegate;
    FOnInteractionStateChanged OnInteractionStateChangedDelegate;
    FOnInteractionFailed OnInteractionFailedDelegate;

    // Helper functions
    void BroadcastInteractionStarted();
    void BroadcastInteractionEnded();
    void BroadcastInteractionRangeEntered();
    void BroadcastInteractionRangeExited();
    void BroadcastInteractionStateChanged();
    void BroadcastInteractionFailed();
}; 