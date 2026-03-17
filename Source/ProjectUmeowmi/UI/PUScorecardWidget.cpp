#include "PUScorecardWidget.h"
#include "PUAspectProfileWidget.h"
#include "../DishCustomization/PUDishBlueprintLibrary.h"
#include "../DishCustomization/PUOrderBase.h"
#include "../DishCustomization/PUDishBase.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

UPUScorecardWidget::UPUScorecardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPUScorecardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogTemp, Display, TEXT("[Scorecard] NativeConstruct: Bindings (DishImage=%s, SealImage=%s, BaseIngredientsContainer=%s)"), DishImage ? TEXT("OK") : TEXT("NULL"), SealImage ? TEXT("OK") : TEXT("NULL"), BaseIngredientsContainer ? TEXT("OK") : TEXT("NULL"));
	UpdateDisplay();
}

void UPUScorecardWidget::SetScorecardData(const FPUScorecardData& InData)
{
	ScorecardData = InData;
	UpdateDisplay();
}

void UPUScorecardWidget::SetDishImage(UTexture2D* DishTexture)
{
	OverrideDishTexture = DishTexture;
	UpdateDisplay();
}

void UPUScorecardWidget::ShowFromOrder(const FPUOrderBase& Order, UTexture2D* OptionalDishTexture)
{
	UE_LOG(LogTemp, Display, TEXT("[Scorecard] ShowFromOrder: Building scorecard data (Satisfaction=%.2f)"), Order.GetFinalSatisfactionScore());
	ScorecardData = UPUDishBlueprintLibrary::GetScorecardData(Order);
	OverrideDishTexture = OptionalDishTexture;
	if (!OverrideDishTexture)
	{
		OverrideDishTexture = UPUDishBlueprintLibrary::GetLoadedPreviewTexture(Order.GetCompletedDish());
	}
	UE_LOG(LogTemp, Display, TEXT("[Scorecard] ShowFromOrder: SealTier=%d, BaseIngredients=%d, FlavorAspects=%d, TextureAspects=%d, DishTex=%s"),
		(int32)ScorecardData.SealTier, ScorecardData.BaseIngredients.Num(), ScorecardData.FlavorProfile.TopAspects.Num(), ScorecardData.TextureProfile.TopAspects.Num(),
		OverrideDishTexture ? TEXT("OK") : TEXT("NULL"));
	UpdateDisplay();
	PlaySealAnimation();
}

void UPUScorecardWidget::PlaySealAnimation()
{
	// Seal animation: same for all tiers. Set the seal image first, then animate.
	// Blueprint can override to add custom animation (e.g. scale in, fade in).
	UpdateSealImage();
	// TODO: Trigger animation - can be done in Blueprint override or via Animation blueprint
}

void UPUScorecardWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnScorecardClosed.Broadcast();
}

void UPUScorecardWidget::UpdateDisplay()
{
	UE_LOG(LogTemp, Display, TEXT("[Scorecard] UpdateDisplay: DishImage=%s, SealImage=%s, BaseIngredientsContainer=%s, FlavorProfileContainer=%s, TextureProfileContainer=%s, AspectProfileWidgetClass=%s"),
		DishImage ? TEXT("OK") : TEXT("NULL"), SealImage ? TEXT("OK") : TEXT("NULL"),
		BaseIngredientsContainer ? TEXT("OK") : TEXT("NULL"), FlavorProfileContainer ? TEXT("OK") : TEXT("NULL"), TextureProfileContainer ? TEXT("OK") : TEXT("NULL"),
		AspectProfileWidgetClass ? *AspectProfileWidgetClass->GetName() : TEXT("NULL (using C++ default)"));

	// Dish name text
	if (DishNameText)
	{
		DishNameText->SetText(ScorecardData.DisplayName);
	}

	// Dish image: use override or leave for Blueprint to set from CompletedDish
	if (DishImage)
	{
		if (OverrideDishTexture)
		{
			DishImage->SetBrushFromTexture(OverrideDishTexture);
		}
		// If no override, Blueprint/caller can set from Order.GetCompletedDish().PreviewTexture
	}

	UpdateSealImage();

	// Base ingredients: SizeBox -> Overlay -> IconImage + checkmark/X (bottom-right)
	if (BaseIngredientsContainer && WidgetTree)
	{
		BaseIngredientsContainer->ClearChildren();
		UE_LOG(LogTemp, Display, TEXT("[Scorecard] UpdateDisplay: Adding %d base ingredients"), ScorecardData.BaseIngredients.Num());
		for (const FPUBaseIngredientEntry& Entry : ScorecardData.BaseIngredients)
		{
			USizeBox* EntrySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			if (!EntrySizeBox) continue;

			EntrySizeBox->SetWidthOverride(128.0f);
			EntrySizeBox->SetHeightOverride(128.0f);

			UOverlay* EntryOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			if (!EntryOverlay)
			{
				BaseIngredientsContainer->AddChild(EntrySizeBox);
				continue;
			}

			// Icon (fills overlay at 128x128; use placeholder if no texture so checkmark/X still shows)
			UImage* IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			if (IconImage)
			{
				if (Entry.PreviewTexture)
				{
					IconImage->SetBrushFromTexture(Entry.PreviewTexture);
					FSlateBrush Brush = IconImage->GetBrush();
					Brush.SetImageSize(FVector2D(128.0f, 128.0f));
					IconImage->SetBrush(Brush);
				}
				EntryOverlay->AddChild(IconImage);
			}

			// Checkmark/X in bottom-right of overlay
			UTexture2D* CheckTex = CheckmarkTexture.LoadSynchronous();
			UTexture2D* XTex = XTexture.LoadSynchronous();
			UWidget* StatusWidget = nullptr;
			if (CheckTex && XTex)
			{
				UImage* StatusImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				if (StatusImage)
				{
					StatusImage->SetBrushFromTexture(Entry.bObtained ? CheckTex : XTex);
					FSlateBrush StatusBrush = StatusImage->GetBrush();
					StatusBrush.SetImageSize(FVector2D(StatusIconSize, StatusIconSize));
					StatusImage->SetBrush(StatusBrush);
					USizeBox* StatusSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
					if (StatusSizeBox)
					{
						StatusSizeBox->SetWidthOverride(StatusIconSize);
						StatusSizeBox->SetHeightOverride(StatusIconSize);
						StatusSizeBox->AddChild(StatusImage);
						StatusWidget = StatusSizeBox;
					}
					else
					{
						StatusWidget = StatusImage;
					}
				}
			}
			else
			{
				UTextBlock* StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				if (StatusText)
				{
					StatusText->SetText(FText::FromString(Entry.bObtained ? TEXT("\u2713") : TEXT("\u2717")));
					StatusText->SetColorAndOpacity(FSlateColor(Entry.bObtained ? FLinearColor::Green : FLinearColor::Red));
					StatusWidget = StatusText;
				}
			}

			if (StatusWidget)
			{
				if (UOverlaySlot* StatusSlot = Cast<UOverlaySlot>(EntryOverlay->AddChild(StatusWidget)))
				{
					StatusSlot->SetHorizontalAlignment(HAlign_Right);
					StatusSlot->SetVerticalAlignment(VAlign_Bottom);
					StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 4.0f));
				}
			}

			EntrySizeBox->AddChild(EntryOverlay);
			BaseIngredientsContainer->AddChild(EntrySizeBox);
		}
	}

	// Flavor profile - spawn 2 aspect widgets (one per top aspect)
	if (FlavorProfileContainer)
	{
		FlavorProfileContainer->ClearChildren();
		UE_LOG(LogTemp, Display, TEXT("[Scorecard] UpdateDisplay: Flavor profile - spawning %d aspect widgets"), ScorecardData.FlavorProfile.TopAspects.Num());
		TSubclassOf<UPUAspectProfileWidget> ClassToUse = AspectProfileWidgetClass;
		if (!ClassToUse)
		{
			ClassToUse = UPUAspectProfileWidget::StaticClass();
		}
		for (const FPUAspectRanking& Aspect : ScorecardData.FlavorProfile.TopAspects)
		{
			if (UPUAspectProfileWidget* AspectWidget = CreateWidget<UPUAspectProfileWidget>(this, ClassToUse))
			{
				AspectWidget->SetAspectData(Aspect);
				FlavorProfileContainer->AddChild(AspectWidget);
			}
		}
	}

	// Texture profile - spawn 2 aspect widgets (one per top aspect)
	if (TextureProfileContainer)
	{
		TextureProfileContainer->ClearChildren();
		UE_LOG(LogTemp, Display, TEXT("[Scorecard] UpdateDisplay: Texture profile - spawning %d aspect widgets"), ScorecardData.TextureProfile.TopAspects.Num());
		TSubclassOf<UPUAspectProfileWidget> ClassToUse = AspectProfileWidgetClass;
		if (!ClassToUse)
		{
			ClassToUse = UPUAspectProfileWidget::StaticClass();
		}
		for (const FPUAspectRanking& Aspect : ScorecardData.TextureProfile.TopAspects)
		{
			if (UPUAspectProfileWidget* AspectWidget = CreateWidget<UPUAspectProfileWidget>(this, ClassToUse))
			{
				AspectWidget->SetAspectData(Aspect);
				TextureProfileContainer->AddChild(AspectWidget);
			}
		}
	}
}

void UPUScorecardWidget::UpdateSealImage()
{
	if (!SealImage) return;

	UTexture2D* SealTex = nullptr;
	switch (ScorecardData.SealTier)
	{
	case EPUScorecardSealTier::Perfect:
		SealTex = SealTexturePerfect.LoadSynchronous();
		break;
	case EPUScorecardSealTier::Great:
		SealTex = SealTextureGreat.LoadSynchronous();
		break;
	case EPUScorecardSealTier::Good:
	default:
		SealTex = SealTextureGood.LoadSynchronous();
		break;
	}

	if (SealTex)
	{
		SealImage->SetBrushFromTexture(SealTex);
		SealImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		UE_LOG(LogTemp, Display, TEXT("[Scorecard] UpdateSealImage: Set seal texture for tier %d"), (int32)ScorecardData.SealTier);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] UpdateSealImage: Seal texture NULL for tier %d (check Class Defaults)"), (int32)ScorecardData.SealTier);
	}
}
