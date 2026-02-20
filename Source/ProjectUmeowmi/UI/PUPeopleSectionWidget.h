// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUJournalSectionWidget.h"
#include "PUPeopleSectionWidget.generated.h"

/**
 * People section of the journal - displays NPCs/characters met, their preferences, etc.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPUPeopleSectionWidget : public UPUJournalSectionWidget
{
	GENERATED_BODY()

public:
	UPUPeopleSectionWidget(const FObjectInitializer& ObjectInitializer);
};
