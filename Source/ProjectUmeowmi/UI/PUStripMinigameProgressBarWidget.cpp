#include "PUStripMinigameProgressBarWidget.h"

#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "PUChopStripMinigameBehavior.h"
#include "PUStripMinigameBehavior.h"
#include "TimerManager.h"

float UPUStripMinigameProgressBarWidget::ComputeEvenTierAnchorPercent(int32 TierIconIndex, int32 TierIconCount)
{
    const int32 Count = FMath::Max(1, TierIconCount);
    const int32 Index = FMath::Clamp(TierIconIndex, 0, Count - 1);
    if (Count <= 1)
    {
        return 0.f;
    }
    return static_cast<float>(Index) / static_cast<float>(Count - 1);
}

void UPUStripMinigameProgressBarWidget::NativeConstruct()
{
    Super::NativeConstruct();
    TierIconCount = FMath::Clamp(MaxTierIcons, 1, 4);
    TierAnchorPercents.SetNum(TierIconCount);
    TierIconStates.SetNum(TierIconCount);
}

void UPUStripMinigameProgressBarWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(LayoutRefreshTimerHandle);
    }
    UnbindFromStripMinigameBehavior();
    Super::NativeDestruct();
}

void UPUStripMinigameProgressBarWidget::BindToStripMinigameBehavior(UPUStripMinigameBehavior* InBehavior)
{
    UnbindFromStripMinigameBehavior();
    BoundBehavior = InBehavior;

    if (UPUChopStripMinigameBehavior* ChopBehavior = Cast<UPUChopStripMinigameBehavior>(BoundBehavior))
    {
        ChopBehavior->OnChopProgressUpdated.AddDynamic(this, &UPUStripMinigameProgressBarWidget::HandleChopProgressUpdated);
        bBoundToChopProgressDelegate = true;
    }

    RefreshFromBoundStripMinigameBehavior();
}

void UPUStripMinigameProgressBarWidget::UnbindFromStripMinigameBehavior()
{
    if (bBoundToChopProgressDelegate)
    {
        if (UPUChopStripMinigameBehavior* ChopBehavior = Cast<UPUChopStripMinigameBehavior>(BoundBehavior))
        {
            ChopBehavior->OnChopProgressUpdated.RemoveDynamic(
                this,
                &UPUStripMinigameProgressBarWidget::HandleChopProgressUpdated);
        }
        bBoundToChopProgressDelegate = false;
    }

    BoundBehavior = nullptr;
}

void UPUStripMinigameProgressBarWidget::RefreshFromBoundStripMinigameBehavior()
{
    SyncVisualsFromBoundBehavior();
}

void UPUStripMinigameProgressBarWidget::SyncVisualsFromBoundBehavior_Implementation()
{
    if (!IsValid(BoundBehavior))
    {
        ApplyProgressVisuals(0.f, 0.f, 0, TArray<float>(), TArray<EPUStripMinigameTierIconState>());
        return;
    }

    if (UPUChopStripMinigameBehavior* ChopBehavior = Cast<UPUChopStripMinigameBehavior>(BoundBehavior))
    {
        const int32 IconCount = FMath::Clamp(
            ChopBehavior->GetProgressBarTierIconCount(),
            1,
            MaxTierIcons);

        TArray<float> Anchors;
        TArray<EPUStripMinigameTierIconState> States;
        Anchors.Reserve(IconCount);
        States.Reserve(IconCount);
        for (int32 Index = 0; Index < IconCount; ++Index)
        {
            Anchors.Add(ChopBehavior->GetChopTierAnchorPercent(Index));
            States.Add(FromChopTierIconState(static_cast<uint8>(ChopBehavior->GetChopTierIconState(Index))));
        }

        const float Overall = ChopBehavior->GetOverallChopProgressNormalized();
        ApplyProgressVisuals(Overall, Overall, IconCount, Anchors, States);
        return;
    }

    ReceiveUnsupportedMinigameBehavior(BoundBehavior);
}

void UPUStripMinigameProgressBarWidget::ApplyProgressVisuals(
    float InFillNormalized,
    float InMarkerNormalized,
    int32 InTierIconCount,
    const TArray<float>& InTierAnchorPercents,
    const TArray<EPUStripMinigameTierIconState>& InTierIconStates)
{
    FillNormalized = FMath::Clamp(InFillNormalized, 0.f, 1.f);
    MarkerNormalized = FMath::Clamp(InMarkerNormalized, 0.f, 1.f);
    TierIconCount = FMath::Clamp(InTierIconCount, 0, MaxTierIcons);
    TierAnchorPercents = InTierAnchorPercents;
    TierIconStates = InTierIconStates;

    if (BarFill)
    {
        BarFill->SetPercent(FillNormalized);
    }

    UpdateCachedTrackWidth();
    ScheduleLayoutRefresh();

    for (int32 Index = 0; Index < TierIconCount; ++Index)
    {
        const EPUStripMinigameTierIconState State =
            TierIconStates.IsValidIndex(Index) ? TierIconStates[Index] : EPUStripMinigameTierIconState::Upcoming;
        ReceiveTierIconStateChanged(Index, State);
    }
}

EPUStripMinigameTierIconState UPUStripMinigameProgressBarWidget::GetTierIconState(int32 TierIconIndex) const
{
    return TierIconStates.IsValidIndex(TierIconIndex) ? TierIconStates[TierIconIndex] : EPUStripMinigameTierIconState::Upcoming;
}

float UPUStripMinigameProgressBarWidget::GetTierAnchorPercent(int32 TierIconIndex) const
{
    return TierAnchorPercents.IsValidIndex(TierIconIndex) ? TierAnchorPercents[TierIconIndex] : 0.f;
}

void UPUStripMinigameProgressBarWidget::HandleChopProgressUpdated(
    EPUChopCompletedCutTier CompletedTier,
    int32 ChopsTowardNextTier,
    int32 ChopsPerTier,
    EPUChopCompletedCutTier TargetTier)
{
    (void)CompletedTier;
    (void)ChopsTowardNextTier;
    (void)ChopsPerTier;
    (void)TargetTier;
    RefreshFromBoundStripMinigameBehavior();
}

void UPUStripMinigameProgressBarWidget::UpdateCachedTrackWidth()
{
    CachedTrackWidth = 0.f;

    auto ReadTrackWidth = [](const UWidget* Widget) -> float
    {
        if (!Widget)
        {
            return 0.f;
        }
        float Width = Widget->GetCachedGeometry().GetLocalSize().X;
        if (Width <= KINDA_SMALL_NUMBER)
        {
            Width = Widget->GetDesiredSize().X;
        }
        return Width;
    };

    CachedTrackWidth = ReadTrackWidth(BarFill);
}

void UPUStripMinigameProgressBarWidget::ScheduleLayoutRefresh()
{
    ApplyLayoutVisuals();

    if (CachedTrackWidth > KINDA_SMALL_NUMBER)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &UPUStripMinigameProgressBarWidget::RefreshTrackLayout));
    }
}

void UPUStripMinigameProgressBarWidget::RefreshTrackLayout()
{
    UpdateCachedTrackWidth();
    ApplyLayoutVisuals();

    if (CachedTrackWidth <= KINDA_SMALL_NUMBER && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            LayoutRefreshTimerHandle,
            this,
            &UPUStripMinigameProgressBarWidget::RefreshTrackLayout,
            0.05f,
            false);
    }
}

void UPUStripMinigameProgressBarWidget::ApplyLayoutVisuals()
{
    for (int32 SlotIndex = 0; SlotIndex < MaxTierIcons; ++SlotIndex)
    {
        UImage* const Icon = GetTierIconWidget(SlotIndex);
        if (!Icon)
        {
            continue;
        }

        if (SlotIndex < TierIconCount)
        {
            Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
            const float Anchor = TierAnchorPercents.IsValidIndex(SlotIndex) ? TierAnchorPercents[SlotIndex] : 0.f;
            PositionWidgetAlongTrack(Icon, Anchor, false);
        }
        else
        {
            Icon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (ProgressMarker)
    {
        if (TierIconCount > 0)
        {
            ProgressMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
            PositionWidgetAlongTrack(ProgressMarker, MarkerNormalized, true);
        }
        else
        {
            ProgressMarker->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UPUStripMinigameProgressBarWidget::PositionWidgetAlongTrack(
    UWidget* Widget,
    float AnchorPercent,
    bool bMarkerAboveTrack) const
{
    if (!Widget || CachedTrackWidth <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const float AnchorX = CachedTrackWidth * FMath::Clamp(AnchorPercent, 0.f, 1.f);

    FVector2D WidgetSize = Widget->GetCachedGeometry().GetLocalSize();
    if (WidgetSize.X <= KINDA_SMALL_NUMBER)
    {
        WidgetSize = Widget->GetDesiredSize();
    }
    if (UImage* const ImageWidget = Cast<UImage>(Widget))
    {
        const FVector2D BrushSize = ImageWidget->GetBrush().GetImageSize();
        if (BrushSize.X > KINDA_SMALL_NUMBER)
        {
            WidgetSize.X = BrushSize.X;
        }
        if (BrushSize.Y > KINDA_SMALL_NUMBER)
        {
            WidgetSize.Y = BrushSize.Y;
        }
    }
    if (WidgetSize.X <= KINDA_SMALL_NUMBER)
    {
        WidgetSize.X = 32.f;
    }
    if (WidgetSize.Y <= KINDA_SMALL_NUMBER)
    {
        WidgetSize.Y = 32.f;
    }

    const float PadLeft = FMath::Max(0.f, AnchorX - WidgetSize.X * 0.5f);

    if (UOverlaySlot* const OverlaySlot = Cast<UOverlaySlot>(Widget->Slot))
    {
        OverlaySlot->SetHorizontalAlignment(HAlign_Left);
        OverlaySlot->SetVerticalAlignment(bMarkerAboveTrack ? VAlign_Top : VAlign_Center);
        const float TopPad = bMarkerAboveTrack ? -WidgetSize.Y : 0.f;
        OverlaySlot->SetPadding(FMargin(PadLeft, TopPad, 0.f, 0.f));
        return;
    }

    Widget->SetRenderTransformPivot(FVector2D(0.5f, bMarkerAboveTrack ? 1.f : 0.5f));
    Widget->SetRenderTranslation(FVector2D(PadLeft, Widget->GetRenderTransform().Translation.Y));
}

UImage* UPUStripMinigameProgressBarWidget::GetTierIconWidget(int32 TierIconIndex) const
{
    switch (TierIconIndex)
    {
    case 0:
        return TierIcon0;
    case 1:
        return TierIcon1;
    case 2:
        return TierIcon2;
    case 3:
        return TierIcon3;
    default:
        return nullptr;
    }
}

EPUStripMinigameTierIconState UPUStripMinigameProgressBarWidget::FromChopTierIconState(uint8 ChopState)
{
    return static_cast<EPUStripMinigameTierIconState>(ChopState);
}
