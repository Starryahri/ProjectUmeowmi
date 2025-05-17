#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PUInteractableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UPUInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class PROJECTUMEOWMI_API IPUInteractableInterface
{
    GENERATED_BODY()

public:
    // Delegates
    DECLARE_MULTICAST_DELEGATE(FOnInteractionStarted);
    DECLARE_MULTICAST_DELEGATE(FOnInteractionEnded);
    DECLARE_MULTICAST_DELEGATE(FOnInteractionRangeEntered);
    DECLARE_MULTICAST_DELEGATE(FOnInteractionRangeExited);
    DECLARE_MULTICAST_DELEGATE(FOnInteractionStateChanged);
    DECLARE_MULTICAST_DELEGATE(FOnInteractionFailed);

    // Core interaction methods
    virtual bool CanInteract() const = 0;
    virtual void StartInteraction() = 0;
    virtual void EndInteraction() = 0;
    
    // Getters for UI/feedback
    virtual FText GetInteractionText() const = 0;
    virtual FText GetInteractionDescription() const = 0;
    
    // Properties
    virtual float GetInteractionRange() const = 0;
    virtual bool IsInteractable() const = 0;

    // Delegate getters
    virtual FOnInteractionStarted& OnInteractionStarted() = 0;
    virtual FOnInteractionEnded& OnInteractionEnded() = 0;
    virtual FOnInteractionRangeEntered& OnInteractionRangeEntered() = 0;
    virtual FOnInteractionRangeExited& OnInteractionRangeExited() = 0;
    virtual FOnInteractionStateChanged& OnInteractionStateChanged() = 0;
    virtual FOnInteractionFailed& OnInteractionFailed() = 0;
}; 