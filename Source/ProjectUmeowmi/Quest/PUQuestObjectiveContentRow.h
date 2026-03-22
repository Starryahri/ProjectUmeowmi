#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUQuestObjectiveContentRow.generated.h"

class UTexture2D;

/**
 * Authoring row for quest objectives: titles, descriptions, icons.
 * Set each Data Table row name to the objective's gameplay tag (e.g. Quest.Sample.Talk), or use ObjectiveTag and search fallback.
 */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUQuestObjectiveContentRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Must match the gameplay tag used in code / dialogue for this objective. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (Categories = "Quest"))
	FGameplayTag ObjectiveTag;

	/** Parent quest tag (for grouping / journal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (Categories = "Quest"))
	FGameplayTag QuestTag;

	/** Display name for the quest (can repeat on each objective row of the same quest). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText QuestTitle;

	/** Short line for HUD tracker / prompts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText ObjectiveTitle;

	/** Longer description for journal / details UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText ObjectiveDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TSoftObjectPtr<UTexture2D> Icon;
};

/** Resolved display data for UI (Blueprint-friendly). */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUQuestObjectiveDisplayInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	bool bFound = false;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FGameplayTag ObjectiveTag;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FGameplayTag QuestTag;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FText QuestTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FText ObjectiveTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FText ObjectiveDescription;

	/** Loaded icon (may be null if soft ref missing or not loaded). */
	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TObjectPtr<UTexture2D> Icon = nullptr;
};
