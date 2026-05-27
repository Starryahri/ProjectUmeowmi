#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUChopStripMinigameBehavior.h"
#include "PUStripMinigameProgressBarWidget.generated.h"

class UImage;
class UProgressBar;
class USizeBox;
class UPUStripMinigameBehavior;

/** Tier pip state on the shared strip minigame progress bar (chop, future marinate, etc.). */
UENUM(BlueprintType)
enum class EPUStripMinigameTierIconState : uint8
{
    Upcoming    UMETA(DisplayName = "Upcoming"),
    InProgress  UMETA(DisplayName = "In Progress"),
    Completed   UMETA(DisplayName = "Completed")
};

/**
 * Reusable strip minigame progress bar (parent Blueprint: WBP_ProgressBar).
 * C++ drives UProgressBar fill percent, marker, tier-icon X positions, and per-state icon tint.
 * Place BarFill (Progress Bar), tier icons, and ProgressMarker in one Overlay (left-aligned).
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Strip Minigame Progress Bar"))
class PROJECTUMEOWMI_API UPUStripMinigameProgressBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Strip Minigame Progress Bar")
    void BindToStripMinigameBehavior(UPUStripMinigameBehavior* InBehavior);

    UFUNCTION(BlueprintCallable, Category = "Strip Minigame Progress Bar")
    void UnbindFromStripMinigameBehavior();

    void SanitizeBoundBehaviorReference();

    static void SanitizeAllLiveStripMinigameProgressBars();

    UFUNCTION(BlueprintCallable, Category = "Strip Minigame Progress Bar")
    void RefreshFromBoundStripMinigameBehavior();

    UFUNCTION(BlueprintPure, Category = "Strip Minigame Progress Bar")
    float GetFillNormalized() const { return FillNormalized; }

    UFUNCTION(BlueprintPure, Category = "Strip Minigame Progress Bar")
    float GetMarkerNormalized() const { return MarkerNormalized; }

    UFUNCTION(BlueprintPure, Category = "Strip Minigame Progress Bar")
    int32 GetTierIconCount() const { return TierIconCount; }

    UFUNCTION(BlueprintPure, Category = "Strip Minigame Progress Bar")
    EPUStripMinigameTierIconState GetTierIconState(int32 TierIconIndex) const;

    UFUNCTION(BlueprintPure, Category = "Strip Minigame Progress Bar")
    float GetTierAnchorPercent(int32 TierIconIndex) const;

    /** Even spacing for N tier icons along the bar (0 .. 1). */
    UFUNCTION(BlueprintPure, Category = "Strip Minigame Progress Bar")
    static float ComputeEvenTierAnchorPercent(int32 TierIconIndex, int32 TierIconCount);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Pulls fill/marker/tier data from the bound behavior (chop today; override for new minigames). */
    UFUNCTION(BlueprintNativeEvent, Category = "Strip Minigame Progress Bar")
    void SyncVisualsFromBoundBehavior();
    virtual void SyncVisualsFromBoundBehavior_Implementation();

    UFUNCTION(BlueprintCallable, Category = "Strip Minigame Progress Bar")
    void ApplyProgressVisuals(
        float InFillNormalized,
        float InMarkerNormalized,
        int32 InTierIconCount,
        const TArray<float>& TierAnchorPercents,
        const TArray<EPUStripMinigameTierIconState>& TierIconStates);

    UFUNCTION(BlueprintNativeEvent, Category = "Strip Minigame Progress Bar", meta = (DisplayName = "On Tier Icon State Changed"))
    void ReceiveTierIconStateChanged(int32 TierIconIndex, EPUStripMinigameTierIconState IconState);
    virtual void ReceiveTierIconStateChanged_Implementation(int32 TierIconIndex, EPUStripMinigameTierIconState IconState);

    UFUNCTION(BlueprintImplementableEvent, Category = "Strip Minigame Progress Bar", meta = (DisplayName = "On Unsupported Minigame Behavior"))
    void ReceiveUnsupportedMinigameBehavior(UPUStripMinigameBehavior* Behavior);

    /** Fill + track via Percent (0–1). Widget type must be Progress Bar; name in UMG: BarFill. */
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Widgets")
    TObjectPtr<UProgressBar> BarFill;

    /** Yellow triangle; translated along the track. */
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Widgets")
    TObjectPtr<UImage> ProgressMarker;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Widgets")
    TObjectPtr<UImage> TierIcon0;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Widgets")
    TObjectPtr<UImage> TierIcon1;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Widgets")
    TObjectPtr<UImage> TierIcon2;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Widgets")
    TObjectPtr<UImage> TierIcon3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Layout", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxTierIcons = 4;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Appearance")
    FLinearColor CompletedTierIconTint = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Appearance")
    FLinearColor InProgressTierIconTint = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Strip Minigame Progress Bar|Appearance")
    FLinearColor UpcomingTierIconTint = FLinearColor::White;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Strip Minigame Progress Bar")
    TObjectPtr<UPUStripMinigameBehavior> BoundBehavior;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Strip Minigame Progress Bar")
    float FillNormalized = 0.f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Strip Minigame Progress Bar")
    float MarkerNormalized = 0.f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Strip Minigame Progress Bar")
    int32 TierIconCount = 0;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Strip Minigame Progress Bar")
    TArray<float> TierAnchorPercents;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Strip Minigame Progress Bar")
    TArray<EPUStripMinigameTierIconState> TierIconStates;

private:
    UFUNCTION()
    void HandleChopProgressUpdated(
        EPUChopCompletedCutTier CompletedTier,
        int32 ChopsTowardNextTier,
        int32 ChopsPerTier,
        EPUChopCompletedCutTier TargetTier);

    UFUNCTION()
    void HandleMarinateProgressUpdated(int32 MixStrokesCompleted, int32 MixStrokesRequired);

    void ApplyLayoutVisuals();
    void UpdateCachedTrackWidth();
    void ScheduleLayoutRefresh();
    void RefreshTrackLayout();
    void PositionWidgetAlongTrack(UWidget* Widget, float AnchorPercent, bool bMarkerAboveTrack) const;
    UImage* GetTierIconWidget(int32 TierIconIndex) const;
    FLinearColor GetTierIconTintForState(EPUStripMinigameTierIconState IconState) const;
    static EPUStripMinigameTierIconState FromChopTierIconState(uint8 ChopState);

    float CachedTrackWidth = 0.f;
    bool bBoundToChopProgressDelegate = false;
    bool bBoundToMarinateProgressDelegate = false;
    FTimerHandle LayoutRefreshTimerHandle;
};
