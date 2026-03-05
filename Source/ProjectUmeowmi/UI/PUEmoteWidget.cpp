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

