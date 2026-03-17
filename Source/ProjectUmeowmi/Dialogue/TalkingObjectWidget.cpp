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

void UTalkingObjectWidget::SetInteractionIcon(UTexture2D* Icon)
{
    if (InteractionIcon)
    {
        if (Icon)
        {
            InteractionIcon->SetBrushFromTexture(Icon);
            InteractionIcon->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            InteractionIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UTalkingObjectWidget::SetSelectionState(bool bSelected, int32 Total)
{
    // Dim unselected targets when multiple overlap
    SetRenderOpacity(bSelected ? 1.0f : 0.5f);
} 