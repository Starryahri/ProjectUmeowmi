#include "PUQuestObjectiveOffscreenIndicatorWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"
#include "SceneView.h"
#include "ProjectUmeowmi/Dialogue/TalkingObject.h"
#include "ProjectUmeowmi/Quest/PUQuestObjectiveContentRow.h"
#include "ProjectUmeowmi/Quest/PUQuestSubsystem.h"
#include "Styling/SlateBrush.h"

namespace
{
/** World-space view basis from the same ViewRotationMatrix as ProjectWorldLocationToScreen (InvView rotation = ViewRotationMatrix^T). */
void GetViewAxesFromProjectionData(const FSceneViewProjectionData& PD, FVector& OutOrigin, FVector& OutForward, FVector& OutRight, FVector& OutUp)
{
	OutOrigin = PD.ViewOrigin;
	OutForward = PD.ViewRotationMatrix.GetColumn(2).GetSafeNormal();
	const FMatrix InvRot = PD.ViewRotationMatrix.GetTransposed();
	OutRight = FVector(InvRot.GetColumn(0)).GetSafeNormal();
	OutUp = FVector(InvRot.GetColumn(1)).GetSafeNormal();
}

/** Constrained view rect + projection data (public API — matches screen projection). */
bool TryGetPlayerProjectionData(APlayerController* PC, FSceneViewProjectionData& OutProjectionData, FIntRect& OutConstrainedRect)
{
	if (ULocalPlayer* const LP = PC ? PC->GetLocalPlayer() : nullptr)
	{
		if (LP->ViewportClient)
		{
			if (LP->GetProjectionData(LP->ViewportClient->Viewport, OutProjectionData))
			{
				OutConstrainedRect = OutProjectionData.GetConstrainedViewRect();
				return OutConstrainedRect.Width() > 0 && OutConstrainedRect.Height() > 0;
			}
		}
	}
	return false;
}

/**
 * Viewport pixel size for the local player — matches the coordinate space of ProjectWorldLocationToScreen(..., bPlayerViewportRelative=true).
 * Falls back to game viewport or widget geometry when needed (e.g. first frame).
 */
bool GetPlayerViewportPixelSize(APlayerController* PC, const FGeometry& WidgetGeometry, FVector2D& OutViewportPixels)
{
	if (PC)
	{
		int32 SX = 0;
		int32 SY = 0;
		PC->GetViewportSize(SX, SY);
		if (SX > 0 && SY > 0)
		{
			OutViewportPixels = FVector2D(static_cast<float>(SX), static_cast<float>(SY));
			return true;
		}
	}
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(OutViewportPixels);
		if (OutViewportPixels.X > 1.f && OutViewportPixels.Y > 1.f)
		{
			return true;
		}
	}
	const FVector2D Local = WidgetGeometry.GetLocalSize();
	if (Local.X > 1.f && Local.Y > 1.f)
	{
		OutViewportPixels = Local;
		return true;
	}
	return false;
}

/** Converts a delta in viewport pixel space to this widget's local layout space (handles DPI / stretch vs game viewport). */
FVector2D PixelDeltaToLocalDelta(const FVector2D& PixelDelta, const FVector2D& ViewportPixels, const FVector2D& LocalWidgetSize)
{
	if (ViewportPixels.X < KINDA_SMALL_NUMBER || ViewportPixels.Y < KINDA_SMALL_NUMBER)
	{
		return PixelDelta;
	}
	return FVector2D(
		PixelDelta.X * (LocalWidgetSize.X / ViewportPixels.X),
		PixelDelta.Y * (LocalWidgetSize.Y / ViewportPixels.Y));
}

FVector2D ComputeClampedEdgePosition(FVector2D ScreenPos, const FVector2D& ViewportSize, float Margin)
{
	const FVector2D Center = ViewportSize * 0.5f;
	FVector2D Dir = ScreenPos - Center;
	const float MinX = Margin;
	const float MinY = Margin;
	const float MaxX = ViewportSize.X - Margin;
	const float MaxY = ViewportSize.Y - Margin;

	if (Dir.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return FVector2D(MinX, Center.Y);
	}

	Dir.Normalize();
	float BestT = TNumericLimits<float>::Max();

	auto TryVertical = [&](float EdgeX)
	{
		if (FMath::Abs(Dir.X) < KINDA_SMALL_NUMBER)
		{
			return;
		}
		const float T = (EdgeX - Center.X) / Dir.X;
		if (T <= 0.f)
		{
			return;
		}
		const float Y = Center.Y + Dir.Y * T;
		if (Y >= MinY - KINDA_SMALL_NUMBER && Y <= MaxY + KINDA_SMALL_NUMBER)
		{
			BestT = FMath::Min(BestT, T);
		}
	};

	auto TryHorizontal = [&](float EdgeY)
	{
		if (FMath::Abs(Dir.Y) < KINDA_SMALL_NUMBER)
		{
			return;
		}
		const float T = (EdgeY - Center.Y) / Dir.Y;
		if (T <= 0.f)
		{
			return;
		}
		const float X = Center.X + Dir.X * T;
		if (X >= MinX - KINDA_SMALL_NUMBER && X <= MaxX + KINDA_SMALL_NUMBER)
		{
			BestT = FMath::Min(BestT, T);
		}
	};

	TryVertical(MinX);
	TryVertical(MaxX);
	TryHorizontal(MinY);
	TryHorizontal(MaxY);

	if (BestT >= TNumericLimits<float>::Max() * 0.5f)
	{
		return FVector2D(MinX, Center.Y);
	}

	return Center + Dir * BestT;
}

bool IsInFrontOfCamera(const FVector& WorldLocation, const FVector& ViewOrigin, const FVector& ViewForward)
{
	const FVector ToTarget = (WorldLocation - ViewOrigin).GetSafeNormal();
	return FVector::DotProduct(ViewForward, ToTarget) >= 0.f;
}

/** Fills OutScreenPos for clamping (may be outside viewport). ViewportSize = constrained view size (same space as ProjectWorldLocationToScreen). */
bool GetScreenPositionForObjective(APlayerController* PC, const FVector& WorldLocation, const FVector2D& ViewportSize, const FVector& ViewOrigin, const FVector& ViewForward, const FVector& ViewRight, const FVector& ViewUp, FVector2D& OutScreenPos)
{
	if (!PC || ViewportSize.X < 1.f || ViewportSize.Y < 1.f)
	{
		return false;
	}
	const FVector Delta = WorldLocation - ViewOrigin;
	const float ForwardDotAlong = FVector::DotProduct(ViewForward, Delta);

	const FVector2D Center = ViewportSize * 0.5f;

	// In front of the rendering camera: use engine projection (authoritative for screen X/Y).
	if (ForwardDotAlong >= 0.f && PC->ProjectWorldLocationToScreen(WorldLocation, OutScreenPos, true))
	{
		return true;
	}

	// Behind camera: mirror world position across the view plane (through camera, perpendicular to forward) so projection matches screen edge convention.
	if (ForwardDotAlong < 0.f)
	{
		const FVector MirroredWorld = WorldLocation - 2.f * ForwardDotAlong * ViewForward;
		if (PC->ProjectWorldLocationToScreen(MirroredWorld, OutScreenPos, true))
		{
			return true;
		}
	}

	// Fallback: bearing in camera space (Y negated so +Y screen = down, matching viewport).
	const FVector ToNorm = Delta.GetSafeNormal();
	float ScreenX = FVector::DotProduct(ToNorm, ViewRight);
	float ScreenY = -FVector::DotProduct(ToNorm, ViewUp);
	if (ForwardDotAlong < 0.f)
	{
		ScreenX = -ScreenX;
		ScreenY = -ScreenY;
	}

	const FVector2D Dir2D(ScreenX, ScreenY);
	if (Dir2D.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		OutScreenPos = FVector2D(Center.X, 0.f);
		return true;
	}
	OutScreenPos = Center + Dir2D.GetSafeNormal() * FMath::Max(ViewportSize.X, ViewportSize.Y);
	return true;
}

bool IsObjectiveOnScreen(const FVector2D& ScreenPos, const FVector2D& ViewportSize, float Margin, bool bInFront)
{
	if (!bInFront)
	{
		return false;
	}
	return ScreenPos.X >= Margin && ScreenPos.X <= ViewportSize.X - Margin && ScreenPos.Y >= Margin && ScreenPos.Y <= ViewportSize.Y - Margin;
}

UTexture2D* GetFallbackWhiteTexture()
{
	static UTexture2D* Cached = nullptr;
	if (!Cached)
	{
		Cached = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	}
	return Cached;
}
} // namespace

UPUQuestObjectiveOffscreenIndicatorWidget::UPUQuestObjectiveOffscreenIndicatorWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPUQuestObjectiveOffscreenIndicatorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureWidgetTreeBuilt();
}

void UPUQuestObjectiveOffscreenIndicatorWidget::EnsureWidgetTreeBuilt()
{
	if (MarkerImage && RootCanvas)
	{
		return;
	}
	UWidgetTree* Tree = WidgetTree;
	if (!Tree)
	{
		return;
	}
	if (!RootCanvas)
	{
		RootCanvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		Tree->RootWidget = RootCanvas;
		RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (!MarkerImage)
	{
		MarkerImage = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		MarkerImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* const MarkerCanvasSlot = RootCanvas->AddChildToCanvas(MarkerImage);
		MarkerCanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		MarkerCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		MarkerCanvasSlot->SetAutoSize(true);
	}
}

void UPUQuestObjectiveOffscreenIndicatorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!MarkerImage || !RootCanvas)
	{
		EnsureWidgetTreeBuilt();
		if (!MarkerImage)
		{
			return;
		}
	}
	UpdateIndicator(MyGeometry);
}

void UPUQuestObjectiveOffscreenIndicatorWidget::UpdateIndicator(const FGeometry& MyGeometry)
{
	// Never Collapse the root UserWidget: Slate stops ticking collapsed widgets, so we would not detect going off-screen again.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	auto HideEdgeMarker = [this]()
	{
		if (MarkerImage)
		{
			MarkerImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	};

	UWorld* World = GetWorld();
	if (!World)
	{
		HideEdgeMarker();
		return;
	}
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = World->GetFirstPlayerController();
	}
	if (!PC)
	{
		HideEdgeMarker();
		return;
	}

	UGameInstance* GI = PC->GetGameInstance();
	UPUQuestSubsystem* Quest = GI ? GI->GetSubsystem<UPUQuestSubsystem>() : nullptr;
	if (!Quest)
	{
		HideEdgeMarker();
		return;
	}

	const FGameplayTag ActiveObjective = Quest->GetActiveObjectiveTag();
	if (!ActiveObjective.IsValid())
	{
		HideEdgeMarker();
		return;
	}

	ATalkingObject* Target = nullptr;
	for (TActorIterator<ATalkingObject> It(World); It; ++It)
	{
		ATalkingObject* T = *It;
		if (!T || !T->bEnableQuestMarker)
		{
			continue;
		}
		T->MigrateDeprecatedQuestObjectiveTagIfNeeded();
		if (ActiveObjective.IsValid() && T->QuestObjectiveTags.HasTagExact(ActiveObjective))
		{
			Target = T;
			break;
		}
	}

	if (!Target || !Target->QuestMarkerWidget)
	{
		HideEdgeMarker();
		return;
	}

	const FVector WorldLoc = Target->QuestMarkerWidget->GetComponentLocation();

	FSceneViewProjectionData ProjectionData;
	FIntRect ConstrainedRect(0, 0, 0, 0);
	FVector ViewOrigin;
	FVector ViewForward;
	FVector ViewRight;
	FVector ViewUp;

	if (TryGetPlayerProjectionData(PC, ProjectionData, ConstrainedRect))
	{
		GetViewAxesFromProjectionData(ProjectionData, ViewOrigin, ViewForward, ViewRight, ViewUp);
	}
	else
	{
		FRotator CamRot;
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->GetCameraViewPoint(ViewOrigin, CamRot);
		}
		else
		{
			PC->GetPlayerViewPoint(ViewOrigin, CamRot);
		}
		ViewForward = CamRot.Vector();
		ViewRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
		ViewUp = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Z);
		int32 SX = 0;
		int32 SY = 0;
		PC->GetViewportSize(SX, SY);
		ConstrainedRect = FIntRect(0, 0, FMath::Max(1, SX), FMath::Max(1, SY));
	}

	FVector2D ConstrainedPixels(static_cast<float>(ConstrainedRect.Width()), static_cast<float>(ConstrainedRect.Height()));
	if (ConstrainedPixels.X < 1.f || ConstrainedPixels.Y < 1.f)
	{
		if (!GetPlayerViewportPixelSize(PC, MyGeometry, ConstrainedPixels))
		{
			HideEdgeMarker();
			return;
		}
		ConstrainedRect = FIntRect(0, 0, FMath::RoundToInt(ConstrainedPixels.X), FMath::RoundToInt(ConstrainedPixels.Y));
	}

	const bool bInFront = IsInFrontOfCamera(WorldLoc, ViewOrigin, ViewForward);

	FVector2D ScreenPos;
	if (!GetScreenPositionForObjective(PC, WorldLoc, ConstrainedPixels, ViewOrigin, ViewForward, ViewRight, ViewUp, ScreenPos))
	{
		HideEdgeMarker();
		return;
	}

	/** Full viewport (window) for placing a fullscreen HUD widget when the game view is letterboxed inside it. */
	int32 FullViewportX = 0;
	int32 FullViewportY = 0;
	PC->GetViewportSize(FullViewportX, FullViewportY);
	const FVector2D FullViewportPixels(FMath::Max(1, FullViewportX), FMath::Max(1, FullViewportY));
	const FVector2D FullViewportCenter = FullViewportPixels * 0.5f;

	const bool bOnScreen = IsObjectiveOnScreen(ScreenPos, ConstrainedPixels, ScreenEdgeMargin, bInFront);

	UWidgetComponent* const QuestMarker = Target->QuestMarkerWidget;

	if (bOnScreen)
	{
		HideEdgeMarker();
		QuestMarker->SetVisibility(true);
		return;
	}

	if (MarkerImage)
	{
		MarkerImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// Hide world-space marker while edge indicator is active (avoid duplicate icons).
	QuestMarker->SetVisibility(false);

	const FVector2D ClampedConstrained = ComputeClampedEdgePosition(ScreenPos, ConstrainedPixels, ScreenEdgeMargin);

	// Offset from fullscreen widget center: constrained rect may be letterboxed inside the viewport.
	const FVector2D PixelDeltaFromCenter = FVector2D(ConstrainedRect.Min) + ClampedConstrained - FullViewportCenter;

	// Canvas slot uses local Slate units; map from viewport pixels so layout matches resolution and DPI.
	FVector2D LocalWidgetSize = MyGeometry.GetLocalSize();
	if (LocalWidgetSize.X < 1.f || LocalWidgetSize.Y < 1.f)
	{
		LocalWidgetSize = FullViewportPixels;
	}
	const FVector2D LocalOffset = PixelDeltaToLocalDelta(PixelDeltaFromCenter, FullViewportPixels, LocalWidgetSize);

	if (UCanvasPanelSlot* const MarkerCanvasSlot = Cast<UCanvasPanelSlot>(MarkerImage->Slot))
	{
		MarkerCanvasSlot->SetPosition(LocalOffset);
	}

	MarkerImage->SetDesiredSizeOverride(MarkerDrawSize);
	MarkerImage->SetRenderTransformAngle(0.f);

	UTexture2D* IconTex = nullptr;
	if (UPUQuestSubsystem* QuestSys = GI->GetSubsystem<UPUQuestSubsystem>())
	{
		FPUQuestObjectiveDisplayInfo Info;
		if (QuestSys->GetObjectiveDisplayInfo(ActiveObjective, Info) && Info.Icon)
		{
			IconTex = Info.Icon;
		}
	}

	if (!IconTex)
	{
		IconTex = GetFallbackWhiteTexture();
	}

	if (IconTex)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(IconTex);
		Brush.ImageSize = MarkerDrawSize;
		MarkerImage->SetBrush(Brush);
	}
}
