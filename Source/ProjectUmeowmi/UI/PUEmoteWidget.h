#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUEmoteWidget.generated.h"

class UImage;
class UWidgetAnimation;
class UTexture2D;

/**
 * Widget class for displaying an emote icon above a character's head.
 * Mirrors the pattern of UTalkingObjectWidget (component + widget class on actor, single image).
 */
UCLASS()
class PROJECTUMEOWMI_API UPUEmoteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the emote icon texture. */
	void SetEmoteIcon(UTexture2D* Icon);

	/** Clear the emote icon and hide the image. */
	void ClearEmoteIcon();

	/** Play the FadeUp animation forward (fade in) if it exists. */
	void PlayFadeIn();

	/** Play the FadeUp animation in reverse (fade out) if it exists. */
	void PlayFadeOut();

	/** Get the length of the FadeUp animation in seconds. Returns default 0.25f if animation exists, 0 if missing. */
	float GetFadeUpDuration() const;

protected:
	virtual void NativeConstruct() override;

	/** The image displaying the emote icon. */
	UPROPERTY(meta = (BindWidget))
	UImage* EmoteImage;

	/** Optional fade animation for emotes. Bind to an animation named FadeUp on the widget Blueprint. */
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	UWidgetAnimation* FadeUp;
};

