#include "PUProjectUmeowmiGameInstance.h"
#include "ProjectUmeowmiCharacter.h"
#include "LevelTransition/PULevelSpawnPoint.h"
#include "PUPlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"

UPUProjectUmeowmiGameInstance::UPUProjectUmeowmiGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHasSavedOrder = false;
	bTransitionInProgress = false;
	PlayerSaveGame = nullptr;
}

void UPUProjectUmeowmiGameInstance::Init()
{
	Super::Init();
	
	// Try to load existing save game, or create new one if it doesn't exist
	if (!LoadGame())
	{
		CreateNewGame();
	}
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

	// Build the level path - OpenLevel expects just the level name or full path
	// If TargetLevelName already includes path, use it; otherwise construct it
	FString LevelPath = TargetLevelName;
	if (!LevelPath.StartsWith(TEXT("/Game/")))
	{
		// Check if this is a Chapter0 map (in subdirectory)
		FString Subdirectory = TEXT("");
		if (TargetLevelName.StartsWith(TEXT("L_Chapter0_")))
		{
			Subdirectory = TEXT("Chapter0/");
		}
		
		// Construct full path with subdirectory if needed
		if (Subdirectory.IsEmpty())
		{
			LevelPath = FString::Printf(TEXT("/Game/LuckyFatCatDiner/Maps/%s"), *TargetLevelName);
		}
		else
		{
			LevelPath = FString::Printf(TEXT("/Game/LuckyFatCatDiner/Maps/%s%s"), *Subdirectory, *TargetLevelName);
		}
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
		UGameplayStatics::OpenLevel(World, FName(*LevelPath));
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
	// Note: OnPostLoadMap will be called automatically when the level finishes loading
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

void UPUProjectUmeowmiGameInstance::CreateNewGame()
{
	// Clear all unlocked ingredients and dialogue states
	UnlockedIngredientTags.Empty();
	CompletedDialogueNames.Empty();

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

