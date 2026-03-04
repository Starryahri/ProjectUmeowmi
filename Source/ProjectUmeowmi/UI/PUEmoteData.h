#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUEmoteData.generated.h"

class UTexture2D;
class USoundBase;

/**
 * Data describing a single emote that can be shown over a character's head.
 * Intended for use in a DataTable so designers can add and tweak emotes in the editor.
 */
USTRUCT(BlueprintType)
struct FPUEmoteData : public FTableRowBase
{
	GENERATED_BODY()

	FPUEmoteData()
		: Duration(2.0f)
		, bLoop(false)
	{
	}

	/** Gameplay tag used to request this emote (e.g. Emote.Happy, Emote.Confused). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emote")
	FGameplayTag EmoteTag;

	/** Icon displayed in the emote widget above the character. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emote")
	UTexture2D* Icon = nullptr;

	/** Optional sound to play when the emote appears. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emote")
	USoundBase* Sound = nullptr;

	/** How long the emote should stay visible, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emote", meta = (ClampMin = "0.0"))
	float Duration;

	/** When true, the emote does not auto-dismiss and must be cleared explicitly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emote")
	bool bLoop;
};

