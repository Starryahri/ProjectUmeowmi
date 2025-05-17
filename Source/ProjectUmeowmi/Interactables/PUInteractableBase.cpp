#include "PUInteractableBase.h"

APUInteractableBase::APUInteractableBase()
{
    PrimaryActorTick.bCanEverTick = false;
}

bool APUInteractableBase::CanInteract() const
{
    return bIsInteractable;
}

void APUInteractableBase::StartInteraction()
{
    if (CanInteract())
    {
        BroadcastInteractionStarted();
    }
    else
    {
        BroadcastInteractionFailed();
    }
}

void APUInteractableBase::EndInteraction()
{
    BroadcastInteractionEnded();
}

FText APUInteractableBase::GetInteractionText() const
{
    return InteractionText;
}

FText APUInteractableBase::GetInteractionDescription() const
{
    return InteractionDescription;
}

float APUInteractableBase::GetInteractionRange() const
{
    return InteractionRange;
}

bool APUInteractableBase::IsInteractable() const
{
    return bIsInteractable;
}

void APUInteractableBase::BroadcastInteractionStarted()
{
    OnInteractionStartedDelegate.Broadcast();
}

void APUInteractableBase::BroadcastInteractionEnded()
{
    OnInteractionEndedDelegate.Broadcast();
}

void APUInteractableBase::BroadcastInteractionRangeEntered()
{
    OnInteractionRangeEnteredDelegate.Broadcast();
}

void APUInteractableBase::BroadcastInteractionRangeExited()
{
    OnInteractionRangeExitedDelegate.Broadcast();
}

void APUInteractableBase::BroadcastInteractionStateChanged()
{
    OnInteractionStateChangedDelegate.Broadcast();
}

void APUInteractableBase::BroadcastInteractionFailed()
{
    OnInteractionFailedDelegate.Broadcast();
} 