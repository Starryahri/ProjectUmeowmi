#pragma once

#include "CoreMinimal.h"
#include "DlgSystem/DlgNodeData.h"
#include "PUDialogueNodeData.generated.h"

/**
 * Optional per-speech-node data. Add as Node Data on Dlg speech nodes.
 * Portrait layout (giant vs default slot) is independent of Speaker State, which remains for GetParticipantIcon / expressions.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTUMEOWMI_API UPUDialogueNodeData : public UDlgNodeData
{
	GENERATED_BODY()

public:
	/** When true, PUDialogueBox shows GiantParticipantImage instead of ParticipantImage. Does not affect which texture is used (Speaker State still drives the icon). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait")
	bool bUseGiantPortraitSlot = false;
};
