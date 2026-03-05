#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUEmoteWidget.generated.h"

class UImage;
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

protected:
	virtual void NativeConstruct() override;

	/** The image displaying the emote icon. */
	UPROPERTY(meta = (BindWidget))
	UImage* EmoteImage;
};

