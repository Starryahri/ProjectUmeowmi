#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUDialogueLineContentRow.generated.h"

/**
 * Authoring row for dialogue lines when Dlg speech text is a gameplay tag id (e.g. D.Chapter.Scene.Line01).
 * Prefer Data Table row name == tag string; LineTag is optional fallback for CSV/rename workflows.
 */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUDialogueLineContentRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Optional; used if row name does not match the tag string. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (Categories = "D"))
	FGameplayTag LineTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText Line;
};
