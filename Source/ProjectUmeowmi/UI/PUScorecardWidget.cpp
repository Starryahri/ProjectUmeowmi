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
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** Parent for creating a MID: class default, or brush material (resolves instance/MID to parent). */
	UMaterialInterface* ResolveDishImageParentMaterial(UMaterialInterface* DishImageMaterialFromDefaults, UImage* DishImageWidget)
	{
		if (DishImageMaterialFromDefaults)
		{
			return DishImageMaterialFromDefaults;
		}
		if (!DishImageWidget)
		{
			return nullptr;
		}
		UMaterialInterface* BrushMat = Cast<UMaterialInterface>(DishImageWidget->GetBrush().GetResourceObject());
		if (!BrushMat)
		{
			return nullptr;
		}
		if (UMaterialInstanceDynamic* AsMID = Cast<UMaterialInstanceDynamic>(BrushMat))
		{
			return AsMID->GetMaterial();
		}
		if (UMaterialInstance* AsMI = Cast<UMaterialInstance>(BrushMat))
		{
			return AsMI->GetMaterial();
		}
		return BrushMat;
	}
}

UPUScorecardWidget::UPUScorecardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPUScorecardWidget::AddToViewportScoringStack()
{
	if (GetParent())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] AddToViewportScoringStack: widget is embedded under %s — cannot AddToViewport. Toggle visibility in Blueprint instead."),
			*GetParent()->GetName());
		return;
	}
	AddToViewport(PUScorecardViewportZOrder);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPUScorecardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Draw above scoring dialogue (viewport Z) but let pointer events fall through to dialogue except where children hit-test (buttons, etc.).
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UpdateDisplay();
}

void UPUScorecardWidget::SetScorecardData(const FPUScorecardData& InData)
{
	ScorecardData = InData;
	UpdateDisplay();
}

void UPUScorecardWidget::SetDishImage(UTexture* DishTexture)
{
	OverrideDishTexture = DishTexture;
	UpdateDisplay();
}

void UPUScorecardWidget::ShowFromOrder(const FPUOrderBase& Order, UTexture* OptionalDishTexture)
{
	ScorecardData = UPUDishBlueprintLibrary::GetScorecardData(Order);
	OverrideDishTexture = OptionalDishTexture;
	if (!OverrideDishTexture)
	{
		OverrideDishTexture = UPUDishBlueprintLibrary::GetLoadedPreviewTexture(Order.GetCompletedDish());
	}
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

bool UPUScorecardWidget::EnsureDishImageMIDForDish()
{
	if (!DishImage)
	{
		return false;
	}

	UMaterialInterface* ParentMat = ResolveDishImageParentMaterial(DishImageMaterial.Get(), DishImage);
	if (!ParentMat)
	{
		return false;
	}

	if (!DishImageMID || !IsValid(DishImageMID) || DishImageMaterialUsedForMID != ParentMat)
	{
		DishImageMID = UMaterialInstanceDynamic::Create(ParentMat, this);
		DishImageMaterialUsedForMID = ParentMat;
	}

	return DishImageMID != nullptr;
}

void UPUScorecardWidget::UpdateDisplay()
{
	// Dish name text
	if (DishNameText)
	{
		DishNameText->SetText(ScorecardData.DisplayName);
	}

	// Dish image: material with "DishRender" parameter when possible, else plain texture brush
	if (DishImage)
	{
		if (OverrideDishTexture)
		{
			if (EnsureDishImageMIDForDish() && DishImageMID)
			{
				static const FName DishRenderParam(TEXT("DishRender"));
				DishImageMID->SetTextureParameterValue(DishRenderParam, OverrideDishTexture);
				DishImage->SetBrushFromMaterial(DishImageMID);
			}
			else if (UTexture2D* Tex2D = Cast<UTexture2D>(OverrideDishTexture))
			{
				DishImage->SetBrushFromTexture(Tex2D);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Scorecard] Dish texture is a render target or non-2D texture; assign DishImageMaterial on the scorecard widget class (parent material with DishRender parameter)."));
			}
		}
		// If no override, Blueprint/caller can set from Order.GetCompletedDish().PreviewTexture
	}

	UpdateSealImage();

	// Base ingredients: SizeBox -> Overlay -> IconImage + checkmark/X (bottom-right)
	if (BaseIngredientsContainer && WidgetTree)
	{
		BaseIngredientsContainer->ClearChildren();
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
	case EPUScorecardSealTier::Okay:
		SealTex = SealTextureOkay.LoadSynchronous();
		if (!SealTex) SealTex = SealTextureGood.LoadSynchronous(); // Fallback for existing Blueprints
		break;
	case EPUScorecardSealTier::NeedsImprovement:
		SealTex = SealTextureNeedsImprovement.LoadSynchronous();
		break;
	default:
		SealTex = SealTextureOkay.LoadSynchronous();
		break;
	}

	if (SealTex)
	{
		SealImage->SetBrushFromTexture(SealTex);
		SealImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] UpdateSealImage: Seal texture NULL for tier %d (check Class Defaults)"), (int32)ScorecardData.SealTier);
	}
}
