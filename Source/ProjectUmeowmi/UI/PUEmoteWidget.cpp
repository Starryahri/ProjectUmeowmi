#include "ProjectUmeowmi/UI/PUEmoteWidget.h"

#include "Components/Image.h"

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
	}
}

void UPUEmoteWidget::ClearEmoteIcon()
{
	if (EmoteImage)
	{
		EmoteImage->SetBrushFromTexture(nullptr);
		EmoteImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

