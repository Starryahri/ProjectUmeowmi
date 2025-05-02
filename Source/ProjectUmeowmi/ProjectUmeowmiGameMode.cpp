// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectUmeowmiGameMode.h"
#include "ProjectUmeowmiCharacter.h"
#include "UObject/ConstructorHelpers.h"

AProjectUmeowmiGameMode::AProjectUmeowmiGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Characters/Bao/Blueprints/BP_Bao"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
