#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUAspectRadarIconRow.generated.h"

class UTexture2D;

/**
 * Data table row: maps a dish-profile aspect gameplay tag (e.g. Profile.Flavor.Umami) to a radar segment icon.
 * Row names are free-form; lookup is by AspectTag.
 */
USTRUCT(BlueprintType)
struct PROJECTUMEOWMI_API FPUAspectRadarIconRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aspect Icon", meta = (Categories = "Profile"))
	FGameplayTag AspectTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aspect Icon")
	TObjectPtr<UTexture2D> Icon = nullptr;
};
