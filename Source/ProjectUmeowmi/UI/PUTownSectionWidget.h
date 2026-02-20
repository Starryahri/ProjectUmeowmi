// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUJournalSectionWidget.h"
#include "PUTownSectionWidget.generated.h"

/**
 * Town section of the journal - displays town/location info, map, or discovered places.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPUTownSectionWidget : public UPUJournalSectionWidget
{
	GENERATED_BODY()

public:
	UPUTownSectionWidget(const FObjectInitializer& ObjectInitializer);
};
