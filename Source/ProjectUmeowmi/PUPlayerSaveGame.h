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

	// Save version for future migration support
	UPROPERTY(VisibleAnywhere, Category = "Save Data")
	int32 SaveVersion = 1;
};

