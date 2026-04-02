// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUJournalTypes.generated.h"

/** One row in the journal tab list — edit the Journal Tabs array on the journal widget. */
USTRUCT(BlueprintType)
struct FPUJournalTabEntry
{
	GENERATED_BODY()

	/** Unique id for this tab. Used with Switch To Tab By Id and Common UI registration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	FName TabId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	TSubclassOf<UUserWidget> SectionWidgetClass;

	/** If empty, the tab label is derived from TabId (first letter capitalized). Use Display Name Override for custom labels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	FText DisplayNameOverride;
};
