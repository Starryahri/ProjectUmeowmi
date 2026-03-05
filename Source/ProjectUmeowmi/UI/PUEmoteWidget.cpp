#include "ProjectUmeowmi/UI/PUEmoteWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "MovieScene.h"

void UPUEmoteWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPUEmoteWidget::SetEmoteIcon(UTexture2D* Icon)
{
	if (EmoteImage && Icon)
	{
		EmoteImage->SetBrushFromTexture(Icon);
		EmoteImage->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Display, TEXT("[Emote] UPUEmoteWidget::SetEmoteIcon - set icon to %s"), *Icon->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[Emote] UPUEmoteWidget::SetEmoteIcon - no-op (EmoteImage=%s Icon=%s)"), EmoteImage ? TEXT("valid") : TEXT("null"), Icon ? *Icon->GetName() : TEXT("null"));
	}
}

void UPUEmoteWidget::ClearEmoteIcon()
{
	if (EmoteImage)
	{
		EmoteImage->SetBrushFromTexture(nullptr);
		EmoteImage->SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogTemp, Display, TEXT("[Emote] UPUEmoteWidget::ClearEmoteIcon"));
	}
}

void UPUEmoteWidget::PlayFadeIn()
{
	if (FadeUp)
	{
		PlayAnimationForward(FadeUp, 2.0f, false);
	}
}

void UPUEmoteWidget::PlayFadeOut()
{
	if (FadeUp)
	{
		PlayAnimationReverse(FadeUp, 2.0f, false);
	}
}

float UPUEmoteWidget::GetFadeUpDuration() const
{
	if (!FadeUp) return 0.0f;
	UMovieScene* MovieScene = FadeUp->GetMovieScene();
	if (!MovieScene) return 0.5f;
	TRange<FFrameNumber> Range = MovieScene->GetPlaybackRange();
	FFrameRate TickResolution = MovieScene->GetTickResolution();
	const int32 NumFrames = Range.Size<FFrameNumber>().Value;
	return NumFrames > 0 ? (static_cast<float>(NumFrames) / TickResolution.AsDecimal()) : 0.5f;
}

