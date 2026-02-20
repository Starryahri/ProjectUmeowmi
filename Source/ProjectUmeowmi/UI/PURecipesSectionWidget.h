// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUJournalSectionWidget.h"
#include "PURecipesSectionWidget.generated.h"

/**
 * Recipes section of the journal - displays discovered recipes in a two-page spread layout.
 * Based on the reference: left page and right page each show a recipe with title, description,
 * difficulty stars, ingredients list, liked-by avatars, flavor profile, and illustration.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPURecipesSectionWidget : public UPUJournalSectionWidget
{
	GENERATED_BODY()

public:
	UPURecipesSectionWidget(const FObjectInitializer& ObjectInitializer);
};
