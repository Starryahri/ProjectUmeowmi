#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "PUCommonUserWidget.h"
#include "PUAspectProfileWidget.h"
#include "PUScorecardTypes.h"
#include "PUScorecardWidget.generated.h"

/**
 * Dish scoring (3D capture / frame) — lowest Z. Must be below the scoring dialogue or the
 * UMG layer draws on top and you hear typewriter (logic + sound) but do not see the line.
 * SelfHitTestInvisible does not change paint order.
 */
inline constexpr int32 PUScoringSceneViewportZOrder = 50000;
/** Scoring dialogue box — above dish scene so text and typewriter are visible. */
inline constexpr int32 PUScoringDialogueViewportZOrder = PUScoringSceneViewportZOrder + 1;
/** Scorecard — above dialogue when shown (modal). */
inline constexpr int32 PUScorecardViewportZOrder = PUScoringDialogueViewportZOrder + 1;

/**
 * Dish customization UMG — above the entire scoring stack (50000+) so stray scoring/scorecard widgets cannot steal
 * virtual-cursor hover (they sit at Z 50000+ by design; dish was previously ~250 and was underneath).
 */
inline constexpr int32 PUDishCustomizationViewportZOrder = PUScorecardViewportZOrder + 100;
/** On-screen pointer during dish customization — above dish panel. */
inline constexpr int32 PUDishVirtualCursorViewportZOrder = PUDishCustomizationViewportZOrder + 50;
/** Journal AddToViewport Z when opened while dish customization is active (must beat dish + cursor). */
inline constexpr int32 PUJournalViewportZOrderDuringDishCustomization = PUDishVirtualCursorViewportZOrder + 25;
/** Radial menu when added directly to viewport — just above the virtual cursor. */
inline constexpr int32 PURadialMenuViewportZOrder = PUDishVirtualCursorViewportZOrder + 20;
/** Default popup Z for normal gameplay (no dish customization). */
inline constexpr int32 PUPopupViewportZOrder = 1000;
/** Dialogue during dish customization — above journal/cursor/dish panel. */
inline constexpr int32 PUDialogueViewportZOrderDuringDishCustomization = PUJournalViewportZOrderDuringDishCustomization + 25;
/** Popup during dish customization — above dialogue so tutorial modals stay on top. */
inline constexpr int32 PUPopupViewportZOrderDuringDishCustomization = PUDialogueViewportZOrderDuringDishCustomization + 25;

class UWorld;

/** Viewport Z for dialogue: scoring stack Z normally, above dish customization when that UI is open. */
PROJECTUMEOWMI_API int32 PUResolveDialogueViewportZOrder(const UWorld* World);

/** Viewport Z for popups: default Z normally, above dish customization when that UI is open. */
PROJECTUMEOWMI_API int32 PUResolvePopupViewportZOrder(const UWorld* World);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScorecardClosed);

class UImage;
class UTextBlock;
class UVerticalBox;
class UPanelWidget;
class UMaterialInterface;
class UMaterialInstanceDynamic;
struct FPUOrderBase;

/**
 * Scorecard widget displayed when completing/delivering an order to a dish giver.
 * Shows: dish capture, seal of approval (3 tiers), base ingredients, flavor profile, texture profile.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUScorecardWidget : public UPUCommonUserWidget
{
	GENERATED_BODY()

public:
	UPUScorecardWidget(const FObjectInitializer& ObjectInitializer);

	/** Viewport Z for the dish scoring widget (lowest among scoring layers). */
	UFUNCTION(BlueprintPure, Category = "UI|Dish Scoring")
	static int32 GetDishScoringSceneViewportZOrder() { return PUScoringSceneViewportZOrder; }

	/** Viewport Z for the scoring dialogue box (above dish scene). */
	UFUNCTION(BlueprintPure, Category = "UI|Dish Scoring")
	static int32 GetScoringDialogueViewportZOrder() { return PUScoringDialogueViewportZOrder; }

	/** Viewport Z for the scorecard (top of scoring stack). */
	UFUNCTION(BlueprintPure, Category = "UI|Dish Scoring")
	static int32 GetScorecardLayerViewportZOrder() { return PUScorecardViewportZOrder; }

	/** Add to viewport at GetScorecardLayerViewportZOrder(). */
	UFUNCTION(BlueprintCallable, Category = "UI|Dish Scoring")
	void AddToViewportScoringStack();

	/**
	 * Set scorecard data from a completed order. Call when displaying the scorecard after order delivery.
	 * @param InData - Scorecard data from UPUDishBlueprintLibrary::GetScorecardData(Order)
	 */
	UFUNCTION(BlueprintCallable, Category = "Scorecard")
	void SetScorecardData(const FPUScorecardData& InData);

	/**
	 * Set the dish image (scene capture render target, baked texture, or preview). Pass nullptr to use CompletedDish.PreviewTexture from order.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scorecard")
	void SetDishImage(UTexture* DishTexture);

	/**
	 * Show the scorecard with optional dish texture (e.g. scene capture RT for material "DishRender"). If null, uses dish preview from data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scorecard")
	void ShowFromOrder(const FPUOrderBase& Order, UTexture* OptionalDishTexture = nullptr);

	/** Play the seal animation (same for all tiers - seal image changes based on tier). */
	UFUNCTION(BlueprintCallable, Category = "Scorecard")
	void PlaySealAnimation();

	/** Close/dismiss the scorecard */
	UFUNCTION(BlueprintCallable, Category = "Scorecard")
	void Close();

	/** Called when the scorecard is closed (e.g. by user or animation complete) */
	UPROPERTY(BlueprintAssignable, Category = "Scorecard")
	FOnScorecardClosed OnScorecardClosed;

protected:
	virtual void NativeConstruct() override;

	/** Dish name. Bind a Text Block named exactly "DishNameText". */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UTextBlock> DishNameText;

	/** Dish capture/preview image */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UImage> DishImage;

	/** Seal of approval image - set texture based on tier, animate in */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UImage> SealImage;

	/** Container for base ingredient icons - bind any panel (HorizontalBox, VerticalBox, WrapBox, etc.) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UPanelWidget> BaseIngredientsContainer;

	/** Container for flavor profile - bind any panel (HorizontalBox, VerticalBox, WrapBox, etc.) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UPanelWidget> FlavorProfileContainer;

	/** Container for texture profile - bind any panel (HorizontalBox, VerticalBox, WrapBox, etc.) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UPanelWidget> TextureProfileContainer;

	/** Widget class for flavor/texture profile. Assign WBP_AspectProfile here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard", meta = (AllowAbstract = "false"))
	TSubclassOf<UPUAspectProfileWidget> AspectProfileWidgetClass;

	/**
	 * Parent material for the dish image (User Interface domain). Must expose a Texture parameter named "DishRender".
	 * Strongly recommended: assign this on your scorecard widget Blueprint (Class Defaults). If unset, resolves the brush material (or its parent if the brush is an instance/MID).
	 * Dish capture uses SceneColor (HDR): use Translucent/Additive UI blend as needed; alpha may follow engine "Inv Opacity" — try 1-A on A if the mask looks inverted.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Dish Image")
	TObjectPtr<UMaterialInterface> DishImageMaterial;

	/** Dynamic instance; "DishRender" is set to OverrideDishTexture (UTextureRenderTarget2D from capture or UTexture2D). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DishImageMID;

	/** Material parent the MID was created from (recreate MID if this changes). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DishImageMaterialUsedForMID;

	/** Seal textures for 4 grades: Perfect (A), Great (B), Okay (C), Needs Improvement (F) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Seals")
	TSoftObjectPtr<UTexture2D> SealTexturePerfect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Seals")
	TSoftObjectPtr<UTexture2D> SealTextureGreat;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Seals")
	TSoftObjectPtr<UTexture2D> SealTextureOkay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Seals")
	TSoftObjectPtr<UTexture2D> SealTextureNeedsImprovement;

	/** @deprecated Use SealTextureOkay. Fallback when SealTextureOkay is unset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Seals", meta = (DeprecationMessage = "Use SealTextureOkay"))
	TSoftObjectPtr<UTexture2D> SealTextureGood;

	/** Optional texture for checkmark (obtained ingredient). If unset, falls back to Unicode ✓. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Base Ingredients")
	TSoftObjectPtr<UTexture2D> CheckmarkTexture;

	/** Optional texture for X (missing ingredient). If unset, falls back to Unicode ✗. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Base Ingredients")
	TSoftObjectPtr<UTexture2D> XTexture;

	/** Size of checkmark/X image when using custom textures (default 32) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Base Ingredients", meta = (ClampMin = "8", ClampMax = "64"))
	float StatusIconSize = 32.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Scorecard")
	FPUScorecardData ScorecardData;

	UPROPERTY(BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UTexture> OverrideDishTexture;

	void UpdateDisplay();
	void UpdateSealImage();

	/** Returns true if a material parent was found and DishImageMID is valid for applying the dish texture. */
	bool EnsureDishImageMIDForDish();

	/** Debug: bind state + Slate visibility — filter [PUDialogueScoring] [DBG/ScorecardVisual]. */
	void LogScorecardVisualDebug(const TCHAR* Phase) const;
};
