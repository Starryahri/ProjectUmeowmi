// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUJournalAspectRowWidget.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Components/RichTextBlock.h"

UPUJournalAspectRowWidget::UPUJournalAspectRowWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPUJournalAspectRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateDisplay();
}

void UPUJournalAspectRowWidget::SetAspectNameAndStarRating(FName InAspectName, int32 StarRating0to5)
{
	AspectName = InAspectName;
	StarRating = FMath::Clamp(StarRating0to5, 0, 5);
	UpdateDisplay();
}

void UPUJournalAspectRowWidget::UpdateDisplay()
{
	if (!WidgetTree)
	{
		return;
	}

	if (AspectNameText)
	{
		AspectNameText->SetText(FText::FromName(AspectName));
	}

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
				if (!StarImage)
				{
					continue;
				}

				StarImage->SetBrushFromTexture((i < StarRating) ? FilledTex : UnfilledTex);
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
					StarsStr += (i < StarRating) ? TEXT("★") : TEXT("☆");
				}
				StarsBlock->SetText(FText::FromString(StarsStr));
				StarRatingContainer->AddChild(StarsBlock);
			}
		}
	}

	if (AspectBorder && AspectColorDataTable && !AspectName.IsNone())
	{
		if (const FRichTextStyleRow* StyleRow = AspectColorDataTable->FindRow<FRichTextStyleRow>(AspectName, TEXT("JournalAspectColor"), false))
		{
			const FLinearColor Color = StyleRow->TextStyle.ColorAndOpacity.GetSpecifiedColor();
			AspectBorder->SetBrushColor(Color);
		}
	}
}
