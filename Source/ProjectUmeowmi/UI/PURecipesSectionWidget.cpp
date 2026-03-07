// Copyright Epic Games, Inc. All Rights Reserved.

#include "PURecipesSectionWidget.h"
#include "PURecipeIngredientEntryWidget.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "../DishCustomization/PUDishBase.h"
#include "../DishCustomization/PUDishBlueprintLibrary.h"
#include "Components/VerticalBox.h"
#include "Components/Image.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

UPURecipesSectionWidget::UPURecipesSectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionType = EJournalSectionType::Recipes;
}

void UPURecipesSectionWidget::OnSectionActivated_Implementation()
{
	Super::OnSectionActivated_Implementation();
	// When Recipes tab is shown, display the current dish (from Game Instance)
	UWorld* World = GetWorld();
	if (World)
	{
		if (UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>())
		{
			FGameplayTag CurrentTag = GI->GetCurrentDishTag();
			if (!CurrentTag.IsValid())
			{
				// No current dish - use first unlocked
				TArray<FGameplayTag> Ordered = GI->GetOrderedUnlockedDishTags();
				if (Ordered.Num() > 0)
				{
					CurrentTag = Ordered[0];
					GI->SetCurrentDishTag(CurrentTag);
				}
			}
			if (CurrentTag.IsValid())
			{
				DisplayDishByTag(CurrentTag);
			}
		}
	}
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

void UPURecipesSectionWidget::SetRecipeIllustration(UImage* Image, const FPUDishBase& DishData)
{
	if (!Image) return;

	UTexture2D* LoadedTexture = UPUDishBlueprintLibrary::GetLoadedJournalTexture(DishData);
	if (LoadedTexture)
	{
		Image->SetBrushFromTexture(LoadedTexture);
	}
}

void UPURecipesSectionWidget::DisplayDishByTag_Implementation(const FGameplayTag& DishTag)
{
	if (!DishTag.IsValid()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>();
	if (!GI) return;

	FPUDishBase DishData;
	if (!GI->GetDishDataForTag(DishTag, DishData)) return;

	if (IngredientsContainer)
	{
		PopulateIngredientsList(IngredientsContainer, DishData);
	}
	if (RecipeIllustrationImage)
	{
		SetRecipeIllustration(RecipeIllustrationImage, DishData);
	}
}
