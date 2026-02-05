#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Rendering/DrawElements.h"

/**
 * Custom Slate widget that draws a line from center to a target direction
 * Similar to Unity's Debug.DrawLine or radar graph line rendering
 */
class SDirectionLineWidget : public SWidget
{
public:
    SLATE_BEGIN_ARGS(SDirectionLineWidget)
        : _LineColor(FLinearColor::White)
        , _LineThickness(2.0f)
        , _LineLength(100.0f)
        , _AngleDegrees(0.0f)
        , _bIsVisible(true)
    {}
        SLATE_ATTRIBUTE(FLinearColor, LineColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ATTRIBUTE(float, LineLength)
        SLATE_ATTRIBUTE(float, AngleDegrees)
        SLATE_ATTRIBUTE(bool, bIsVisible)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // SWidget interface
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
                         const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
                         int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
    virtual FVector2D ComputeDesiredSize(float) const override;

    // Update the line properties
    void SetLineColor(const FLinearColor& InColor);
    void SetLineThickness(float InThickness);
    void SetLineLength(float InLength);
    void SetAngleDegrees(float InAngle);
    void SetVisibility(bool bVisible);

private:
    FLinearColor LineColor;
    float LineThickness;
    float LineLength;
    float AngleDegrees;
    bool bIsVisible;
};

