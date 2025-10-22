// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ProjectUmeowmiGameMode.generated.h"

// Forward declarations
class ATalkingObject;
class UDlgDialogue;
class UDlgContext;
class UPUDialogueBox;

UCLASS(minimalapi)
class AProjectUmeowmiGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	// Cutscene state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cutscene")
	bool bCutsceneActive = false;
	
	// Cutscene dialogue reference
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cutscene")
	UDlgContext* CutsceneDialogueContext = nullptr;
	
	// Cutscene dialogue asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutscene")
	UDlgDialogue* LevelCutsceneDialogue = nullptr;
	
	// Whether to automatically start cutscene when level begins
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutscene")
	bool bAutoStartCutscene = false;
	
	// Timer handle for checking dialogue completion
	FTimerHandle DialogueCheckTimer;

public:
	AProjectUmeowmiGameMode();
	
	// Cutscene management
	UFUNCTION(BlueprintCallable, Category = "Cutscene")
	void StartLevelCutscene(UDlgDialogue* CutsceneDialogue);
	
	UFUNCTION(BlueprintCallable, Category = "Cutscene")
	void EndLevelCutscene();
	
	UFUNCTION(BlueprintCallable, Category = "Cutscene")
	bool IsCutsceneActive() const { return bCutsceneActive; }
	
	// Blueprint events for cutscene control
	UFUNCTION(BlueprintImplementableEvent, Category = "Cutscene")
	void OnCutsceneStarted();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Cutscene")
	void OnCutsceneEnded();
	
	// Dialogue completion callback
	UFUNCTION()
	void OnCutsceneDialogueEnded(UDlgContext* Context);
	
	// Override StartPlay to handle automatic cutscene
	virtual void StartPlay() override;
};



