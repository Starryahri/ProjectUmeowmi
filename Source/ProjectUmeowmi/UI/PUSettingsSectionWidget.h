// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUJournalSectionWidget.h"
#include "PUSettingsSectionWidget.generated.h"

/**
 * Settings section of the journal - game options, audio, controls, etc.
 * Typically includes a gear icon on the tab (per reference).
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPUSettingsSectionWidget : public UPUJournalSectionWidget
{
	GENERATED_BODY()

public:
	UPUSettingsSectionWidget(const FObjectInitializer& ObjectInitializer);
};
