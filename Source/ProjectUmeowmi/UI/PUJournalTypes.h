// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUJournalTypes.generated.h"

/** Journal section types - corresponds to the tabs in the recipe book */
UENUM(BlueprintType)
enum class EJournalSectionType : uint8
{
	Recipes     UMETA(DisplayName = "Recipes"),
	Ingredients UMETA(DisplayName = "Ingredients"),
	People      UMETA(DisplayName = "People"),
	Town        UMETA(DisplayName = "Town"),
	Settings    UMETA(DisplayName = "Settings")
};

/** Tab name IDs for registration with CommonTabListWidgetBase */
namespace JournalTabNames
{
	const FName Recipes     = FName(TEXT("Recipes"));
	const FName Ingredients = FName(TEXT("Ingredients"));
	const FName People      = FName(TEXT("People"));
	const FName Town        = FName(TEXT("Town"));
	const FName Settings    = FName(TEXT("Settings"));
}
