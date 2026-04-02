// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUJournalSectionWidget.h"
#include "PUQuestsSectionWidget.generated.h"

/**
 * Quests / objectives section of the journal — active and completed quests, objectives, tracking UI.
 * Layout and data binding live in a Blueprint child (e.g. WBP_QuestsSection).
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPUQuestsSectionWidget : public UPUJournalSectionWidget
{
	GENERATED_BODY()

public:
	UPUQuestsSectionWidget(const FObjectInitializer& ObjectInitializer);
};
