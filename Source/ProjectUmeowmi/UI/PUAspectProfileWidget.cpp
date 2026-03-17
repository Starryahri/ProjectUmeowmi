#include "PUAspectProfileWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/PanelWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"
#include "Components/RichTextBlock.h"

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

	// Top 3 contributing ingredient icons
	if (IngredientsContainer)
	{
		IngredientsContainer->ClearChildren();
		for (const FPUBaseIngredientEntry& Entry : AspectData.TopContributingIngredients)
		{
			if (!Entry.PreviewTexture) continue;

			UImage* IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			if (!IconImage) continue;

			IconImage->SetBrushFromTexture(Entry.PreviewTexture);
			USizeBox* IconSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			if (IconSizeBox)
			{
				IconSizeBox->SetWidthOverride(128.0f);
				IconSizeBox->SetHeightOverride(128.0f);
				IconSizeBox->AddChild(IconImage);
				IngredientsContainer->AddChild(IconSizeBox);
			}
			else
			{
				IngredientsContainer->AddChild(IconImage);
			}
		}
	}

	// Star rating - use custom textures if both set, else fallback to ★/☆ text
	if (StarRatingContainer)
	{
		StarRatingContainer->ClearChildren();
		UTexture2D* FilledTex = StarTextureFilled.LoadSynchronous();
		UTexture2D* UnfilledTex = StarTextureUnfilled.LoadSynchronous();
		if (FilledTex && UnfilledTex)
		{
			for (int32 i = 0; i < 5; ++i)
			{
				UImage* StarImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				if (!StarImage) continue;

				StarImage->SetBrushFromTexture((i < AspectData.StarRating) ? FilledTex : UnfilledTex);
				USizeBox* StarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
				if (StarSizeBox)
				{
					StarSizeBox->SetWidthOverride(StarImageSize);
					StarSizeBox->SetHeightOverride(StarImageSize);
					StarSizeBox->AddChild(StarImage);
					StarRatingContainer->AddChild(StarSizeBox);
				}
				else
				{
					StarRatingContainer->AddChild(StarImage);
				}
			}
		}
		else
		{
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

	// Aspect border color from Rich Text Style data table (row name = aspect name)
	if (AspectBorder && AspectColorDataTable && AspectData.AspectName.IsValid())
	{
		if (const FRichTextStyleRow* StyleRow = AspectColorDataTable->FindRow<FRichTextStyleRow>(AspectData.AspectName, TEXT("AspectColor")))
		{
			FLinearColor Color = StyleRow->TextStyle.ColorAndOpacity.GetSpecifiedColor();
			AspectBorder->SetBrushColor(Color);
		}
	}
}
