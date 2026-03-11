#pragma once

#include "CoreMinimal.h"
#include "PUCommonUserWidget.h"
#include "PUScorecardTypes.h"
#include "PUAspectProfileWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UHorizontalBox;

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

	/** Container for top 3 contributing ingredients */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UHorizontalBox> IngredientsContainer;

	/** Container for star rating (5 stars) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Scorecard")
	TObjectPtr<UHorizontalBox> StarRatingContainer;

	UPROPERTY(BlueprintReadOnly, Category = "Scorecard")
	FPUAspectRanking AspectData;

	void UpdateDisplay();
};
