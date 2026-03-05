#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DishCustomization/PUOrderBase.h"
#include "GameplayTagContainer.h"
#include "UI/PUPopupData.h"
#include "PUProjectUmeowmiGameInstance.generated.h"

class AProjectUmeowmiCharacter;
class APULevelSpawnPoint;
class UPUPlayerSaveGame;
class UUserWidget;
class UPUPopupWidget;
class USoundBase;

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

	virtual void Init() override;
	virtual void Shutdown() override;

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

	/** Returns true if a level transition is currently in progress (e.g. fade out, loading). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Level Transition")
	bool IsLevelTransitionInProgress() const { return bTransitionInProgress; }

	// Ingredient Inventory System
	/**
	 * Unlock an ingredient (adds it to the unlocked set)
	 * @param IngredientTag - The gameplay tag of the ingredient to unlock
	 * @param bSilent - If true, do not show the unlock popup (e.g. when adding dish ingredients to pantry)
	 * @return True if the ingredient was successfully unlocked (or was already unlocked)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ingredient Inventory", meta = (AdvancedDisplay = "1"))
	bool UnlockIngredient(const FGameplayTag& IngredientTag, bool bSilent = false);

	/**
	 * Unlock multiple ingredients at once
	 * @param IngredientTags - Array of gameplay tags to unlock
	 * @param bSilent - If true, do not show the unlock popup (e.g. when adding dish ingredients to pantry)
	 * @return Number of ingredients successfully unlocked (including ones that were already unlocked)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ingredient Inventory", meta = (AdvancedDisplay = "1"))
	int32 UnlockIngredients(const TArray<FGameplayTag>& IngredientTags, bool bSilent = false);

	/**
	 * Check if an ingredient is unlocked
	 * @param IngredientTag - The gameplay tag of the ingredient to check
	 * @return True if the ingredient is unlocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Ingredient Inventory")
	bool IsIngredientUnlocked(const FGameplayTag& IngredientTag) const;

	/**
	 * Get all unlocked ingredient tags
	 * @return Set of all unlocked ingredient gameplay tags
	 */
	UFUNCTION(BlueprintCallable, Category = "Ingredient Inventory")
	TSet<FGameplayTag> GetUnlockedIngredients() const { return UnlockedIngredientTags; }

	// Recipe/Dish Journal System
	/**
	 * Unlock a dish/recipe (adds it to the journal)
	 * @param DishTag - The gameplay tag of the dish to unlock
	 * @return True if the dish was successfully unlocked (or was already unlocked)
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Journal")
	bool UnlockDish(const FGameplayTag& DishTag);

	/**
	 * Unlock multiple dishes at once
	 * @param DishTags - Array of gameplay tags to unlock
	 * @return Number of dishes successfully unlocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Journal")
	int32 UnlockDishes(const TArray<FGameplayTag>& DishTags);

	/**
	 * Check if a dish is unlocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Journal")
	bool IsDishUnlocked(const FGameplayTag& DishTag) const;

	/**
	 * Get all unlocked dish tags
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Journal")
	TSet<FGameplayTag> GetUnlockedDishes() const { return UnlockedDishTags; }

	/**
	 * Set the dish the player is currently working on (e.g. during customization).
	 * When opening the journal recipe section during customization, this dish is shown first.
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Journal")
	void SetCurrentDishTag(const FGameplayTag& DishTag);

	/**
	 * Get the dish the player is currently working on (may be invalid)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Recipe Journal")
	FGameplayTag GetCurrentDishTag() const { return CurrentDishTag; }

	/**
	 * Clear the current dish (e.g. when exiting customization)
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Journal")
	void ClearCurrentDishTag();

	// Save/Load System
	/**
	 * Save the current game state to disk
	 * @param SlotName - The save slot name (defaults to "PlayerSave")
	 * @return True if save was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Save/Load")
	bool SaveGame(const FString& SlotName = TEXT("PlayerSave"));

	/**
	 * Load game state from disk
	 * @param SlotName - The save slot name (defaults to "PlayerSave")
	 * @return True if load was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Save/Load")
	bool LoadGame(const FString& SlotName = TEXT("PlayerSave"));

	/**
	 * Create a new game (initializes default state)
	 * This will clear all unlocked ingredients and start fresh
	 * @param bClearSaveFile - If true, deletes the existing save file (default: true)
	 */
	UFUNCTION(BlueprintCallable, Category = "Save/Load")
	void CreateNewGame(bool bClearSaveFile = true);

	/**
	 * Delete the save file from disk
	 * @param SlotName - The save slot name (defaults to "PlayerSave")
	 * @return True if the save file was successfully deleted
	 */
	UFUNCTION(BlueprintCallable, Category = "Save/Load")
	bool DeleteSaveGame(const FString& SlotName = TEXT("PlayerSave"));

	/**
	 * Check if a save file exists
	 * @param SlotName - The save slot name (defaults to "PlayerSave")
	 * @return True if save file exists
	 */
	UFUNCTION(BlueprintCallable, Category = "Save/Load")
	bool DoesSaveGameExist(const FString& SlotName = TEXT("PlayerSave")) const;

	// Dialogue State (stubbed for future use)
	/**
	 * Mark a dialogue as completed (stubbed for future implementation)
	 * @param DialogueName - The name of the dialogue asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue State")
	void MarkDialogueCompleted(const FName& DialogueName);

	/**
	 * Check if a dialogue has been completed (stubbed for future implementation)
	 * @param DialogueName - The name of the dialogue asset
	 * @return True if the dialogue has been completed
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue State")
	bool IsDialogueCompleted(const FName& DialogueName) const;

	// Dialogue Typewriter Settings (global, persisted to save)
	/** Whether dialogue text uses typewriter effect (character-by-character reveal) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue Settings")
	bool GetDialogueTypewriterEnabled() const { return bUseDialogueTypewriterEffect; }

	UFUNCTION(BlueprintCallable, Category = "Dialogue Settings")
	void SetDialogueTypewriterEnabled(bool bEnabled);

	/** Delay between characters in seconds (e.g. 0.02 = fast, 0.05 = medium) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue Settings")
	float GetDialogueTypewriterCharacterDelay() const { return DialogueTypewriterCharacterDelay; }

	UFUNCTION(BlueprintCallable, Category = "Dialogue Settings")
	void SetDialogueTypewriterSpeed(float CharacterDelaySeconds);

	/** If true, clicking "Next" while typing instantly completes the text */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue Settings")
	bool GetDialogueTypewriterSkipOnInput() const { return bDialogueTypewriterSkipOnInput; }

	UFUNCTION(BlueprintCallable, Category = "Dialogue Settings")
	void SetDialogueTypewriterSkipOnInput(bool bSkipOnInput);

	/** Sound to play for each character during typewriter effect. Leave empty for no sound. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue Settings")
	USoundBase* GetDialogueTypewriterSound() const { return DialogueTypewriterSound; }

	UFUNCTION(BlueprintCallable, Category = "Dialogue Settings")
	void SetDialogueTypewriterSound(USoundBase* Sound);

	/** Pitch variation range (0.1 = ±10%). Getter for typewriter pitch variation. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue Settings")
	float GetDialogueTypewriterPitchVariation() const { return DialogueTypewriterPitchVariation; }

	// Level Transition Lock System
	/**
	 * Unlock a level transition by its LockID.
	 * Can be called from anywhere (dialogue, Blueprint, C++).
	 * @param LockID - The LockID set on the APULevelTransition actor
	 * @return True if the transition was unlocked (or was already unlocked)
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	bool UnlockLevelTransition(const FName& LockID);

	/**
	 * Check if a level transition is unlocked
	 * @param LockID - The LockID set on the APULevelTransition actor
	 * @return True if the transition is unlocked (or LockID is NAME_None)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Level Transition")
	bool IsLevelTransitionUnlocked(const FName& LockID) const;

	/**
	 * Get all unlocked level transition IDs
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Level Transition")
	TSet<FName> GetUnlockedLevelTransitions() const { return UnlockedLevelTransitionIDs; }

	// Popup Manager System
	// Delegate for popup button callbacks (declared before use)
	DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPopupClosed, FName, ButtonID);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPopupClosedEvent, FName, ButtonID);

	/**
	 * Show a popup with the given data
	 * @param PopupData - The popup configuration data
	 * @param OnPopupClosed - Optional callback when popup is closed (returns button ID that was pressed)
	 * Note: Use the OnPopupClosedEvent multicast delegate for Blueprint callbacks instead
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup Manager", meta = (CallInEditor = "true"))
	void ShowPopup(const FPopupData& PopupData);
	
	/**
	 * Show a popup with the given data and callback.
	 * From Blueprint: Use the OnPopupClosed pin - create a Custom Event with a ButtonID (Name) parameter and connect it.
	 * The callback fires when the popup closes, with the ButtonID of the button that was pressed (e.g. "YES", "NO", "BACK", "NEXT").
	 * @param PopupData - The popup configuration data
	 * @param OnPopupClosed - Callback when popup is closed (returns button ID that was pressed)
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup Manager", meta = (DisplayName = "Show Popup With Callback"))
	void ShowPopupWithCallback(const FPopupData& PopupData, FOnPopupClosed OnPopupClosed);

	/**
	 * Show an ingredient unlock popup
	 * @param IngredientTag - The ingredient that was unlocked
	 * @param IngredientDisplayName - Optional display name (if not provided, will try to get from data table)
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup Manager")
	void ShowIngredientUnlockPopup(const FGameplayTag& IngredientTag, const FText& IngredientDisplayName = FText::GetEmpty());

	/**
	 * Show an ingredient unlock popup for multiple ingredients
	 * @param IngredientTags - Array of ingredients that were unlocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup Manager")
	void ShowIngredientUnlockPopupMultiple(const TArray<FGameplayTag>& IngredientTags);

	/**
	 * Close the current popup (if one is showing)
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup Manager")
	void CloseCurrentPopup();

	/**
	 * Called by the popup widget when it closes (internal use)
	 * @param ButtonID - The ID of the button that was pressed (or NAME_None if closed via other means)
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup Manager")
	void NotifyPopupClosed(FName ButtonID);

	/**
	 * Check if a popup is currently showing
	 * @return True if a popup is active
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup Manager")
	bool IsPopupShowing() const { return CurrentPopupWidget != nullptr; }
	
	/** Broadcast when any popup closes. Bind to this (e.g. from Event Construct) to react to popup button presses. Passes the ButtonID (e.g. "BACK", "NEXT"). */
	UPROPERTY(BlueprintAssignable, Category = "Popup Manager|Events", meta = (DisplayName = "On Popup Closed"))
	FOnPopupClosedEvent OnPopupClosedEvent;

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

	// Ingredient Inventory
	UPROPERTY(BlueprintReadOnly, Category = "Ingredient Inventory")
	TSet<FGameplayTag> UnlockedIngredientTags;

	// Starting ingredients that are unlocked when creating a new game
	// Can be configured in Blueprint or via code
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Inventory")
	TSet<FGameplayTag> StartingIngredientTags;

	// Recipe Journal - unlocked dishes
	UPROPERTY(BlueprintReadOnly, Category = "Recipe Journal")
	TSet<FGameplayTag> UnlockedDishTags;

	// Dish the player is currently working on (during customization). Shown first when opening journal.
	UPROPERTY(BlueprintReadWrite, Category = "Recipe Journal")
	FGameplayTag CurrentDishTag;

	// Starting dishes unlocked when creating a new game (e.g. your two initial recipes)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe Journal")
	TSet<FGameplayTag> StartingDishTags;

	// Data tables for journal dish lookup (set in Game Instance Blueprint - same as cooking station)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe Journal")
	TObjectPtr<class UDataTable> DishDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe Journal")
	TObjectPtr<class UDataTable> IngredientDataTable;

	// Dialogue State (stubbed for future use)
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue State")
	TSet<FName> CompletedDialogueNames;

	// Dialogue Typewriter Settings (global, persisted to save)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dialogue Settings")
	bool bUseDialogueTypewriterEffect = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dialogue Settings")
	float DialogueTypewriterCharacterDelay = 0.02f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dialogue Settings")
	bool bDialogueTypewriterSkipOnInput = true;

	/** Sound to play for each character during typewriter effect. Set in Game Instance Blueprint. Defaults to none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Settings")
	TObjectPtr<USoundBase> DialogueTypewriterSound = nullptr;

	/** Pitch variation range (e.g. 0.1 = ±10%). Pitch randomly varies between (1 - Value) and (1 + Value) per character. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dialogue Settings", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float DialogueTypewriterPitchVariation = 0.1f;

	// Level Transition Lock System - IDs that have been unlocked (persisted to save)
	UPROPERTY(BlueprintReadOnly, Category = "Level Transition")
	TSet<FName> UnlockedLevelTransitionIDs;

	// Save Game Reference
	UPROPERTY()
	UPUPlayerSaveGame* PlayerSaveGame;

	// Debug/Development Settings
	// If true, CreateNewGame() will delete existing save files (useful for testing)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save/Load|Debug")
	bool bClearSaveOnNewGame = true;

	// If true, always start with a new game (ignores existing save files) - useful for debugging
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save/Load|Debug")
	bool bAlwaysStartNewGame = false;

	// Popup Manager
	// Set this in DefaultEngine.ini under [/Script/ProjectUmeowmi.PUProjectUmeowmiGameInstance] as:
	// PopupWidgetClass=/Game/Path/To/Your/WBP_Popup.WBP_Popup_C
	// Or set it in your GameInstance Blueprint
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Popup Manager")
	TSoftClassPtr<class UPUPopupWidget> PopupWidgetClass;

	UPROPERTY()
	class UPUPopupWidget* CurrentPopupWidget;

	UPROPERTY()
	TArray<FPopupData> PopupQueue;

	FOnPopupClosed CurrentPopupCallback;

	// Internal popup management
	void ProcessPopupQueue();
	void OnPopupWidgetClosed(FName ButtonID);

private:
	// Stored level path for delayed loading after fade
	FString PendingLevelPath;

	// Delegate handle for PostLoadMapWithWorld
	FDelegateHandle PostLoadMapDelegateHandle;

	/**
	 * Called after fade out completes to actually load the level
	 */
	void LoadLevelAfterFade();
	
	/**
	 * Handle PostLoadMapWithWorld delegate - called when a level finishes loading
	 * This is more reliable than GameMode::StartPlay() in packaged builds
	 */
	void HandlePostLoadMap(UWorld* LoadedWorld);

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

