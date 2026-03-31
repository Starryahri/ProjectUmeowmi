#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUVirtualCursorUserWidget.generated.h"

/**
 * Optional base for the virtual cursor widget. Reparent your cursor widget from UUserWidget to this class,
 * then override OnVirtualCursorInteractVisual in Blueprint (e.g. swap image or color on press vs release).
 */
UCLASS(Abstract, Blueprintable)
class PROJECTUMEOWMI_API UPUVirtualCursorUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called when Interact press starts (MouseClick Started) and when it ends (MouseClick Completed). */
	UFUNCTION(BlueprintNativeEvent, Category = "Virtual Cursor")
	void OnVirtualCursorInteractVisual(bool bPressed);

	virtual void OnVirtualCursorInteractVisual_Implementation(bool bPressed);
};
