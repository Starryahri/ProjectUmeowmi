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

	// Ingredient Inventory System
	/**
	 * Unlock an ingredient (adds it to the unlocked set)
	 * @param IngredientTag - The gameplay tag of the ingredient to unlock
	 * @return True if the ingredient was successfully unlocked (or was already unlocked)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ingredient Inventory")
	bool UnlockIngredient(const FGameplayTag& IngredientTag);

	/**
	 * Unlock multiple ingredients at once
	 * @param IngredientTags - Array of gameplay tags to unlock
	 * @return Number of ingredients successfully unlocked (including ones that were already unlocked)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ingredient Inventory")
	int32 UnlockIngredients(const TArray<FGameplayTag>& IngredientTags);

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

	// Dialogue State (stubbed for future use)
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue State")
	TSet<FName> CompletedDialogueNames;

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

