#include "PUAspectProfileWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

UPUAspectProfileWidget::UPUAspectProfileWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPUAspectProfileWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateDisplay();
}

void UPUAspectProfileWidget::SetAspectData(const FPUAspectRanking& InRanking)
{
	AspectData = InRanking;
	UE_LOG(LogTemp, Display, TEXT("[Scorecard] SetAspectData: Aspect=%s, TopIngredients=%d, StarRating=%d"), *AspectData.AspectName.ToString(), AspectData.TopContributingIngredients.Num(), AspectData.StarRating);
	UpdateDisplay();
}

void UPUAspectProfileWidget::UpdateDisplay()
{
	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] PUAspectProfileWidget::UpdateDisplay: WidgetTree is NULL"));
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("[Scorecard] PUAspectProfileWidget::UpdateDisplay: AspectNameText=%s, IngredientsContainer=%s, StarRatingContainer=%s"),
		AspectNameText ? TEXT("OK") : TEXT("NULL"), IngredientsContainer ? TEXT("OK") : TEXT("NULL"), StarRatingContainer ? TEXT("OK") : TEXT("NULL"));

	// Aspect name
	if (AspectNameText)
	{
		AspectNameText->SetText(FText::FromName(AspectData.AspectName));
	}

	// Top 3 contributing ingredients
	if (IngredientsContainer)
	{
		IngredientsContainer->ClearChildren();
		for (const FText& Ingredient : AspectData.TopContributingIngredients)
		{
			UTextBlock* IngBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			if (IngBlock)
			{
				IngBlock->SetText(Ingredient);
				if (UHorizontalBoxSlot* IngSlot = Cast<UHorizontalBoxSlot>(IngredientsContainer->AddChild(IngBlock)))
				{
					IngSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
				}
			}
		}
	}

	// Star rating (e.g. ★★★☆☆)
	if (StarRatingContainer)
	{
		StarRatingContainer->ClearChildren();
		UTextBlock* StarsBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (StarsBlock)
		{
			FString StarsStr;
			for (int32 i = 0; i < 5; ++i)
			{
				StarsStr += (i < AspectData.StarRating) ? TEXT("★") : TEXT("☆");
			}
			StarsBlock->SetText(FText::FromString(StarsStr));
			StarRatingContainer->AddChild(StarsBlock);
		}
	}
}
