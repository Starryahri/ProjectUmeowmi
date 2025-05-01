#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PUCommonUserWidget.generated.h"

/**
 * Base widget class for all UI widgets in the ProjectUmeowmi project.
 * Inherits from CommonUserWidget to provide consistent styling and behavior.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTUMEOWMI_API UPUCommonUserWidget : public UCommonUserWidget
{
    GENERATED_BODY()

public:
    /** Constructor */
    UPUCommonUserWidget(const FObjectInitializer& ObjectInitializer);

    /** Called when the widget is constructed */
    virtual void NativeConstruct() override;

    /** Called when the widget is destroyed */
    virtual void NativeDestruct() override;
}; 