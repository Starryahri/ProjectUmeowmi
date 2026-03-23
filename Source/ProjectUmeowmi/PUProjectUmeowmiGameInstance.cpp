#include "PUProjectUmeowmiGameInstance.h"
#include "ProjectUmeowmiCharacter.h"
#include "UI/PUDialogueBox.h"
#include "Dialogue/TalkingObject.h"
#include "LevelTransition/PULevelSpawnPoint.h"
#include "PUPlayerSaveGame.h"
#include "DishCustomization/PUIngredientBase.h"
#include "DishCustomization/PUDishBlueprintLibrary.h"
#include "UI/PUPopupWidget.h"
#include "DishCustomization/PUDishCustomizationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
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
#include "Sound/SoundBase.h"

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

	if (UPUQuestSubsystem* Q = GetSubsystem<UPUQuestSubsystem>())
	{
		Q->SetCachedQuestObjectiveContentTable(QuestObjectiveContentTable);
	}
	
	// Bind to PostLoadMapWithWorld delegate to detect when levels finish loading
	// This is more reliable than relying on GameMode::StartPlay() in packaged builds
	PostLoadMapDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPUProjectUmeowmiGameInstance::HandlePostLoadMap);
	
	// If debug flag is set, always start with a new game (ignores existing saves)
	if (bAlwaysStartNewGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUProjectUmeowmiGameInstance::Init - bAlwaysStartNewGame is enabled, creating new game (ignoring existing save)"));
		CreateNewGame();
		if (UPUQuestSubsystem* Q = GetSubsystem<UPUQuestSubsystem>())
		{
			Q->EnsureQuestRootTagDefault();
			Q->BroadcastQuestObjectiveChanged();
		}
		return;
	}
	
	// Try to load existing save game, or create new one if it doesn't exist
	if (!LoadGame())
	{
		CreateNewGame();
	}

	if (UPUQuestSubsystem* Q = GetSubsystem<UPUQuestSubsystem>())
	{
		Q->EnsureQuestRootTagDefault();
		Q->BroadcastQuestObjectiveChanged();
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

	// Hide all interaction UI elements before fade/transition
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ATalkingObject::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		if (ATalkingObject* TalkingObject = Cast<ATalkingObject>(Actor))
		{
			TalkingObject->HideInteractionWidgetForTransition();
		}
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

			// Apply camera position index from spawn point so the isometric camera faces the correct angle
			if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(PlayerController->GetPawn()))
			{
				int32 CameraIndex = TargetSpawnPoint->GetCameraPositionIndex();
				int32 NumPositions = FMath::Max(1, Character->GetNumberOfCameraPositions());
				CameraIndex = FMath::Clamp(CameraIndex, 0, NumPositions - 1);
				Character->SetCameraPositionIndex(CameraIndex);
				Character->InitializeCameraPositionFromBlueprint();
				UE_LOG(LogTemp, Log, TEXT("Set camera position index to %d for spawn point: %s"), CameraIndex, *TargetSpawnPoint->GetSpawnPointTag().ToString());
			}
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
bool UPUProjectUmeowmiGameInstance::UnlockIngredient(const FGameplayTag& IngredientTag, bool bSilent)
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

	if (!bSilent)
	{
		ShowIngredientUnlockPopup(IngredientTag);
	}

	// Auto-save when an ingredient is unlocked
	SaveGame();

	return true;
}

int32 UPUProjectUmeowmiGameInstance::UnlockIngredients(const TArray<FGameplayTag>& IngredientTags, bool bSilent)
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

		if (!bSilent)
		{
			// Collect newly unlocked ingredient tags for popup
			TArray<FGameplayTag> NewlyUnlockedTags;
			for (const FGameplayTag& Tag : IngredientTags)
			{
				if (Tag.IsValid() && UnlockedIngredientTags.Contains(Tag))
				{
					NewlyUnlockedTags.Add(Tag);
				}
			}

			if (NewlyUnlockedTags.Num() == 1)
			{
				ShowIngredientUnlockPopup(NewlyUnlockedTags[0]);
			}
			else if (NewlyUnlockedTags.Num() > 1)
			{
				ShowIngredientUnlockPopupMultiple(NewlyUnlockedTags);
			}
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

// Recipe Journal System
bool UPUProjectUmeowmiGameInstance::UnlockDish(const FGameplayTag& DishTag)
{
	if (!DishTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUProjectUmeowmiGameInstance::UnlockDish - Invalid dish tag provided"));
		return false;
	}

	if (UnlockedDishTags.Contains(DishTag))
	{
		return true;
	}

	UnlockedDishTags.Add(DishTag);
	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::UnlockDish - Unlocked dish: %s"), *DishTag.ToString());
	SaveGame();
	return true;
}

int32 UPUProjectUmeowmiGameInstance::UnlockDishes(const TArray<FGameplayTag>& DishTags)
{
	int32 Count = 0;
	for (const FGameplayTag& Tag : DishTags)
	{
		if (Tag.IsValid() && UnlockDish(Tag))
		{
			Count++;
		}
	}
	return Count;
}

bool UPUProjectUmeowmiGameInstance::IsDishUnlocked(const FGameplayTag& DishTag) const
{
	if (!DishTag.IsValid()) return false;
	return UnlockedDishTags.Contains(DishTag);
}

void UPUProjectUmeowmiGameInstance::SetCurrentDishTag(const FGameplayTag& DishTag)
{
	CurrentDishTag = DishTag;
}

void UPUProjectUmeowmiGameInstance::ClearCurrentDishTag()
{
	CurrentDishTag = FGameplayTag();
}

TArray<FGameplayTag> UPUProjectUmeowmiGameInstance::GetOrderedUnlockedDishTags() const
{
	TArray<FGameplayTag> Ordered;
	Ordered.Reserve(UnlockedDishTags.Num());
	for (const FGameplayTag& Tag : UnlockedDishTags)
	{
		if (Tag.IsValid())
		{
			Ordered.Add(Tag);
		}
	}
	Ordered.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
	return Ordered;
}

FGameplayTag UPUProjectUmeowmiGameInstance::CycleJournalDish(int32 Direction)
{
	TArray<FGameplayTag> Ordered = GetOrderedUnlockedDishTags();
	if (Ordered.Num() == 0)
	{
		return FGameplayTag();
	}

	int32 CurrentIndex = 0;
	if (CurrentDishTag.IsValid())
	{
		const int32 Found = Ordered.Find(CurrentDishTag);
		if (Found != INDEX_NONE)
		{
			CurrentIndex = Found;
		}
	}

	int32 NewIndex = CurrentIndex + Direction;
	if (NewIndex < 0)
	{
		NewIndex = Ordered.Num() - 1;
	}
	else if (NewIndex >= Ordered.Num())
	{
		NewIndex = 0;
	}

	const FGameplayTag NewTag = Ordered[NewIndex];
	CurrentDishTag = NewTag;
	return NewTag;
}

bool UPUProjectUmeowmiGameInstance::GetDishDataForTag(const FGameplayTag& DishTag, FPUDishBase& OutDish) const
{
	if (!DishTag.IsValid() || !DishDataTable) return false;
	return UPUDishBlueprintLibrary::GetDishFromDataTable(DishDataTable, IngredientDataTable, DishTag, OutDish);
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
	PlayerSaveGame->UnlockedDishTags = UnlockedDishTags;
	PlayerSaveGame->CompletedDialogueNames = CompletedDialogueNames;
	PlayerSaveGame->UnlockedLevelTransitionIDs = UnlockedLevelTransitionIDs;
	PlayerSaveGame->bUseDialogueTypewriterEffect = bUseDialogueTypewriterEffect;
	PlayerSaveGame->DialogueTypewriterCharacterDelay = DialogueTypewriterCharacterDelay;
	PlayerSaveGame->bDialogueTypewriterSkipOnInput = bDialogueTypewriterSkipOnInput;
	PlayerSaveGame->DialogueSkipModeCharacterDelay = DialogueSkipModeCharacterDelay;
	PlayerSaveGame->bTutorialCompleted = bTutorialCompleted;
	PlayerSaveGame->TutorialStep = TutorialStep;

	if (UPUQuestSubsystem* Q = GetSubsystem<UPUQuestSubsystem>())
	{
		Q->ExportToSave(PlayerSaveGame);
	}
	PlayerSaveGame->SaveVersion = 2;

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
	UnlockedDishTags = PlayerSaveGame->UnlockedDishTags;
	CompletedDialogueNames = PlayerSaveGame->CompletedDialogueNames;
	UnlockedLevelTransitionIDs = PlayerSaveGame->UnlockedLevelTransitionIDs;
	bUseDialogueTypewriterEffect = PlayerSaveGame->bUseDialogueTypewriterEffect;
	DialogueTypewriterCharacterDelay = PlayerSaveGame->DialogueTypewriterCharacterDelay;
	bDialogueTypewriterSkipOnInput = PlayerSaveGame->bDialogueTypewriterSkipOnInput;
	DialogueSkipModeCharacterDelay = PlayerSaveGame->DialogueSkipModeCharacterDelay;
	bTutorialCompleted = PlayerSaveGame->bTutorialCompleted;
	TutorialStep = PlayerSaveGame->TutorialStep;

	if (UPUQuestSubsystem* Q = GetSubsystem<UPUQuestSubsystem>())
	{
		Q->ImportFromSave(PlayerSaveGame);
		if (Q->MigrateQuestSaveIfNeeded(PlayerSaveGame))
		{
			SaveGame();
		}
	}

	// Migration: old saves may not have UnlockedDishTags; initialize from StartingDishTags if empty
	if (UnlockedDishTags.Num() == 0 && StartingDishTags.Num() > 0)
	{
		UnlockedDishTags = StartingDishTags;
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::LoadGame - Migrated %d starting dishes to unlocked"), StartingDishTags.Num());
	}

	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::LoadGame - Successfully loaded game from slot: %s (Unlocked ingredients: %d, dishes: %d)"), 
		*SlotName, UnlockedIngredientTags.Num(), UnlockedDishTags.Num());

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

	// Clear all unlocked ingredients, dishes, dialogue states, level transitions, and tutorial state FIRST
	UnlockedIngredientTags.Empty();
	UnlockedDishTags.Empty();
	CurrentDishTag = FGameplayTag();
	CompletedDialogueNames.Empty();
	UnlockedLevelTransitionIDs.Empty();
	bTutorialCompleted = false;
	TutorialStep = 0;

	if (UPUQuestSubsystem* Q = GetSubsystem<UPUQuestSubsystem>())
	{
		Q->ResetQuestStateForNewGame();
	}
	
	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::CreateNewGame - Cleared all unlocked ingredients and dishes"));

	// Unlock starting ingredients and dishes
	UnlockedIngredientTags = StartingIngredientTags;
	UnlockedDishTags = StartingDishTags;
	
	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::CreateNewGame - Created new game with %d starting ingredients, %d starting dishes"), 
		StartingIngredientTags.Num(), StartingDishTags.Num());

	if (bAutoStartInitialQuestOnNewGame && InitialQuestTag.IsValid() && InitialObjectiveTag.IsValid())
	{
		if (UPUQuestSubsystem* Q = GetSubsystem<UPUQuestSubsystem>())
		{
			Q->StartQuest(InitialQuestTag, InitialObjectiveTag, false);
		}
	}

	// Create a new save game object
	PlayerSaveGame = Cast<UPUPlayerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPUPlayerSaveGame::StaticClass()));
	if (PlayerSaveGame)
	{
		// Initialize with starting ingredients and dishes
		PlayerSaveGame->UnlockedIngredientTags = UnlockedIngredientTags;
		PlayerSaveGame->UnlockedDishTags = UnlockedDishTags;
		PlayerSaveGame->CompletedDialogueNames.Empty();
		PlayerSaveGame->UnlockedLevelTransitionIDs.Empty();
		PlayerSaveGame->bUseDialogueTypewriterEffect = bUseDialogueTypewriterEffect;
		PlayerSaveGame->DialogueTypewriterCharacterDelay = DialogueTypewriterCharacterDelay;
		PlayerSaveGame->bDialogueTypewriterSkipOnInput = bDialogueTypewriterSkipOnInput;
		PlayerSaveGame->DialogueSkipModeCharacterDelay = DialogueSkipModeCharacterDelay;
		PlayerSaveGame->bTutorialCompleted = false;
		PlayerSaveGame->TutorialStep = 0;
		if (UPUQuestSubsystem* Q = GetSubsystem<UPUQuestSubsystem>())
		{
			Q->ExportToSave(PlayerSaveGame);
		}
		PlayerSaveGame->SaveVersion = 2;

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

// Tutorial System
void UPUProjectUmeowmiGameInstance::SetTutorialStep(int32 Step)
{
	TutorialStep = FMath::Clamp(Step, 0, 7);
}

void UPUProjectUmeowmiGameInstance::AdvanceTutorialStep()
{
	++TutorialStep;
	if (TutorialStep >= 7)
	{
		TutorialStep = 7;
		SetTutorialCompleted();
	}
	else
	{
		SaveGame();
	}
}

void UPUProjectUmeowmiGameInstance::SetTutorialCompleted()
{
	bTutorialCompleted = true;
	TutorialStep = 7;
	SaveGame();
	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::SetTutorialCompleted - Tutorial marked as completed"));
}

FGameplayTag UPUProjectUmeowmiGameInstance::GetTutorialAllowedIngredientTag() const
{
	switch (TutorialStep)
	{
		case 1: return TutorialStep1IngredientTag;
		case 2: return TutorialStep2IngredientTag;
		default: return FGameplayTag();
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

void UPUProjectUmeowmiGameInstance::SetDialogueTypewriterEnabled(bool bEnabled)
{
	bUseDialogueTypewriterEffect = bEnabled;
	SaveGame();
}

void UPUProjectUmeowmiGameInstance::SetDialogueTypewriterSpeed(float CharacterDelaySeconds)
{
	DialogueTypewriterCharacterDelay = FMath::Max(0.001f, CharacterDelaySeconds);
	SaveGame();
}

void UPUProjectUmeowmiGameInstance::SetDialogueTypewriterSkipOnInput(bool bSkipOnInput)
{
	bDialogueTypewriterSkipOnInput = bSkipOnInput;
	SaveGame();
}

void UPUProjectUmeowmiGameInstance::SetDialogueSkipModeSpeed(float CharacterDelaySeconds)
{
	DialogueSkipModeCharacterDelay = CharacterDelaySeconds;
	SaveGame();
}

void UPUProjectUmeowmiGameInstance::SetDialogueTypewriterSound(USoundBase* Sound)
{
	DialogueTypewriterSound = Sound;
}

// Level Transition Lock System
bool UPUProjectUmeowmiGameInstance::UnlockLevelTransition(const FName& LockID)
{
	if (LockID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUProjectUmeowmiGameInstance::UnlockLevelTransition - Invalid LockID (NAME_None)"));
		return false;
	}

	if (UnlockedLevelTransitionIDs.Contains(LockID))
	{
		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::UnlockLevelTransition - Level transition %s is already unlocked"), *LockID.ToString());
		return true;
	}

	UnlockedLevelTransitionIDs.Add(LockID);
	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::UnlockLevelTransition - Unlocked level transition: %s"), *LockID.ToString());

	SaveGame();
	return true;
}

bool UPUProjectUmeowmiGameInstance::IsLevelTransitionUnlocked(const FName& LockID) const
{
	// No LockID means always unlocked
	if (LockID == NAME_None)
	{
		return true;
	}

	return UnlockedLevelTransitionIDs.Contains(LockID);
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

	// Add to viewport first so widget hierarchy is built before we set focus
	PopupWidget->AddToViewport(1000); // High z-order to appear on top

	// Set popup data directly (now we have a proper C++ class!)
	PopupWidget->SetPopupData(PopupData);

	// Set input mode and focus for ALL popups - required for controller support
	if (PlayerController)
	{
		// Handle modal behavior - block movement/look if modal
		if (PopupData.bModal)
		{
			PlayerController->SetIgnoreMoveInput(true);
			PlayerController->SetIgnoreLookInput(true);
		}

		// UI-only input so controller can navigate to popup buttons
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		if (UWidget* FocusTarget = PopupWidget->GetPreferredFocusTarget())
		{
			if (TSharedPtr<SWidget> SlateWidget = FocusTarget->GetCachedWidget())
			{
				InputMode.SetWidgetToFocus(SlateWidget);
			}
		}
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;

		UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::ShowPopup - Popup shown with focus (Modal: %d)"), PopupData.bModal);
	}

	// Broadcast event
	OnPopupClosedEvent.Broadcast(NAME_None);

	UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::ShowPopup - Showing popup: %s"), *PopupData.Title.ToString());
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
	PopupData.bAutoDismiss = false;
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
	PopupData.bAutoDismiss = false;
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

void UPUProjectUmeowmiGameInstance::NotifyDialogueOpened()
{
	bDialogueOpen = true;
	OnDialogueOpenedEvent.Broadcast();
}

void UPUProjectUmeowmiGameInstance::NotifyDialogueClosed()
{
	bDialogueOpen = false;
	OnDialogueClosedEvent.Broadcast();
}

void UPUProjectUmeowmiGameInstance::OnPopupWidgetClosed(FName ButtonID)
{
	// Restore input after popup closes - must use GameAndUI with DoNotLock (FInputModeGameOnly
	// would switch to LockOnCapture/CapturePermanently and break mouse in customization).
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			// Check if we're in dish customization (cooking/plating) - if so, keep move/look blocked
			bool bInCustomization = false;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (UPUDishCustomizationComponent* DishComp = It->FindComponentByClass<UPUDishCustomizationComponent>())
				{
					if (DishComp->IsCustomizing())
					{
						bInCustomization = true;
						break;
					}
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] OnPopupWidgetClosed - bInCustomization=%d"), bInCustomization);
			// Use Reset to clear stacked ignore state; SetIgnore* uses a counter that accumulates across popups
			PlayerController->ResetIgnoreInputFlags();
			AProjectUmeowmiCharacter* PlayerChar = nullptr;
			if (APawn* Pawn = PlayerController->GetPawn())
			{
				PlayerChar = Cast<AProjectUmeowmiCharacter>(Pawn);
			}
			bool bDialogueVisible = false;
			UWidget* FocusTarget = nullptr;
			if (PlayerChar)
			{
				if (UPUDialogueBox* DialogueBox = PlayerChar->GetDialogueBox())
				{
					bDialogueVisible = (DialogueBox->GetVisibility() == ESlateVisibility::Visible);
					if (bDialogueVisible)
					{
						FocusTarget = DialogueBox->GetFocusTarget();
					}
				}
			}
			if (bInCustomization || bDialogueVisible)
			{
				PlayerController->SetIgnoreMoveInput(true);
				PlayerController->SetIgnoreLookInput(true);
			}

			// GameAndUI + DoNotLock: allows free mouse for UI and 3D ingredient interaction
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

			// Restore focus to dialogue if still visible (popup had priority, now hand back to dialogue)
			if (FocusTarget)
			{
				if (TSharedPtr<SWidget> SlateWidget = FocusTarget->GetCachedWidget())
				{
					InputMode.SetWidgetToFocus(SlateWidget);
				}
			}
			PlayerController->SetInputMode(InputMode);
			PlayerController->bShowMouseCursor = true;

			UE_LOG(LogTemp, Log, TEXT("UPUProjectUmeowmiGameInstance::OnPopupWidgetClosed - Input restored (focus: %s)"), FocusTarget ? *FocusTarget->GetName() : TEXT("none"));
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

// --- Quest (forwards to UPUQuestSubsystem) ---
FGameplayTag UPUProjectUmeowmiGameInstance::GetQuestRootTag() const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q ? Q->QuestRootTag : FGameplayTag();
}

void UPUProjectUmeowmiGameInstance::SetQuestRootTag(const FGameplayTag& Tag)
{
	if (UPUQuestSubsystem* Q = GetQuestSubsystem())
	{
		Q->QuestRootTag = Tag;
	}
}

bool UPUProjectUmeowmiGameInstance::StartQuest(const FGameplayTag& QuestTag, const FGameplayTag& FirstObjectiveTag, bool bSave)
{
	return GetQuestSubsystem() ? GetQuestSubsystem()->StartQuest(QuestTag, FirstObjectiveTag, bSave) : false;
}

void UPUProjectUmeowmiGameInstance::SetActiveObjective(const FGameplayTag& ObjectiveTag, bool bSave)
{
	if (UPUQuestSubsystem* Q = GetQuestSubsystem())
	{
		Q->SetActiveObjective(ObjectiveTag, bSave);
	}
}

bool UPUProjectUmeowmiGameInstance::CompleteObjective(const FGameplayTag& ObjectiveTag, bool bSave)
{
	return GetQuestSubsystem() ? GetQuestSubsystem()->CompleteObjective(ObjectiveTag, bSave) : false;
}

bool UPUProjectUmeowmiGameInstance::CompleteQuest(const FGameplayTag& QuestTag, bool bSave)
{
	return GetQuestSubsystem() ? GetQuestSubsystem()->CompleteQuest(QuestTag, bSave) : false;
}

bool UPUProjectUmeowmiGameInstance::IsObjectiveCompleted(const FGameplayTag& ObjectiveTag) const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q && Q->IsObjectiveCompleted(ObjectiveTag);
}

bool UPUProjectUmeowmiGameInstance::IsQuestCompleted(const FGameplayTag& QuestTag) const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q && Q->IsQuestCompleted(QuestTag);
}

bool UPUProjectUmeowmiGameInstance::IsObjectiveActive(const FGameplayTag& ObjectiveTag) const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q && Q->IsObjectiveActive(ObjectiveTag);
}

bool UPUProjectUmeowmiGameInstance::IsQuestActive(const FGameplayTag& QuestTag) const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q && Q->IsQuestActive(QuestTag);
}

FGameplayTag UPUProjectUmeowmiGameInstance::GetActiveQuestTag() const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q ? Q->GetActiveQuestTag() : FGameplayTag();
}

FGameplayTag UPUProjectUmeowmiGameInstance::GetActiveObjectiveTag() const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q ? Q->GetActiveObjectiveTag() : FGameplayTag();
}

TSet<FGameplayTag> UPUProjectUmeowmiGameInstance::GetCompletedObjectiveTags() const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q ? Q->GetCompletedObjectiveTags() : TSet<FGameplayTag>();
}

TSet<FGameplayTag> UPUProjectUmeowmiGameInstance::GetCompletedQuestTags() const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q ? Q->GetCompletedQuestTags() : TSet<FGameplayTag>();
}

void UPUProjectUmeowmiGameInstance::AddObjectiveProgress(const FGameplayTag& ObjectiveTag, int32 Delta, bool bSave)
{
	if (UPUQuestSubsystem* Q = GetQuestSubsystem())
	{
		Q->AddObjectiveProgress(ObjectiveTag, Delta, bSave);
	}
}

int32 UPUProjectUmeowmiGameInstance::GetObjectiveProgress(const FGameplayTag& ObjectiveTag) const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q ? Q->GetObjectiveProgress(ObjectiveTag) : 0;
}

void UPUProjectUmeowmiGameInstance::ClearActiveQuestState(bool bSave)
{
	if (UPUQuestSubsystem* Q = GetQuestSubsystem())
	{
		Q->ClearActiveQuestState(bSave);
	}
}

bool UPUProjectUmeowmiGameInstance::GetObjectiveDisplayInfo(const FGameplayTag& ObjectiveTag, FPUQuestObjectiveDisplayInfo& OutInfo) const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q ? Q->GetObjectiveDisplayInfo(ObjectiveTag, OutInfo) : false;
}

bool UPUProjectUmeowmiGameInstance::GetActiveObjectiveDisplayInfo(FPUQuestObjectiveDisplayInfo& OutInfo) const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q ? Q->GetActiveObjectiveDisplayInfo(OutInfo) : false;
}

bool UPUProjectUmeowmiGameInstance::GetQuestDisplayInfo(const FGameplayTag& QuestTag, FText& OutQuestTitle, FText& OutFirstObjectiveTitle) const
{
	const UPUQuestSubsystem* Q = GetQuestSubsystem();
	return Q ? Q->GetQuestDisplayInfo(QuestTag, OutQuestTitle, OutFirstObjectiveTitle) : false;
}

