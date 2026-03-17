#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "PUCommonButton.generated.h"

/**
 * Project base button class for all Common UI buttons in ProjectUmeowmi.
 * Inherits from CommonButtonBase so one style (or one Blueprint based on this class)
 * can drive the look of every button. Set the Style in the editor or in a child Blueprint
 * to change all instances at once.
 */
UCLASS(Blueprintable, ClassGroup = UI, meta = (Category = "Project Umeowmi"))
class PROJECTUMEOWMI_API UPUCommonButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPUCommonButton(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
};
