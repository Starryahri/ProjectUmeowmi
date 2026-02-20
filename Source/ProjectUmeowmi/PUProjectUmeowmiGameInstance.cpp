#include "PUProjectUmeowmiGameInstance.h"
#include "ProjectUmeowmiCharacter.h"
#include "LevelTransition/PULevelSpawnPoint.h"
#include "PUPlayerSaveGame.h"
#include "DishCustomization/PUIngredientBase.h"
#include "DishCustomization/PUDishBlueprintLibrary.h"
#include "UI/PUPopupWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "UObject/StructOnScope.h"
#include "Components/Button.h"
#include "UObject/UObjectGlobals.h"

UPUProjectUmeowmiGameInstance::UPUProjectUmeowmiGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHasSavedOrder = false;
	bTransitionInProgress = false;
	PlayerSaveGame = nullptr;
	CurrentPopupWidget = nullptr;
}

void UPUProjectUmeowmiGameInstance::Init()
{
	Super::Init();
	
	// Bind to PostLoadMapWithWorld delegate to detect when levels finish loading
	// This is more reliable than relying on GameMode::StartPlay() in packaged builds
	PostLoadMapDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPUProjectUmeowmiGameInstance::HandlePostLoadMap);
	
	// If debug flag is set, always start with a new game (ignores existing saves)
	if (bAlwaysStartNewGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUProjectUmeowmiGameInstance::Init - bAlwaysStartNewGame is enabled, creating new game (ignoring existing save)"));
		CreateNewGame();
		return;
	}
	
	// Try to load existing save game, or create new one if it doesn't exist
	if (!LoadGame())
	{
		CreateNewGame();
	}
}

void UPUProjectUmeowmiGameInstance::Shutdown()
{
	// Unbind the delegate to prevent memory leaks
	if (PostLoadMapDelegateHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapDelegateHandle);
		PostLoadMapDelegateHandle.Reset();
	}

	Super::Shutdown();
}

void UPUProjectUmeowmiGameInstance::TransitionToLevel(const FString& TargetLevelName, const FName& SpawnPointTag, bool bUseFade)
{
	if (bTransitionInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("Level transition already in progress, ignoring request"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Starting level transition to: %s (Spawn Point: %s)"), *TargetLevelName, *SpawnPointTag.ToString());

	// Save current player state
	SavePlayerState();

	// Store transition data
	PendingSpawnPointTag = SpawnPointTag;
	bTransitionInProgress = true;

	// Call Blueprint event
	OnTransitionStarted(TargetLevelName);

	// Get the world and player controller
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get world for level transition"));
		bTransitionInProgress = false;
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get player controller for level transition"));
		bTransitionInProgress = false;
		return;
	}

	// Build the level path - OpenLevel can accept either:
	// 1. Just the level name (e.g., "L_Chapter0_2_LolaRoom") - preferred for packaged builds
	// 2. Full path without extension (e.g., "/Game/LuckyFatCatDiner/Maps/L_Chapter0_2_LolaRoom")
	// 
	// In packaged builds, using just the level name is more reliable as the engine
	// will find the level in the cooked content automatically.
	FString LevelPath = TargetLevelName;
	
	// If it's already a full path, try to extract just the level name
	if (LevelPath.StartsWith(TEXT("/Game/")))
	{
		// Extract just the filename from the path
		FString Path, Filename, Extension;
		FPaths::Split(LevelPath, Path, Filename, Extension);
		LevelPath = Filename;
		UE_LOG(LogTemp, Log, TEXT("TransitionToLevel: Extracted level name '%s' from path '%s'"), *LevelPath, *TargetLevelName);
	}
	else
	{
		// Remove any .umap extension if present
		LevelPath.ReplaceInline(TEXT(".umap"), TEXT(""));
		UE_LOG(LogTemp, Log, TEXT("TransitionToLevel: Using level name '%s'"), *LevelPath);
	}

	// Store the level path for loading after fade completes
	PendingLevelPath = LevelPath;

	// Use fade if requested
	if (bUseFade)
	{
		// Fade out: FadeAlpha (X=start, Y=end), so 0 to 1 means transparent to opaque (black)
		// FadeTime is the duration in seconds - using 1 second
		PlayerController->ClientSetCameraFade(true, FColor::Black, FVector2D(0.0f, 1.0f), 1.0f, true, true);
		
		// Wait for fade to complete before loading the level
		FTimerHandle FadeTimerHandle;
		World->GetTimerManager().SetTimer(FadeTimerHandle, this, &UPUProjectUmeowmiGameInstance::LoadLevelAfterFade, 1.0f, false);
	}
	else
	{
		// No fade, load immediately
		UE_LOG(LogTemp, Log, TEXT("Loading level immediately (no fade): %s"), *LevelPath);
		UGameplayStatics::OpenLevel(World, FName(*LevelPath));
		// Note: OpenLevel is async, success/failure will be apparent when HandlePostLoadMap is called
	}
}

void UPUProjectUmeowmiGameInstance::OnLevelLoaded()
{
	if (!bTransitionInProgress)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Level loaded, positioning player at spawn point: %s"), *PendingSpawnPointTag.ToString());

	// Position player at spawn point
	PositionPlayerAtSpawnPoint(PendingSpawnPointTag);

	// Restore player state
	RestorePlayerState();

	// Fade in: FadeAlpha (X=start, Y=end), so 1 to 0 means opaque (black) to transparent
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->ClientSetCameraFade(true, FColor::Black, FVector2D(1.0f, 0.0f), 1.5f, true, false);
	}

	// Clear transition state
	bTransitionInProgress = false;
	PendingSpawnPointTag = NAME_None;

	// Call Blueprint event
	OnTransitionCompleted();
}

void UPUProjectUmeowmiGameInstance::SavePlayerState()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(PlayerController->GetPawn());
	if (Character && Character->HasCurrentOrder())
	{
		SavedPlayerOrder = Character->GetCurrentOrder();
		bHasSavedOrder = true;
		UE_LOG(LogTemp, Log, TEXT("Saved player order: %s"), *SavedPlayerOrder.OrderID.ToString());
	}
	else
	{
		bHasSavedOrder = false;
		UE_LOG(LogTemp, Log, TEXT("No player order to save"));
	}
}

void UPUProjectUmeowmiGameInstance::RestorePlayerState()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(PlayerController->GetPawn());
	if (Character && bHasSavedOrder)
	{
		Character->SetCurrentOrder(SavedPlayerOrder);
		UE_LOG(LogTemp, Log, TEXT("Restored player order: %s"), *SavedPlayerOrder.OrderID.ToString());
	}
}

void UPUProjectUmeowmiGameInstance::PositionPlayerAtSpawnPoint(const FName& SpawnPointTag)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get world for spawn point positioning"));
		return;
	}

	// Find all spawn points in the level
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, APULevelSpawnPoint::StaticClass(), FoundActors);

	APULevelSpawnPoint* TargetSpawnPoint = nullptr;

	// Look for spawn point with matching tag
	for (AActor* Actor : FoundActors)
	{
		APULevelSpawnPoint* SpawnPoint = Cast<APULevelSpawnPoint>(Actor);
		if (SpawnPoint && SpawnPoint->GetSpawnPointTag() == SpawnPointTag)
		{
			TargetSpawnPoint = SpawnPoint;
			break;
		}
	}

	// If no matching tag found, use the first spawn point (or default player start)
	if (!TargetSpawnPoint && FoundActors.Num() > 0)
	{
		TargetSpawnPoint = Cast<APULevelSpawnPoint>(FoundActors[0]);
		UE_LOG(LogTemp, Warning, TEXT("Spawn point with tag '%s' not found, using first available spawn point"), *SpawnPointTag.ToString());
	}

	if (TargetSpawnPoint)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController && PlayerController->GetPawn())
		{
			FVector SpawnLocation = TargetSpawnPoint->GetActorLocation();
			FRotator SpawnRotation = TargetSpawnPoint->GetActorRotation();

			PlayerController->GetPawn()->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
			UE_LOG(LogTemp, Log, TEXT("Positioned player at spawn point: %s (Location: %s)"), 
				*TargetSpawnPoint->GetSpawnPointTag().ToString(), *SpawnLocation.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No spawn point found in level, player will use default PlayerStart"));
	}
}

void UPUProjectUmeowmiGameInstance::ClearSavedPlayerOrder()
{
	SavedPlayerOrder = FPUOrderBase();
	bHasSavedOrder = false;
	UE_LOG(LogTemp, Log, TEXT("Cleared saved player order"));
}

void UPUProjectUmeowmiGameInstance::LoadLevelAfterFade()
{
	UWorld* World = GetWorld();
	if (!World || PendingLevelPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot load level: World is null or no pending level path"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Fade out complete, loading level: %s"), *PendingLevelPath);
	
	// Load the level after fade has completed
	// Note: HandlePostLoadMap will be called via delegate when the level finishes loading
	// OpenLevel is async, so we can't check success here - HandlePostLoadMap will be called on success
	UGameplayStatics::OpenLevel(World, FName(*PendingLevelPath));
}

// Ingredient Inventory System
bool UPUProjectUmeowmiGameInstance::UnlockIngredient(const FGameplayTag& IngredientTag)
{
	if (!IngredientTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUProjectUmeowmiGameInstance::UnlockIngredient - Invalid ingredient tag provided"));
		return false;
	}

	if (UnlockedIngredientTags.Contains(IngredientTag))
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::UnlockIngredient - Ingredient %s is already unlocked"), *IngredientTag.ToString());
		return true; // Already unlocked, consider it successful
	}

	UnlockedIngredientTags.Add(IngredientTag);
	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::UnlockIngredient - Unlocked ingredient: %s"), *IngredientTag.ToString());

	// Show unlock popup
	ShowIngredientUnlockPopup(IngredientTag);

	// Auto-save when an ingredient is unlocked
	SaveGame();

	return true;
}

int32 UPUProjectUmeowmiGameInstance::UnlockIngredients(const TArray<FGameplayTag>& IngredientTags)
{
	int32 UnlockedCount = 0;
	int32 NewlyUnlockedCount = 0;

	for (const FGameplayTag& Tag : IngredientTags)
	{
		if (!Tag.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("UPUProjectUmeowmiGameInstance::UnlockIngredients - Skipping invalid ingredient tag"));
			continue;
		}

		if (UnlockedIngredientTags.Contains(Tag))
		{
			UnlockedCount++; // Count as successful (already unlocked)
		}
		else
		{
			UnlockedIngredientTags.Add(Tag);
			UnlockedCount++;
			NewlyUnlockedCount++;
			UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::UnlockIngredients - Unlocked ingredient: %s"), *Tag.ToString());
		}
	}

	if (NewlyUnlockedCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::UnlockIngredients - Unlocked %d new ingredients (total: %d)"), 
			NewlyUnlockedCount, UnlockedCount);
		
		// Collect newly unlocked ingredient tags
		TArray<FGameplayTag> NewlyUnlockedTags;
		for (const FGameplayTag& Tag : IngredientTags)
		{
			if (Tag.IsValid() && UnlockedIngredientTags.Contains(Tag))
			{
				NewlyUnlockedTags.Add(Tag);
			}
		}

		// Show unlock popup for newly unlocked ingredients
		if (NewlyUnlockedTags.Num() == 1)
		{
			ShowIngredientUnlockPopup(NewlyUnlockedTags[0]);
		}
		else if (NewlyUnlockedTags.Num() > 1)
		{
			ShowIngredientUnlockPopupMultiple(NewlyUnlockedTags);
		}
		
		// Auto-save when ingredients are unlocked
		SaveGame();
	}
	else if (UnlockedCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::UnlockIngredients - All %d ingredients were already unlocked"), UnlockedCount);
	}

	return UnlockedCount;
}

bool UPUProjectUmeowmiGameInstance::IsIngredientUnlocked(const FGameplayTag& IngredientTag) const
{
	if (!IngredientTag.IsValid())
	{
		return false;
	}

	return UnlockedIngredientTags.Contains(IngredientTag);
}

// Save/Load System
bool UPUProjectUmeowmiGameInstance::SaveGame(const FString& SlotName)
{
	if (!PlayerSaveGame)
	{
		// Create a new save game object if we don't have one
		PlayerSaveGame = Cast<UPUPlayerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPUPlayerSaveGame::StaticClass()));
		if (!PlayerSaveGame)
		{
			UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::SaveGame - Failed to create save game object"));
			return false;
		}
	}

	// Copy current state to save game
	PlayerSaveGame->UnlockedIngredientTags = UnlockedIngredientTags;
	PlayerSaveGame->CompletedDialogueNames = CompletedDialogueNames;

	// Save to disk
	if (UGameplayStatics::SaveGameToSlot(PlayerSaveGame, SlotName, 0))
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::SaveGame - Successfully saved game to slot: %s"), *SlotName);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::SaveGame - Failed to save game to slot: %s"), *SlotName);
		return false;
	}
}

bool UPUProjectUmeowmiGameInstance::LoadGame(const FString& SlotName)
{
	if (!DoesSaveGameExist(SlotName))
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::LoadGame - No save game found in slot: %s"), *SlotName);
		return false;
	}

	USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SlotName, 0);
	if (!LoadedGame)
	{
		UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::LoadGame - Failed to load game from slot: %s"), *SlotName);
		return false;
	}

	PlayerSaveGame = Cast<UPUPlayerSaveGame>(LoadedGame);
	if (!PlayerSaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::LoadGame - Loaded save game is not of type UPUPlayerSaveGame"));
		return false;
	}

	// Restore state from save game
	UnlockedIngredientTags = PlayerSaveGame->UnlockedIngredientTags;
	CompletedDialogueNames = PlayerSaveGame->CompletedDialogueNames;

	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::LoadGame - Successfully loaded game from slot: %s (Unlocked ingredients: %d)"), 
		*SlotName, UnlockedIngredientTags.Num());

	return true;
}

void UPUProjectUmeowmiGameInstance::CreateNewGame(bool bClearSaveFile)
{
	// Use the exposed property value (can be toggled in editor for debugging)
	// If parameter is explicitly false, don't clear. Otherwise use the property value
	// Note: When called with default parameter (true), it uses bClearSaveOnNewGame property
	bool bShouldClear = bClearSaveFile ? bClearSaveOnNewGame : false;
	
	// Delete existing save file if requested
	if (bShouldClear)
	{
		DeleteSaveGame();
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::CreateNewGame - Cleared save file (bClearSaveOnNewGame: %d)"), bClearSaveOnNewGame);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::CreateNewGame - Keeping existing save file"));
	}

	// Clear all unlocked ingredients and dialogue states FIRST
	UnlockedIngredientTags.Empty();
	CompletedDialogueNames.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::CreateNewGame - Cleared all unlocked ingredients (was %d, now %d)"), 
		UnlockedIngredientTags.Num(), 0);

	// Unlock starting ingredients
	UnlockedIngredientTags = StartingIngredientTags;
	
	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::CreateNewGame - Created new game with %d starting ingredients"), 
		StartingIngredientTags.Num());

	// Create a new save game object
	PlayerSaveGame = Cast<UPUPlayerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPUPlayerSaveGame::StaticClass()));
	if (PlayerSaveGame)
	{
		// Initialize with starting ingredients
		PlayerSaveGame->UnlockedIngredientTags = UnlockedIngredientTags;
		PlayerSaveGame->CompletedDialogueNames.Empty();
		PlayerSaveGame->SaveVersion = 1;

		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::CreateNewGame - Save game object created"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::CreateNewGame - Failed to create save game object"));
	}
}

bool UPUProjectUmeowmiGameInstance::DoesSaveGameExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

bool UPUProjectUmeowmiGameInstance::DeleteSaveGame(const FString& SlotName)
{
	if (!DoesSaveGameExist(SlotName))
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::DeleteSaveGame - No save file exists in slot: %s"), *SlotName);
		return true; // Consider it successful if it doesn't exist
	}

	// Delete the save file
	if (UGameplayStatics::DeleteGameInSlot(SlotName, 0))
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::DeleteSaveGame - Successfully deleted save file: %s"), *SlotName);
		
		// Clear the in-memory save game reference
		PlayerSaveGame = nullptr;
		
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::DeleteSaveGame - Failed to delete save file: %s"), *SlotName);
		return false;
	}
}

// Dialogue State (stubbed for future use)
void UPUProjectUmeowmiGameInstance::MarkDialogueCompleted(const FName& DialogueName)
{
	if (DialogueName == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUProjectUmeowmiGameInstance::MarkDialogueCompleted - Invalid dialogue name provided"));
		return;
	}

	if (CompletedDialogueNames.Contains(DialogueName))
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::MarkDialogueCompleted - Dialogue %s is already marked as completed"), *DialogueName.ToString());
		return;
	}

	CompletedDialogueNames.Add(DialogueName);
	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::MarkDialogueCompleted - Marked dialogue as completed: %s"), *DialogueName.ToString());

	// Auto-save when a dialogue is completed
	SaveGame();
}

bool UPUProjectUmeowmiGameInstance::IsDialogueCompleted(const FName& DialogueName) const
{
	if (DialogueName == NAME_None)
	{
		return false;
	}

	return CompletedDialogueNames.Contains(DialogueName);
}

// Popup Manager System
void UPUProjectUmeowmiGameInstance::ShowPopup(const FPopupData& PopupData)
{
	ShowPopupWithCallback(PopupData, FOnPopupClosed());
}

void UPUProjectUmeowmiGameInstance::ShowPopupWithCallback(const FPopupData& PopupData, FOnPopupClosed OnPopupClosed)
{
	// If a popup is already showing, queue this one
	if (CurrentPopupWidget != nullptr)
	{
		PopupQueue.Add(PopupData);
		// Store callback if provided
		if (OnPopupClosed.IsBound())
		{
			// Note: We can only store one callback per popup in the queue
			// For multiple callbacks, you'd need a more complex system
			UE_LOG(LogTemp, Warning, TEXT("UPUProjectUmeowmiGameInstance::ShowPopup - Popup queued, but callback may not work correctly with queued popups"));
		}
		return;
	}

	// Check if we have a popup widget class set
	TSubclassOf<UPUPopupWidget> PopupWidgetClassLoaded = PopupWidgetClass.LoadSynchronous();
	if (!PopupWidgetClassLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::ShowPopup - PopupWidgetClass is not set! Please set it in DefaultEngine.ini or create a GameInstance Blueprint."));
		return;
	}

	// Create the popup widget
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::ShowPopup - Cannot get world"));
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::ShowPopup - Cannot get player controller"));
		return;
	}

	UPUPopupWidget* PopupWidget = CreateWidget<UPUPopupWidget>(PlayerController, PopupWidgetClassLoaded);
	if (!PopupWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("UPUProjectUmeowmiGameInstance::ShowPopup - Failed to create popup widget"));
		return;
	}

	CurrentPopupWidget = PopupWidget;

	// Store the callback
	CurrentPopupCallback = OnPopupClosed;

	// Handle modal behavior - block input if modal
	if (PopupData.bModal && PlayerController)
	{
		// Block player movement and look input
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
		
		// Set input mode to UI only (mouse visible, can interact with UI)
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
		
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::ShowPopup - Modal popup: Input blocked"));
	}

	// Add to viewport
	PopupWidget->AddToViewport(1000); // High z-order to appear on top

	// Set popup data directly (now we have a proper C++ class!)
	PopupWidget->SetPopupData(PopupData);

	// Broadcast event
	OnPopupClosedEvent.Broadcast(NAME_None);

	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::ShowPopup - Showing popup: %s (Modal: %d)"), *PopupData.Title.ToString(), PopupData.bModal);
}

void UPUProjectUmeowmiGameInstance::ShowIngredientUnlockPopup(const FGameplayTag& IngredientTag, const FText& IngredientDisplayName)
{
	if (!IngredientTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUProjectUmeowmiGameInstance::ShowIngredientUnlockPopup - Invalid ingredient tag"));
		return;
	}

	FPopupData PopupData;
	PopupData.PopupType = EPopupType::Notification;
	PopupData.Title = FText::FromString(TEXT("New Ingredient Unlocked!"));
	
	// Get display name
	FText DisplayName = IngredientDisplayName;
	if (DisplayName.IsEmpty())
	{
		// Try to get from ingredient tag (remove "Ingredient." prefix)
		FString TagString = IngredientTag.ToString();
		if (TagString.StartsWith(TEXT("Ingredient.")))
		{
			FString IngredientName = TagString.RightChop(11); // Remove "Ingredient." prefix
			// Capitalize first letter
			if (IngredientName.Len() > 0)
			{
				IngredientName[0] = FChar::ToUpper(IngredientName[0]);
			}
			DisplayName = FText::FromString(IngredientName);
		}
		else
		{
			DisplayName = FText::FromString(TagString);
		}
	}

	PopupData.Message = FText::Format(FText::FromString(TEXT("You unlocked: {0}")), DisplayName);
	PopupData.bModal = false;
	PopupData.bAutoDismiss = true;
	PopupData.AutoDismissTime = 3.0f;
	PopupData.bShowCloseButton = true;
	
	// Add ingredient tag to additional data
	PopupData.AdditionalData.Add(IngredientTag);

	// Default OK button
	FPopupButtonData OKButton;
	OKButton.ButtonID = FName(TEXT("OK"));
	OKButton.ButtonLabel = FText::FromString(TEXT("OK"));
	OKButton.bIsPrimary = true;
	PopupData.Buttons.Add(OKButton);

	ShowPopup(PopupData);
}

void UPUProjectUmeowmiGameInstance::ShowIngredientUnlockPopupMultiple(const TArray<FGameplayTag>& IngredientTags)
{
	if (IngredientTags.Num() == 0)
	{
		return;
	}

	FPopupData PopupData;
	PopupData.PopupType = EPopupType::Notification;
	
	if (IngredientTags.Num() == 1)
	{
		// Single ingredient - use the single ingredient popup
		ShowIngredientUnlockPopup(IngredientTags[0]);
		return;
	}

	// Multiple ingredients
	PopupData.Title = FText::FromString(TEXT("New Ingredients Unlocked!"));
	
	FString MessageString = FString::Printf(TEXT("You unlocked %d new ingredients:"), IngredientTags.Num());
	for (int32 i = 0; i < IngredientTags.Num() && i < 5; ++i) // Limit to 5 for display
	{
		FString TagString = IngredientTags[i].ToString();
		if (TagString.StartsWith(TEXT("Ingredient.")))
		{
			FString IngredientName = TagString.RightChop(11);
			if (IngredientName.Len() > 0)
			{
				IngredientName[0] = FChar::ToUpper(IngredientName[0]);
			}
			MessageString += FString::Printf(TEXT("\n• %s"), *IngredientName);
		}
	}
	
	if (IngredientTags.Num() > 5)
	{
		MessageString += FString::Printf(TEXT("\n... and %d more"), IngredientTags.Num() - 5);
	}

	PopupData.Message = FText::FromString(MessageString);
	PopupData.bModal = false;
	PopupData.bAutoDismiss = true;
	PopupData.AutoDismissTime = 4.0f; // Slightly longer for multiple ingredients
	PopupData.bShowCloseButton = true;
	
	// Add all ingredient tags to additional data
	PopupData.AdditionalData = IngredientTags;

	// Default OK button
	FPopupButtonData OKButton;
	OKButton.ButtonID = FName(TEXT("OK"));
	OKButton.ButtonLabel = FText::FromString(TEXT("OK"));
	OKButton.bIsPrimary = true;
	PopupData.Buttons.Add(OKButton);

	ShowPopup(PopupData);
}

void UPUProjectUmeowmiGameInstance::CloseCurrentPopup()
{
	if (CurrentPopupWidget)
	{
		// Call Close directly (now we have a proper C++ class!)
		CurrentPopupWidget->Close(NAME_None);
	}
}

void UPUProjectUmeowmiGameInstance::NotifyPopupClosed(FName ButtonID)
{
	OnPopupWidgetClosed(ButtonID);
}

void UPUProjectUmeowmiGameInstance::OnPopupWidgetClosed(FName ButtonID)
{
	// Restore input if it was blocked (for modal popups)
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			// Restore input (safe to call even if not blocked)
			PlayerController->SetIgnoreMoveInput(false);
			PlayerController->SetIgnoreLookInput(false);
			
			// Restore game input mode
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
			// Note: Don't force mouse cursor off - let the game decide
			
			UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::OnPopupWidgetClosed - Input restored"));
		}
	}

	// Call the stored callback if bound
	if (CurrentPopupCallback.IsBound())
	{
		CurrentPopupCallback.Execute(ButtonID);
		CurrentPopupCallback.Unbind();
	}

	// Broadcast event
	OnPopupClosedEvent.Broadcast(ButtonID);

	// Clear current popup
	CurrentPopupWidget = nullptr;

	// Process queue
	ProcessPopupQueue();
}

void UPUProjectUmeowmiGameInstance::ProcessPopupQueue()
{
	if (PopupQueue.Num() > 0 && CurrentPopupWidget == nullptr)
	{
		FPopupData NextPopup = PopupQueue[0];
		PopupQueue.RemoveAt(0);
		ShowPopup(NextPopup);
	}
}

void UPUProjectUmeowmiGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// Only process if we have a transition in progress
	if (!bTransitionInProgress)
	{
		return;
	}

	// Verify the loaded world is valid
	if (!LoadedWorld)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("HandlePostLoadMap: Level loaded (World: %s), scheduling OnLevelLoaded()"), 
		LoadedWorld ? *LoadedWorld->GetName() : TEXT("NULL"));

	// Use a timer to delay OnLevelLoaded() slightly to ensure player controller and pawn are fully initialized
	// This is especially important in packaged builds where timing can differ from the editor
	// Use the loaded world's timer manager to ensure we're using the correct world
	if (LoadedWorld)
	{
		FTimerHandle LevelLoadedTimerHandle;
		LoadedWorld->GetTimerManager().SetTimer(LevelLoadedTimerHandle, this, &UPUProjectUmeowmiGameInstance::OnLevelLoaded, 0.1f, false);
	}
}

