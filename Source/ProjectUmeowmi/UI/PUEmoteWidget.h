#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUEmoteWidget.generated.h"

class UImage;
class UTexture2D;

/**
 * Simple widget for displaying a single emote icon above a character's head.
 * Designed to be driven from C++ or Blueprint via ShowEmoteByTag on actors.
 */
UCLASS()
class PROJECTUMEOWMI_API UPUEmoteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Set the emote icon texture. Passing nullptr leaves the current icon unchanged. */
	UFUNCTION(BlueprintCallable, Category = "Emote")
	void SetEmoteIcon(UTexture2D* Icon);

	/** Clear the emote icon and hide the image (widget visibility is controlled by the owning actor). */
	UFUNCTION(BlueprintCallable, Category = "Emote")
	void ClearEmoteIcon();

protected:
	/** Image widget that displays the emote icon. */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* EmoteImage;
};

