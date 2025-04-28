#include "TalkingObjectWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UTalkingObjectWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UTalkingObjectWidget::SetInteractionKey(const FString& Key)
{
    if (InteractionKeyText)
    {
        InteractionKeyText->SetText(FText::FromString(Key));
    }
}

void UTalkingObjectWidget::SetInteractionText(const FString& Text)
{
    if (InteractionDescriptionText)
    {
        InteractionDescriptionText->SetText(FText::FromString(Text));
    }
}

void UTalkingObjectWidget::SetInteractionIcon(UTexture2D* Icon)
{
    if (InteractionIcon && Icon)
    {
        InteractionIcon->SetBrushFromTexture(Icon);
    }
} 