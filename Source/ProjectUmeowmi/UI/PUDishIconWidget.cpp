#include "PUDishIconWidget.h"
#include "CommonLazyImage.h"

void UPUDishIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (DishImage)
	{
		DishImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPUDishIconWidget::SetDishTexture(UTexture2D* Texture)
{
	if (DishImage && Texture)
	{
		DishImage->SetBrushFromTexture(Texture);
		DishImage->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPUDishIconWidget::ClearDishIcon()
{
	if (DishImage)
	{
		DishImage->SetBrushFromTexture(nullptr);
		DishImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}
