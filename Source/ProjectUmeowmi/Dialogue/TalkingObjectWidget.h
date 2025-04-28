#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TalkingObjectWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * Widget class for displaying interaction UI above TalkingObjects
 */
UCLASS()
class PROJECTUMEOWMI_API UTalkingObjectWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Sets the interaction key text */
    void SetInteractionKey(const FString& Key);

    /** Sets the interaction text */
    void SetInteractionText(const FString& Text);

    /** Sets the interaction icon */
    void SetInteractionIcon(UTexture2D* Icon);

protected:
    virtual void NativeConstruct() override;

    /** The text block displaying the interaction key */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* InteractionKeyText;

    /** The text block displaying the interaction description */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* InteractionDescriptionText;

    /** The image displaying the interaction icon */
    UPROPERTY(meta = (BindWidget))
    UImage* InteractionIcon;
}; 