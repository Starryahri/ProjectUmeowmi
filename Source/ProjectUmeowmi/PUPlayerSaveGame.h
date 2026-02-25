#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "PUPlayerSaveGame.generated.h"

/**
 * Save game class for persisting player progress
 * Stores unlocked ingredients and dialogue states
 */
UCLASS()
class PROJECTUMEOWMI_API UPUPlayerSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPUPlayerSaveGame();

	// Unlocked ingredients (stored as gameplay tags)
	UPROPERTY(VisibleAnywhere, Category = "Save Data")
	TSet<FGameplayTag> UnlockedIngredientTags;

	// Unlocked recipes/dishes (stored as gameplay tags)
	UPROPERTY(VisibleAnywhere, Category = "Save Data")
	TSet<FGameplayTag> UnlockedDishTags;

	// Completed dialogue names (stubbed for future use)
	// Stores dialogue asset names that have been completed
	UPROPERTY(VisibleAnywhere, Category = "Save Data")
	TSet<FName> CompletedDialogueNames;

	// Unlocked level transition IDs (LockID set on APULevelTransition actors)
	// When a transition has a LockID, it's locked until UnlockLevelTransition is called
	UPROPERTY(VisibleAnywhere, Category = "Save Data")
	TSet<FName> UnlockedLevelTransitionIDs;

	// Dialogue typewriter effect settings (global, persisted)
	/** Whether dialogue text uses typewriter effect (character-by-character reveal) */
	UPROPERTY(VisibleAnywhere, Category = "Save Data")
	bool bUseDialogueTypewriterEffect = true;

	/** Delay between characters in seconds (e.g. 0.02 = fast, 0.05 = medium). Only used when typewriter is enabled. */
	UPROPERTY(VisibleAnywhere, Category = "Save Data")
	float DialogueTypewriterCharacterDelay = 0.02f;

	/** If true, clicking "Next" while typing instantly completes the text */
	UPROPERTY(VisibleAnywhere, Category = "Save Data")
	bool bDialogueTypewriterSkipOnInput = true;

	// Save version for future migration support
	UPROPERTY(VisibleAnywhere, Category = "Save Data")
	int32 SaveVersion = 1;
};

