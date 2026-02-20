// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUJournalSectionWidget.h"
#include "PUIngredientsSectionWidget.generated.h"

/**
 * Ingredients section of the journal - displays discovered/collected ingredients.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPUIngredientsSectionWidget : public UPUJournalSectionWidget
{
	GENERATED_BODY()

public:
	UPUIngredientsSectionWidget(const FObjectInitializer& ObjectInitializer);
};
