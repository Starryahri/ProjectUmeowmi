#pragma once

#include "CoreMinimal.h"
#include "PUCommonUserWidget.h"
#include "PUScorecardTypes.h"
#include "Engine/Texture2D.h"
#include "PUAspectProfileWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UPanelWidget;
class UBorder;
class UDataTable;

/**
 * Widget that displays ONE aspect: aspect name, top 3 contributing ingredients, and 5-star rating.
 * The scorecard shows two of these per profile (flavor + texture).
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUAspectProfileWidget : public UPUCommonUserWidget
{
	GENERATED_BODY()

public:
	UPUAspectProfileWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * Set the data for this single aspect.
	 * @param InRanking - One aspect: name, top 3 ingredients, star rating
	 */
	UFUNCTION(BlueprintCallable, Category = "Scorecard")
	void SetAspectData(const FPUAspectRanking& InRanking);

	/** Get current aspect data */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scorecard")
	const FPUAspectRanking& GetAspectData() const { return AspectData; }

protected:
	virtual void NativeConstruct() override;

	/** Aspect name (e.g. "Umami", "Crispy") */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UTextBlock> AspectNameText;

	/** Container for top 3 contributing ingredient icons - bind any panel */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UPanelWidget> IngredientsContainer;

	/** Container for star rating (5 stars) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UHorizontalBox> StarRatingContainer;

	/** Filled star texture - if set with StarTextureUnfilled, uses images instead of ★/☆ text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Stars")
	TSoftObjectPtr<UTexture2D> StarTextureFilled;

	/** Unfilled/empty star texture - if set with StarTextureFilled, uses images instead of ★/☆ text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Stars")
	TSoftObjectPtr<UTexture2D> StarTextureUnfilled;

	/** Size of each star image when using custom textures (default 24) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard|Stars", meta = (ClampMin = "8", ClampMax = "128"))
	float StarImageSize = 24.0f;

	/** Border to tint with the aspect color from AspectColorDataTable (row name = aspect name) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UBorder> AspectBorder;

	/** Rich Text Style data table - row names = aspect names (Umami, Salt, Sweet, etc.). Color from TextStyle.ColorAndOpacity is applied to AspectBorder. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scorecard", meta = (RequiredAssetDataTags = "RowStructure=/Script/UMG.RichTextStyleRow"))
	TObjectPtr<UDataTable> AspectColorDataTable;

	UPROPERTY(BlueprintReadOnly, Category = "Scorecard")
	FPUAspectRanking AspectData;

	void UpdateDisplay();
};
