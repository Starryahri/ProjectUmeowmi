#include "PULevelTransition.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DlgSystem/DlgContext.h"

APULevelTransition::APULevelTransition()
{
	PrimaryActorTick.bCanEverTick = false;

	// Configure talking-object defaults
	ObjectType = ETalkingObjectType::System;
	InteractionRange = 250.0f;

	// Make sure the interaction sphere uses our range
	if (InteractionSphere)
	{
		InteractionSphere->SetSphereRadius(InteractionRange);
	}

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

bool APULevelTransition::CanInteract() const
{
	// For level transitions, we just require the player to be in range.
	// We don't rely on dialogues, unlike normal talking objects.
	return IsPlayerInRange();
}

void APULevelTransition::StartInteraction()
{
	if (TargetLevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot transition: TargetLevelName is empty for %s"), *GetName());
		return;
	}

	// Perform the transition when the player presses the interact key
	PerformTransition();
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

	// If auto-trigger is enabled, perform transition immediately on overlap
	if (bAutoTrigger)
	{
		PerformTransition();
	}
	// Otherwise, we rely on the normal talking-object interaction flow:
	// - Base class registers this talking object with the character
	// - Character's Interact input calls StartInteraction()
}

bool APULevelTransition::OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName)
{
	// Handle level transition event from dialogue
	if (EventName == TEXT("TransitionLevel") || EventName == TEXT("LevelTransition"))
	{
		UE_LOG(LogTemp, Log, TEXT("APULevelTransition::OnDialogueEvent - Level transition triggered from dialogue"));
		
		// Perform the transition using the configured target level and spawn point
		PerformTransition();
		return true;
	}
	
	// Let base class handle other events (like GenerateOrder, etc.)
	return Super::OnDialogueEvent_Implementation(Context, EventName);
}

