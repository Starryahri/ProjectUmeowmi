#include "PUProjectUmeowmiGameInstance.h"
#include "ProjectUmeowmiCharacter.h"
#include "LevelTransition/PULevelSpawnPoint.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UPUProjectUmeowmiGameInstance::UPUProjectUmeowmiGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHasSavedOrder = false;
	bTransitionInProgress = false;
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

	// Use fade if requested
	if (bUseFade)
	{
		// Fade out
		PlayerController->ClientSetCameraFade(true, FColor::Black, FVector2D::ZeroVector, 0.0f, true, true);
	}

	// Load the level
	// Note: OnPostLoadMap will be called automatically when the level finishes loading
	UGameplayStatics::OpenLevel(World, FName(*LevelPath));
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

	// Fade in
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->ClientSetCameraFade(false, FColor::Black, FVector2D::ZeroVector, 0.5f, true, true);
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

