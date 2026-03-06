#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUDishIconWidget.generated.h"

class UCommonLazyImage;

/**
 * Widget for displaying the plated dish texture above the character's head.
 * Uses Common Lazy Image for consistent styling with the rest of the UI.
 */
UCLASS()
class PROJECTUMEOWMI_API UPUDishIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the dish texture to display. */
	void SetDishTexture(class UTexture2D* Texture);

	/** Clear and hide the dish icon. */
	void ClearDishIcon();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UCommonLazyImage* DishImage;
};
