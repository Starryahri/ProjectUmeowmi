// Copyright Epic Games, Inc. All Rights Reserved.

#include "PURecipesSectionWidget.h"
#include "PURecipeIngredientEntryWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "Components/VerticalBox.h"
#include "Blueprint/UserWidget.h"

UPURecipesSectionWidget::UPURecipesSectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionType = EJournalSectionType::Recipes;
}

void UPURecipesSectionWidget::PopulateIngredientsList(UVerticalBox* Container, const FPUDishBase& DishData,
	TSubclassOf<UPURecipeIngredientEntryWidget> EntryWidgetClass)
{
	if (!Container) return;

	Container->ClearChildren();

	TSubclassOf<UPURecipeIngredientEntryWidget> ClassToUse = EntryWidgetClass;
	if (!ClassToUse)
	{
		ClassToUse = UPURecipeIngredientEntryWidget::StaticClass();
	}

	for (const FIngredientInstance& Instance : DishData.IngredientInstances)
	{
		FText DisplayName = Instance.IngredientData.DisplayName;
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromName(Instance.IngredientTag.GetTagName());
		}

		UPURecipeIngredientEntryWidget* Entry = CreateWidget<UPURecipeIngredientEntryWidget>(this, ClassToUse);
		if (Entry)
		{
			Entry->SetData(DisplayName, false);
			Container->AddChild(Entry);
		}
	}
}
