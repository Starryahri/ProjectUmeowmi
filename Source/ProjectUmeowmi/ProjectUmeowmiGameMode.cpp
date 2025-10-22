// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectUmeowmiGameMode.h"
#include "ProjectUmeowmiCharacter.h"
#include "DlgSystem/DlgManager.h"
#include "DlgSystem/DlgContext.h"
#include "GameFramework/PlayerController.h"
#include "UI/PUDialogueBox.h"

#include "UObject/ConstructorHelpers.h"

AProjectUmeowmiGameMode::AProjectUmeowmiGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Characters/Bao/Blueprints/BP_Bao"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	
	// Initialize cutscene settings
	bCutsceneActive = false;
	bAutoStartCutscene = false;
}

void AProjectUmeowmiGameMode::StartPlay()
{
	Super::StartPlay();
	
	// Check if we should automatically start a cutscene
	if (bAutoStartCutscene && LevelCutsceneDialogue)
	{
		// Start the cutscene after a short delay to ensure everything is initialized
		FTimerHandle CutsceneTimer;
		GetWorld()->GetTimerManager().SetTimer(CutsceneTimer, [this]()
		{
			StartLevelCutscene(LevelCutsceneDialogue);
		}, 0.5f, false);
	}
}

void AProjectUmeowmiGameMode::StartLevelCutscene(UDlgDialogue* CutsceneDialogue)
{
	if (!CutsceneDialogue || bCutsceneActive)
	{
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Starting level cutscene with dialogue: %s"), *CutsceneDialogue->GetName());
	
	// Disable player input but keep mouse visible
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		PC->bShowMouseCursor = true; // Always keep mouse visible
	}
	
	// Get the player character to access the dialogue box
	ACharacter* PlayerCharacter = Cast<ACharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get player character for cutscene"));
		return;
	}
	
	AProjectUmeowmiCharacter* ProjectCharacter = Cast<AProjectUmeowmiCharacter>(PlayerCharacter);
	if (!ProjectCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to cast player character to ProjectUmeowmiCharacter"));
		return;
	}
	
	// Create participants array (just the player character for cutscenes)
	TArray<UObject*> Participants;
	Participants.Add(ProjectCharacter);
	
	// Start the dialogue
	CutsceneDialogueContext = UDlgManager::StartDialogue(CutsceneDialogue, Participants);
	if (CutsceneDialogueContext)
	{
		// Get the dialogue box and open it
		UPUDialogueBox* DialogueBox = ProjectCharacter->GetDialogueBox();
		if (DialogueBox)
		{
			DialogueBox->Open(CutsceneDialogueContext);
		}
		
		bCutsceneActive = true;
		
		// Call Blueprint event
		OnCutsceneStarted();
		
		// Set up a timer to check for dialogue completion
		GetWorld()->GetTimerManager().SetTimer(DialogueCheckTimer, [this]()
		{
			if (CutsceneDialogueContext && CutsceneDialogueContext->HasDialogueEnded())
			{
				OnCutsceneDialogueEnded(CutsceneDialogueContext);
			}
		}, 0.1f, true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to start cutscene dialogue"));
		EndLevelCutscene();
	}
}

void AProjectUmeowmiGameMode::EndLevelCutscene()
{
	if (!bCutsceneActive)
	{
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Ending level cutscene"));
	
	// Re-enable player input and ensure mouse is visible
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		PC->bShowMouseCursor = true; // Always keep mouse visible
	}
	
	// Clear the dialogue check timer
	GetWorld()->GetTimerManager().ClearTimer(DialogueCheckTimer);
	
	// Clean up dialogue context
	if (CutsceneDialogueContext)
	{
		// Close the dialogue box if it's open
		if (ACharacter* PlayerCharacter = Cast<ACharacter>(GetWorld()->GetFirstPlayerController()->GetPawn()))
		{
			if (AProjectUmeowmiCharacter* ProjectCharacter = Cast<AProjectUmeowmiCharacter>(PlayerCharacter))
			{
				if (UPUDialogueBox* DialogueBox = ProjectCharacter->GetDialogueBox())
				{
					DialogueBox->Close();
				}
			}
		}
		
		CutsceneDialogueContext = nullptr;
	}
	
	bCutsceneActive = false;
	
	// Call Blueprint event
	OnCutsceneEnded();
}

void AProjectUmeowmiGameMode::OnCutsceneDialogueEnded(UDlgContext* Context)
{
	UE_LOG(LogTemp, Log, TEXT("Cutscene dialogue ended"));
	EndLevelCutscene();
}


