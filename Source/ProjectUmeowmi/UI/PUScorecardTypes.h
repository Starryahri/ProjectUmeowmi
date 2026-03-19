#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "PUScorecardTypes.generated.h"

/** Single base ingredient entry for scorecard display (name + optional icon + obtained status). */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUBaseIngredientEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	TObjectPtr<UTexture2D> PreviewTexture = nullptr;

	/** True if the player included this core/base ingredient in the completed dish. Used for checkmark/X display. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	bool bObtained = true;
};

/** Seal of approval tier - 4 grades for scorecard display (A/B/C/F). */
UENUM(BlueprintType)
enum class EPUScorecardSealTier : uint8
{
	Perfect = 0 UMETA(DisplayName = "Perfect (A)"),
	Great = 1 UMETA(DisplayName = "Great (B)"),
	Okay = 2 UMETA(DisplayName = "Okay (C)"),
	NeedsImprovement = 3 UMETA(DisplayName = "Needs Improvement (F)")
};

/** Single aspect ranking: aspect name, top 3 contributing ingredients (icons), total value, and 0-5 star rating. */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUAspectRanking
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	FName AspectName;

	/** Top 3 contributing ingredients (icon + optional name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	TArray<FPUBaseIngredientEntry> TopContributingIngredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	float TotalValue = 0.0f;

	/** Integer star rating 0-5. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	int32 StarRating = 0;
};

/** Profile data for flavor or texture: top 2 aspects, each with top 3 ingredients, plus overall star rating. */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUAspectProfileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	TArray<FPUAspectRanking> TopAspects;

	/** Overall profile star rating 0-5 (integer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	int32 StarRating = 0;
};

/** Full scorecard data for display when an order is completed. */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUScorecardData
{
	GENERATED_BODY()

	/** Display name shown on the scorecard (e.g. dish name or order giver name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	FText DisplayName;

	/** Seal tier based on satisfaction score (Perfect/Great/Good). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	EPUScorecardSealTier SealTier = EPUScorecardSealTier::Okay;

	/** Base ingredients from the recipe or completed dish (display name + icon). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	TArray<FPUBaseIngredientEntry> BaseIngredients;

	/** Flavor profile: top 2 aspects, top 3 ingredients each, star rating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	FPUAspectProfileData FlavorProfile;

	/** Texture profile: top 2 aspects, top 3 ingredients each, star rating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scorecard")
	FPUAspectProfileData TextureProfile;
};
