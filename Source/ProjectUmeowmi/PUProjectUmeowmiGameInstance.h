#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DishCustomization/PUOrderBase.h"
#include "PUProjectUmeowmiGameInstance.generated.h"

class AProjectUmeowmiCharacter;
class APULevelSpawnPoint;

/**
 * GameInstance that persists across level transitions.
 * Handles level loading/unloading and player state preservation.
 */
UCLASS()
class PROJECTUMEOWMI_API UPUProjectUmeowmiGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPUProjectUmeowmiGameInstance(const FObjectInitializer& ObjectInitializer);

	/**
	 * Transition to a new level with a specific spawn point.
	 * @param TargetLevelName - The name of the level to load (e.g., "L_Chapter0_2_LolaRoom")
	 * @param SpawnPointTag - The tag/ID of the spawn point to use in the target level
	 * @param bUseFade - Whether to use a fade transition (optional, defaults to true)
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void TransitionToLevel(const FString& TargetLevelName, const FName& SpawnPointTag, bool bUseFade = true);

	/**
	 * Get the saved player state (orders, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	FPUOrderBase GetSavedPlayerOrder() const { return SavedPlayerOrder; }

	/**
	 * Check if there's a saved player order
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	bool HasSavedPlayerOrder() const { return bHasSavedOrder; }

	/**
	 * Clear the saved player order
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void ClearSavedPlayerOrder();

	/**
	 * Called when a level has finished loading.
	 * This is where we position the player at the spawn point.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void OnLevelLoaded();

	// Blueprint events
	UFUNCTION(BlueprintImplementableEvent, Category = "Level Transition")
	void OnTransitionStarted(const FString& TargetLevelName);

	UFUNCTION(BlueprintImplementableEvent, Category = "Level Transition")
	void OnTransitionCompleted();

protected:
	// Saved player state
	UPROPERTY(BlueprintReadWrite, Category = "Level Transition")
	FPUOrderBase SavedPlayerOrder;

	UPROPERTY(BlueprintReadWrite, Category = "Level Transition")
	bool bHasSavedOrder = false;

	// Current transition data
	UPROPERTY(BlueprintReadWrite, Category = "Level Transition")
	FName PendingSpawnPointTag;

	UPROPERTY(BlueprintReadWrite, Category = "Level Transition")
	bool bTransitionInProgress = false;

private:
	/**
	 * Save the current player state before transitioning
	 */
	void SavePlayerState();

	/**
	 * Restore the player state after transitioning
	 */
	void RestorePlayerState();

	/**
	 * Find and position player at spawn point
	 */
	void PositionPlayerAtSpawnPoint(const FName& SpawnPointTag);

};

