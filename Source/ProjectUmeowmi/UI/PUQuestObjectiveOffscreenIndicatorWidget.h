#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUQuestObjectiveOffscreenIndicatorWidget.generated.h"

class UCanvasPanel;
class UImage;

/**
 * Full-screen overlay that draws the active quest objective marker at the viewport edge when the target is off-screen.
 * Hides the world-space QuestMarkerWidget on the matching TalkingObject while the edge marker is shown.
 */
UCLASS()
class PROJECTUMEOWMI_API UPUQuestObjectiveOffscreenIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPUQuestObjectiveOffscreenIndicatorWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Inset from each viewport edge when clamping (game viewport pixels; matches ProjectWorldLocationToScreen). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Markers")
	float ScreenEdgeMargin = 48.f;

	/** Size of the edge marker icon (layout pixels). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Markers")
	FVector2D MarkerDrawSize = FVector2D(64.f, 64.f);

private:
	void EnsureWidgetTreeBuilt();
	void UpdateIndicator(const FGeometry& MyGeometry);

	UPROPERTY()
	UCanvasPanel* RootCanvas = nullptr;

	UPROPERTY()
	UImage* MarkerImage = nullptr;
};
