#include "SDirectionLineWidget.h"
#include "Rendering/DrawElements.h"
#include "Math/UnrealMathUtility.h"

void SDirectionLineWidget::Construct(const FArguments& InArgs)
{
    LineColor = InArgs._LineColor.Get(FLinearColor::White);
    LineThickness = InArgs._LineThickness.Get(2.0f);
    LineLength = InArgs._LineLength.Get(100.0f);
    AngleDegrees = InArgs._AngleDegrees.Get(0.0f);
    bIsVisible = InArgs._bIsVisible.Get(true);
}

int32 SDirectionLineWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!bIsVisible || LineLength <= 0.0f)
    {
        return LayerId;
    }

    // Get the center of the widget
    FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;
    
    // Calculate the end point of the line based on angle and length
    // Convert angle from degrees to radians
    // Note: 0° = right, positive = counter-clockwise (standard math convention)
    float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
    
    // Calculate direction vector (cos for X, sin for Y, but we need to account for Unreal's coordinate system)
    // In Unreal's UI: X increases right, Y increases down
    // So we need: X = cos(angle), Y = -sin(angle) (negative because Y increases down)
    FVector2D Direction(FMath::Cos(AngleRadians), -FMath::Sin(AngleRadians));
    FVector2D EndPoint = Center + (Direction * LineLength);
    
    // Create a brush for the line
    FSlateBrush LineBrush;
    LineBrush.TintColor = FSlateColor(LineColor);
    
    // Draw the line using FSlateDrawElement
    // We'll draw it as a series of line segments or use DrawLines
    TArray<FVector2D> LinePoints;
    LinePoints.Add(Center);
    LinePoints.Add(EndPoint);
    
    TArray<FLinearColor> LineColors;
    LineColors.Add(LineColor);
    LineColors.Add(LineColor);
    
    // Draw the line
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        LinePoints,
        ESlateDrawEffect::None,
        LineColor,
        false, // bAntialias
        LineThickness
    );
    
    return LayerId;
}

FVector2D SDirectionLineWidget::ComputeDesiredSize(float) const
{
    // Return a size that can accommodate the line
    // The line extends from center, so we need at least LineLength * 2 in both dimensions
    float Size = FMath::Max(LineLength * 2.0f, 10.0f);
    return FVector2D(Size, Size);
}

void SDirectionLineWidget::SetLineColor(const FLinearColor& InColor)
{
    LineColor = InColor;
}

void SDirectionLineWidget::SetLineThickness(float InThickness)
{
    LineThickness = InThickness;
}

void SDirectionLineWidget::SetLineLength(float InLength)
{
    LineLength = InLength;
}

void SDirectionLineWidget::SetAngleDegrees(float InAngle)
{
    AngleDegrees = InAngle;
}

void SDirectionLineWidget::SetVisibility(bool bVisible)
{
    bIsVisible = bVisible;
}


