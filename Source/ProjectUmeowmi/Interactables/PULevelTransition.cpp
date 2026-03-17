#include "PULevelTransition.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DlgSystem/DlgContext.h"
#include "DlgSystem/DlgDialogue.h"

APULevelTransition::APULevelTransition()
{
	PrimaryActorTick.bCanEverTick = false;

	// Configure talking-object defaults (sphere radius synced by base TalkingObject::SyncInteractionSphereToRange)
	ObjectType = ETalkingObjectType::System;
	InteractionRange = 250.0f;

	// Default values
	bAutoTrigger = false;
	bUseFadeTransition = true;
}

void APULevelTransition::BeginPlay()
{
	Super::BeginPlay();

	// Bind additional overlap handler for optional auto-trigger behavior
	if (InteractionSphere)
	{
		InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &APULevelTransition::OnTransitionSphereBeginOverlap);
	}

	// Validate configuration
	if (TargetLevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PULevelTransition '%s' has no TargetLevelName set!"), *GetName());
	}
}

bool APULevelTransition::IsUnlocked() const
{
	UPUProjectUmeowmiGameInstance* GameInstance = Cast<UPUProjectUmeowmiGameInstance>(GetGameInstance());
	return GameInstance ? GameInstance->IsLevelTransitionUnlocked(LockID) : (LockID == NAME_None);
}

bool APULevelTransition::CanInteract() const
{
	// Show prompt when: unlocked (can transition) OR locked with a LockedDialogue (can trigger dialogue)
	if (!IsPlayerInRange())
	{
		return false;
	}
	if (IsUnlocked())
	{
		return true;
	}
	// Locked: show prompt only if we have a dialogue to play
	return LockedDialogue != nullptr;
}

void APULevelTransition::StartInteraction()
{
	if (!IsPlayerInRange())
	{
		return;
	}

	if (IsUnlocked())
	{
		if (TargetLevelName.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Cannot transition: TargetLevelName is empty for %s"), *GetName());
			return;
		}
		PerformTransition();
		return;
	}

	// Locked: trigger the locked dialogue instead
	if (LockedDialogue)
	{
		UE_LOG(LogTemp, Log, TEXT("Level transition %s is locked - starting locked dialogue"), *GetName());
		StartDialogueAndSetInteracting(LockedDialogue);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Level transition %s is locked but has no LockedDialogue set"), *GetName());
	}
}

void APULevelTransition::PerformTransition()
{
	UPUProjectUmeowmiGameInstance* GameInstance = Cast<UPUProjectUmeowmiGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get GameInstance for level transition"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Transitioning to level: %s (Spawn Point: %s)"),
		*TargetLevelName, *TargetSpawnPointTag.ToString());

	// Trigger the transition
	GameInstance->TransitionToLevel(TargetLevelName, TargetSpawnPointTag, bUseFadeTransition);
}

void APULevelTransition::OnTransitionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only trigger for player character
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Character->IsPlayerControlled())
	{
		return;
	}

	// If auto-trigger is enabled and unlocked, perform transition immediately on overlap
	if (bAutoTrigger && IsUnlocked())
	{
		PerformTransition();
	}
	// Otherwise, we rely on the normal talking-object interaction flow:
	// - Base class registers this talking object with the character
	// - Character's Interact input calls StartInteraction()
}

bool APULevelTransition::OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName)
{
	// Handle unlock event - unlocks this transition's LockID (can be called from dialogue)
	if (EventName == TEXT("UnlockLevelTransition") || EventName == TEXT("UnlockTransition"))
	{
		if (LockID != NAME_None)
		{
			if (UPUProjectUmeowmiGameInstance* GI = Cast<UPUProjectUmeowmiGameInstance>(GetGameInstance()))
			{
				GI->UnlockLevelTransition(LockID);
				UE_LOG(LogTemp, Log, TEXT("APULevelTransition::OnDialogueEvent - Unlocked level transition: %s"), *LockID.ToString());
			}
		}
		return true;
	}

	// Handle level transition event from dialogue
	if (EventName == TEXT("TransitionLevel") || EventName == TEXT("LevelTransition"))
	{
		if (!IsUnlocked())
		{
			UE_LOG(LogTemp, Log, TEXT("APULevelTransition::OnDialogueEvent - Level transition triggered from dialogue but locked (LockID: %s)"), *LockID.ToString());
			return false;
		}
		UE_LOG(LogTemp, Log, TEXT("APULevelTransition::OnDialogueEvent - Level transition triggered from dialogue"));
		
		// Perform the transition using the configured target level and spawn point
		PerformTransition();
		return true;
	}
	
	// Let base class handle other events (like GenerateOrder, etc.)
	return Super::OnDialogueEvent_Implementation(Context, EventName);
}