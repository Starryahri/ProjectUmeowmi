#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "PUCommonUserWidget.h"
#include "PUAspectProfileWidget.h"
#include "PUScorecardTypes.h"
#include "PUScorecardWidget.generated.h"

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
};
