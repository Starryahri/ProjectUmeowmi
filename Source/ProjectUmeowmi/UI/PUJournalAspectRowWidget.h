// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PUCommonUserWidget.h"
#include "PUJournalAspectRowWidget.generated.h"

class UTextBlock;
class UHorizontalBox;
class UPanelWidget;
class UBorder;
class UDataTable;

/**
 * Single journal aspect row: aspect name + 5-star display (no ingredient icons).
 * Same layout role as PUAspectProfileWidget but only AspectNameText + StarRatingContainer — parent provides vertical lists.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUJournalAspectRowWidget : public UPUCommonUserWidget
{
	GENERATED_BODY()

public:
	UPUJournalAspectRowWidget(const FObjectInitializer& ObjectInitializer);

	/** Aspect label (e.g. FName "Umami") and star count 0–5. */
	UFUNCTION(BlueprintCallable, Category = "Journal|Aspects")
	void SetAspectNameAndStarRating(FName AspectName, int32 StarRating0to5);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Journal|Aspects")
	FName GetAspectName() const { return AspectName; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Journal|Aspects")
	int32 GetStarRating() const { return StarRating; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Aspects|UI")
	TObjectPtr<UTextBlock> AspectNameText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Aspects|UI")
	TObjectPtr<UHorizontalBox> StarRatingContainer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Aspects|Stars")
	TSoftObjectPtr<UTexture2D> StarTextureFilled;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Aspects|Stars")
	TSoftObjectPtr<UTexture2D> StarTextureUnfilled;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Aspects|Stars", meta = (ClampMin = "8", ClampMax = "128"))
	float StarImageSize = 24.0f;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Aspects|UI")
	TObjectPtr<UBorder> AspectBorder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal|Aspects", meta = (RequiredAssetDataTags = "RowStructure=/Script/UMG.RichTextStyleRow"))
	TObjectPtr<UDataTable> AspectColorDataTable;

	void UpdateDisplay();

	UPROPERTY()
	FName AspectName;

	UPROPERTY()
	int32 StarRating = 0;
};
